#include <driver/uart.h>
#include <driver/gpio.h>
#include <driver/adc.h>
#include <array>
#include <string.h>
#include <algorithm>
#include <functional>
#include <mutex>
#include <vector>

#include "esp_vfs.h"
#include "esp_spiffs.h"
#include "esp_log.h"
#include "esp_system.h"

#include "lvgl.h"
#include "lvgl_helpers.h"

#include "lvglui.hpp"

static void guiTask(void*);
static int mouseX = 0;
static int mouseY = 0;

struct KeyEvent {
    int code;
    bool pressed;
};
static QueueHandle_t keyEventsQueue = NULL;

struct UiContext {
public:
    void createWidgets(lv_indev_t *kbInput) {
        this->kbInput = kbInput;
        this->kbGroup = lv_group_create();
        lv_indev_set_group(kbInput, kbGroup);

        lv_obj_t * scr = lv_disp_get_scr_act(NULL);

        // Layout
        auto *layout = lv_cont_create(scr, NULL);
        lv_obj_set_style_local_pad_all(layout, LV_CONT_PART_MAIN, LV_STATE_DEFAULT, 4);
        lv_obj_set_auto_realign(layout, true);
        lv_obj_align_origo(layout, NULL, LV_ALIGN_CENTER, 0, 0);
        lv_cont_set_fit(layout, LV_FIT_TIGHT);
        lv_cont_set_layout(layout, LV_LAYOUT_PRETTY_MID);

        // label
        auto *label = lv_label_create(layout, NULL);
        lv_label_set_text(label, "Press the button!");

        // Button 1
        auto *btn = lv_btn_create(layout, NULL);
        lv_group_add_obj(kbGroup, btn);
        lv_obj_set_user_data(btn, this);
        lv_obj_set_event_cb(btn, btnCallback);

        label = lv_label_create(btn, NULL);
        lv_label_set_text(label, "THE BUTTON");

        // Button 2
        btn = lv_btn_create(layout, NULL);
        lv_group_add_obj(kbGroup, btn);

        label = lv_label_create(btn, NULL);
        lv_label_set_text(label, "Not btn");
    }

    lv_obj_t *halBar = NULL;
private:
    lv_indev_t *kbInput;
    lv_group_t *kbGroup;
    lv_obj_t *activeWindow = NULL;

    void showHalWindow() {
        if(activeWindow != NULL) {
            lv_obj_del(activeWindow);
        }

        lv_obj_t *scr = lv_disp_get_scr_act(NULL);

        activeWindow = lv_win_create(scr, NULL);
        lv_win_set_title(activeWindow, "HAL sensor");
        lv_obj_set_user_data(activeWindow, this);
        lv_obj_set_event_cb(activeWindow, onHalWindowDelete);

        auto *closeBtn = lv_win_add_btn_right(activeWindow, LV_SYMBOL_CLOSE);
        lv_obj_set_user_data(closeBtn, this);
        lv_group_add_obj(kbGroup, closeBtn);
        lv_obj_set_event_cb(closeBtn, closeWindow);

        halBar = lv_bar_create(activeWindow, NULL);
        lv_obj_set_width(halBar, lv_obj_get_width(activeWindow)-20);
        lv_obj_align(halBar, NULL, LV_ALIGN_CENTER, 0, 40);
        lv_bar_set_range(halBar, 0, 1 << 12);
    }

    static void onHalWindowDelete(lv_obj_t *wnd, lv_event_t event) {
        if(event == LV_EVENT_DELETE) {
            auto *ctx = (UiContext*)lv_obj_get_user_data(wnd);
            ctx->halBar = NULL;
        }
    }

    static void closeWindow(lv_obj_t *btn, lv_event_t event) {
        auto *ctx = (UiContext*)lv_obj_get_user_data(btn);
        if(event == LV_EVENT_CLICKED && ctx->activeWindow != NULL) {
            lv_obj_del(ctx->activeWindow);
            ctx->activeWindow = NULL;
        }
    }

    static void btnCallback(lv_obj_t *obj, lv_event_t event) {
        switch(event) {
            case LV_EVENT_CLICKED: {
                auto *ctx = (UiContext*)lv_obj_get_user_data(obj);
                printf("BTN clicked\n");
                ctx->showHalWindow();
                break;
            }
            default:
                //printf("Other event: %d\n", event);
                break;
        }
    }
};

static LvglUiHelper<UiContext> gUi;

