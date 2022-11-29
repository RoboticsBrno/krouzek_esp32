#include <driver/uart.h>
#include <driver/gpio.h>
#include <array>
#include <string.h>
#include <algorithm>
#include <functional>
#include <mutex>
#include <math.h>
#include <vector>

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


using namespace rb;
using namespace gridui;

static Protocol* gProt = nullptr;

static void onPacketReceived(const std::string& cmd, rbjson::Object* pkt) {
    // Let GridUI handle its packets
    if (UI.handleRbPacket(cmd, pkt))
        return;

    // ...any other non-GridUI packets
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
        };
        ESP_ERROR_CHECK(uart_param_config(UART_NUM_0, &uart_config));
        ESP_ERROR_CHECK(uart_set_pin(UART_NUM_0, 1, 3, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
        ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0, 1024, 1024, 0, NULL, 0));
    }

    // Initialize WiFi
    WiFi::connect("Anthrophobia", "Ku1ata2elvA");

    // Initialize RBProtocol
    gProt = new Protocol("FrantaFlinta", "Robocop", "Compiled at " __DATE__ " " __TIME__, onPacketReceived);
    gProt->start();

    // Start serving the web page
    rb_web_start(80);
    rb_web_set_extra_callback(RbCamera::rbWebCallback);

    UI.begin(gProt);

    // Build the UI widgets. Positions/props are set in the layout, so most of the time,
    // you should only set the event handlers here.
    auto builder = Layout.begin();

    // Commit the layout. Beyond this point, calling any builder methods on the UI is invalid.
    builder.commit();

    RbCameraConfig camCfg = {
        .framesize = FRAMESIZE_QVGA,
        .jpeg_quality = 8,
        .quad_decimate = 2,
        .refine_edges = true,
    };

    auto& cam = RbCamera::get();
    ESP_ERROR_CHECK(cam.init(camCfg));

    RbCameraTagDetected tag;
    while (1)
    {
        if(xQueueReceive(cam.tagQueue(), &tag, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        printf("Tag detected %d at %f %f\n", tag.id, tag.center[0], tag.center[1]);
    }
}
