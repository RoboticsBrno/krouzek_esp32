#include <driver/uart.h>
#include <driver/gpio.h>
#include <array>
#include <string.h>
#include <algorithm>
#include <functional>
#include <mutex>

#include "esp_vfs.h"
#include "esp_spiffs.h"
#include "esp_log.h"
#include "esp_system.h"

#include "lvgl.h"
#include "lvgl_helpers.h"

#include "lvglui.hpp"

static void guiTask(void*);

struct UiContext {
    lv_obj_t *label1;
    lv_obj_t *bar1;

    void createWidgets() {
        /* use a pretty small demo for monochrome displays */
        /* Get the current screen */
        lv_obj_t *scr = lv_disp_get_scr_act(NULL);

        /*Create a Label on the currently active screen*/
        this->label1 = lv_label_create(scr, NULL);

        /*Modify the Label's text*/
        lv_label_set_text(this->label1, "Hello\nworld");

        /* Align the Label to the center
        * NULL means align on parent (which is the screen now)
        * 0, 0 at the end means an x, y offset after alignment*/
        lv_obj_align(this->label1, NULL, LV_ALIGN_CENTER, 0, 0);

        this->bar1 = lv_bar_create(scr, NULL);
        lv_obj_set_width(this->bar1, lv_obj_get_width(scr)-20);
        lv_obj_align(this->bar1, NULL, LV_ALIGN_CENTER, 0, 40);
    }
};

static LvglUiHelper<UiContext> gUi;

extern "C" void app_main()
{
    xTaskCreatePinnedToCore(guiTask, "gui", 4096*2, NULL, 0, NULL, 1);

    int val = 0;
    while(1) {
        vTaskDelay(100);

        val += 10;
        gUi.modify([=](UiContext& ctx) {
            lv_bar_set_value(ctx.bar1, val%100, true);
        }); 
    }
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

    gUi.modify([](UiContext& ctx) {
        ctx.createWidgets();
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
