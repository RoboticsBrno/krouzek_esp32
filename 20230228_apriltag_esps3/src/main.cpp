#include <driver/uart.h>
#include <driver/gpio.h>
#include <array>
#include <string.h>
#include <algorithm>
#include <functional>
#include <mutex>
#include <math.h>
#include <vector>
#include <mutex>

#include "esp_vfs.h"
#include "esp_spiffs.h"
#include "esp_log.h"
#include "esp_system.h"

#include "gridui.h"
#include "rbprotocol.h"
#include "rbwebserver.h"
#include "rbwifi.h"

#include "rbcam.h"

#define GRIDUI_LAYOUT_DEFINITION
#include "layout.hpp"

extern "C" {
#include "st7789.h"
};



using namespace rb;
using namespace gridui;

static Protocol* gProt = nullptr;

static std::vector<RbCameraTagDetected> gCurrentTags;
static std::mutex gCurrentTagsMu;

static void onPacketReceived(const std::string& cmd, rbjson::Object* pkt) {
    // Let GridUI handle its packets
    if (UI.handleRbPacket(cmd, pkt))
        return;

    // ...any other non-GridUI packets
}

static void displayCameraTask(void *displayVoid) {
    auto disp = (TFT_t*)displayVoid;
    auto& cam = RbCamera::get();

    uint8_t *rgb565buf = NULL;
    size_t rgb565buf_size = 0;
    while(true) {
        vTaskDelay(pdMS_TO_TICKS(16));

        const int border_px_count = (320-240)/2;

        auto fb = cam.getLastFb();
        if(!fb || fb->len == 0) {
            continue;
        }

        if(rgb565buf == NULL) {
            rgb565buf_size = fb->width * fb->height * 2;
            rgb565buf = (uint8_t*)heap_caps_malloc(rgb565buf_size, MALLOC_CAP_SPIRAM);
        }


        if(!jpg2rgb565(fb->buf, fb->len, rgb565buf, JPG_SCALE_NONE)) {
            ESP_LOGE("main", "failed to convert camera frame to RGB565");
            continue;
        }

        fb.reset();

        uint16_t *start = (uint16_t*)(rgb565buf) + border_px_count;
        //lcdDrawMultiPixels(disp, 0, 0, 240, start);
        {
            spi_master_write_command(disp, 0x2A);	// set column(x) address
            spi_master_write_addr(disp, 0, 240);
            spi_master_write_command(disp, 0x2B);	// set Page(y) address
            spi_master_write_addr(disp, 0, 240);
            spi_master_write_command(disp, 0x2C);	//	Memory Write
        }
        for(int y = 1; y < 240; y++) {
            //lcdDrawMultiPixels(disp, 0, y, 240, start);
            start += 240 + border_px_count * 2;
            spi_master_write_colors(disp, start, 240);
        }

        gCurrentTagsMu.lock();
        for(const auto& tag : gCurrentTags) {
            const auto& c = tag.corners;
            lcdDrawLine(disp, c[0][0] - border_px_count, c[0][1], c[1][0] - border_px_count, c[1][1], RED);
            lcdDrawLine(disp, c[1][0] - border_px_count, c[1][1], c[2][0] - border_px_count, c[2][1], RED);
            lcdDrawLine(disp, c[2][0] - border_px_count, c[2][1], c[3][0] - border_px_count, c[3][1], RED);
            lcdDrawLine(disp, c[3][0] - border_px_count, c[3][1], c[0][0] - border_px_count, c[0][1], RED);
        }
        gCurrentTagsMu.unlock();
        
    }
}

extern "C" void app_main()
{
    {
        uart_config_t uart_config = {
            .baud_rate = 921600,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .source_clk = UART_SCLK_DEFAULT,
        };
        ESP_ERROR_CHECK(uart_param_config(UART_NUM_0, &uart_config));
        ESP_ERROR_CHECK(uart_set_pin(UART_NUM_0, 1, 3, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
        ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0, 1024, 1024, 0, NULL, 0));
    }

    TFT_t disp = {};

    spi_master_init(&disp, 47, 21, 44, 43, -1, 48);
    lcdInit(&disp, 240, 240, 0, 0);

    lcdBacklightOff(&disp);

    const RbCameraConfig camCfg = {
        .framesize = FRAMESIZE_QVGA,
        .jpeg_quality = 8,
        .enable_apriltag = true,
        .sync_tag_frames = false,
        .quad_decimate = 2,
        .refine_edges = true,
    };

    // Initialize WiFi
    WiFi::connect("Anthrophobia", "Ku1ata2elvA");
    //WiFi::startAp("TagDemo", "12345678");

    // Initialize RBProtocol
    gProt = new Protocol("FrantaFlinta", "RobocopS3", "Compiled at " __DATE__ " " __TIME__, onPacketReceived);
    gProt->start();

    // Start serving the web page
    rb_web_start(80);
    rb_web_set_extra_callback(RbCamera::rbWebCallback);

    UI.begin(gProt);

    // Build the UI widgets. Positions/props are set in the layout, so most of the time,
    // you should only set the event handlers here.
    auto builder = Layout.begin();

    builder.enableAprilTag.checked(camCfg.enable_apriltag);
    builder.enableAprilTag.onChanged([](Checkbox& b) {
        RbCamera::get().setAprilTagsEnabled(b.checked());
    });

    builder.enableFrameSync.checked(camCfg.sync_tag_frames);
    builder.enableFrameSync.onChanged([](Checkbox& b) {
        RbCamera::get().setSyncTagFrames(b.checked());
    });

    // Commit the layout. Beyond this point, calling any builder methods on the UI is invalid.
    builder.commit();

    auto& cam = RbCamera::get();
    ESP_ERROR_CHECK(cam.init(camCfg));

    char buf[16];
    uint32_t tagCount = 0;
    TickType_t lastTagAt = 0;

    xTaskCreate(displayCameraTask, "lcd_cam", 8192, &disp, 3, NULL);

    RbCameraTagDetected tag;
    while (1)
    {
        if(xQueueReceive(cam.tagQueue(), &tag, pdMS_TO_TICKS(500)) == pdTRUE) {
            ++tagCount;
            Layout.tagsCnt.setNumber(tagCount);

            snprintf(buf, sizeof(buf), "%.1fx%.1f", tag.center[0], tag.center[1]);
            Layout.tagCoords.setText(buf);

            Layout.tagId.setNumber(tag.id);

            printf("Tag detected %d at %f %f\n", tag.id, tag.center[0], tag.center[1]);

            gCurrentTagsMu.lock();
            if(lastTagAt != 0 && xTaskGetTickCount() - lastTagAt > pdMS_TO_TICKS(50)) {
                Layout.Camera1.clearTags();
                gCurrentTags.clear();
            }
            gCurrentTags.push_back(tag);
            gCurrentTagsMu.unlock();

            Camera::Tag ct;
            ct.id = tag.id;
            memcpy(ct.corners, tag.corners, sizeof(tag.corners));
            Layout.Camera1.addTag(ct);
            lastTagAt = xTaskGetTickCount();

            
        }

        Layout.ramCurrent.setValue(heap_caps_get_free_size(MALLOC_CAP_INTERNAL)/1024);
        Layout.ramMin.setValue(heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL)/1024);

        if(lastTagAt != 0 && xTaskGetTickCount() - lastTagAt > pdMS_TO_TICKS(3000)) {
            lastTagAt = 0;
            Layout.Camera1.clearTags();

            gCurrentTagsMu.lock();
            gCurrentTags.clear();
            gCurrentTagsMu.unlock();
        }
    }
}
