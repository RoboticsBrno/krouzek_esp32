#include <driver/uart.h>
#include <driver/gpio.h>
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

#include "apriltag/apriltag.h"
#include "apriltag/tag16h5.h"

static constexpr const int WIDTH = 176;
static constexpr const int HEIGHT = 144;

static uint8_t frame[WIDTH * HEIGHT];

static void waitForMagic()
{
    static const uint8_t magic[] = {0xFF, 0x10, 0x00, 0x80};
    uint8_t b = 0;
    int idx = 0;

    while (1)
    {
        uart_read_bytes(UART_NUM_0, &b, 1, portMAX_DELAY);
        if (b == magic[idx])
        {
            if (++idx >= sizeof(magic))
            {
                return;
            }
        }
        else
        {
            idx = 0;
        }
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
        };
        ESP_ERROR_CHECK(uart_param_config(UART_NUM_0, &uart_config));
        ESP_ERROR_CHECK(uart_set_pin(UART_NUM_0, 1, 3, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
        ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0, 1024, 256, 0, NULL, 0));
    }

    auto *family = &tag16h5_family;
    auto *detector = apriltag_detector_create();

    // Sets family and how many wrong bits to tolerate
    apriltag_detector_add_family_bits(detector, family, 0);

    detector->quad_decimate = 1; // Decimate input image by this factor
    detector->quad_sigma = 0.0;  // Apply low-pass blur to input; negative sharpens
    detector->nthreads = 1;
    detector->debug = false;
    detector->refine_edges = true; // Spend more time trying to align edges of tags

    while (1)
    {
        waitForMagic();
        int frameReadRes = uart_read_bytes(UART_NUM_0, frame, sizeof(frame), portMAX_DELAY);
        if (frameReadRes != sizeof(frame))
        {
            continue;
        }

        image_u8_t im = {
            .width = WIDTH,
            .height = HEIGHT,
            .stride = WIDTH,
            .buf = frame,
        };

        printf("Got frame %d\n", frameReadRes);

        const auto start = xTaskGetTickCount();
        zarray_t *detections = apriltag_detector_detect(detector, &im);
        const auto stop = xTaskGetTickCount();

        printf("Got detections %d in %d ms\n", zarray_size(detections), stop - start);

        for (int i = 0; i < zarray_size(detections); i++)
        {
            apriltag_detection_t *det;
            zarray_get(detections, i, &det);

            printf("   ID %d, err %d/%f at %fx%f\n", det->id, det->hamming, det->decision_margin, det->c[0], det->c[1]);
        }
        apriltag_detections_destroy(detections);
    }
}