extern "C" void app_main()
{
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_0, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_0, 1, 3, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0, 256, 256, 0, NULL, 0));

    keyEventsQueue = xQueueCreate(8, sizeof(KeyEvent));

    adc1_config_width(ADC_WIDTH_BIT_12);

    xTaskCreatePinnedToCore(guiTask, "gui", 4096*2, NULL, 0, NULL, 1);

    uint8_t buf[16];
    while(1) {
        gUi.modify([](UiContext& ctx) {
            if(ctx.halBar != NULL) {
                lv_bar_set_value(ctx.halBar, xTaskGetTickCount() % (1 << 12), false);
                printf("HALL: %d\n", hall_sensor_read());
            }
        });

        auto read = uart_read_bytes(UART_NUM_0, buf, sizeof(buf), 1);
        if(read <= 0 || buf[0] != 0xFF) {
            continue;
        }

        if(read >= 6 && buf[1] == 0x04) {
            const int16_t dx = buf[2] | (buf[3] << 8);
            const int16_t dy = buf[4] | (buf[5] << 8);
            mouseX = std::min(135, std::max(0, mouseX+dx));
            mouseY = std::min(240, std::max(0, mouseY+dy));
        } else if(read >= 4 && buf[1] == 0x01) {
            KeyEvent ev = {
                .code = buf[2],
                .pressed = buf[3] != 0,
            };
            xQueueSend(keyEventsQueue, &ev, portMAX_DELAY);

            // lorris can't handle press+release
            if(ev.pressed) {
                ev.pressed = false;
                xQueueSend(keyEventsQueue, &ev, portMAX_DELAY);
            }
        }
    }
}

static bool readMouse(lv_indev_drv_t * drv, lv_indev_data_t*data) {
    data->point.x = mouseX;
    data->point.y = mouseY;
    data->state = LV_INDEV_STATE_PR;
    return false;
}

static bool readkeyboard(lv_indev_drv_t * drv, lv_indev_data_t*data) {
    KeyEvent ev;
    if(xQueueReceive(keyEventsQueue, &ev, 0) == pdFALSE) {
        return false;
    }

    data->key = ev.code;
    data->state = ev.pressed ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;

    printf("press %d %d\n", ev.code, ev.pressed);

    return uxQueueMessagesWaiting(keyEventsQueue) > 0;
}

static void guiTask(void*) {
    lv_init();
    lvgl_driver_init();

    lv_color_t* buf1 = (lv_color_t*)heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
    lv_color_t* buf2 = (lv_color_t*)heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);

    lv_disp_buf_t disp_buf;
    uint32_t size_in_px = DISP_BUF_SIZE;

    lv_disp_buf_init(&disp_buf, buf1, buf2, size_in_px);

    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.flush_cb = disp_driver_flush;

    disp_drv.buffer = &disp_buf;
    lv_disp_drv_register(&disp_drv);

    // Mouse cursor
    lv_indev_drv_t mouse_drv;
    lv_indev_drv_init(&mouse_drv);
    mouse_drv.type = LV_INDEV_TYPE_POINTER;
    mouse_drv.read_cb = readMouse;

    lv_indev_t *mouse_indev = lv_indev_drv_register(&mouse_drv);

    LV_IMG_DECLARE(mouse_cursor_icon);                          /*Declare the image file.*/
    lv_obj_t *cursor_obj =  lv_img_create(lv_scr_act(), NULL); /*Create an image object for the cursor */
    lv_img_set_src(cursor_obj, &mouse_cursor_icon);             /*Set the image source*/
    lv_indev_set_cursor(mouse_indev, cursor_obj);               /*Connect the image  object to the driver*/


    // Keyboard
    lv_indev_drv_t kb_drv;
    lv_indev_drv_init(&kb_drv);
    kb_drv.type = LV_INDEV_TYPE_KEYPAD;
    kb_drv.read_cb = readkeyboard;

    lv_indev_t *kb_indev = lv_indev_drv_register(&kb_drv);

    gUi.modify([=](UiContext& ctx) {
        ctx.createWidgets(kb_indev);
    });

    auto lastTicks = xTaskGetTickCount();
    while (1) {
        vTaskDelay(1);

        const auto curTicks = xTaskGetTickCount();
        lv_tick_inc(pdTICKS_TO_MS(curTicks - lastTicks));
        lastTicks = curTicks;

        gUi.modify([](UiContext&) {
            lv_task_handler();
        });
    }
}
