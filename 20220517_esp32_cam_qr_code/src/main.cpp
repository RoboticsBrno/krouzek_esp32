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

#include "esp_camera.h"

#include "quirc/quirc.h"

static const camera_config_t camera_config = {
    .pin_pwdn  = 32,
    .pin_reset = -1,
    .pin_xclk = 0,
    .pin_sscb_sda = 26,
    .pin_sscb_scl = 27,

    .pin_d7 = 35,
    .pin_d6 = 34,
    .pin_d5 = 39,
    .pin_d4 = 36,
    .pin_d3 = 21,
    .pin_d2 = 19,
    .pin_d1 = 18,
    .pin_d0 = 5,
    .pin_vsync = 25,
    .pin_href = 23,
    .pin_pclk = 22,

    .xclk_freq_hz = 20000000, //EXPERIMENTAL: Set to 16MHz on ESP32-S2 or ESP32-S3 to enable EDMA mode
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_GRAYSCALE, //YUV422,GRAYSCALE,RGB565,JPEG
    .frame_size = FRAMESIZE_QVGA, //QQVGA-QXGA Do not use sizes above QVGA when not JPEG

    .jpeg_quality = 12, //0-63 lower number means higher quality
    .fb_count = 1, //if more than one, i2s runs in continuous mode. Use only with JPEG
    .grab_mode = CAMERA_GRAB_LATEST//CAMERA_GRAB_LATEST. Sets when buffers should be filled
};

static void extract_codes(struct quirc *quirc) {
    const int count = quirc_count(quirc);
    if(count == 0) {
        return;
    }

    struct quirc_code *code = new struct quirc_code;
    struct quirc_data *data = new struct quirc_data;

    for(int i = 0; i < count; ++i) {
        quirc_extract(quirc, i, code);
        auto err = quirc_decode(code, data);
        if(err != QUIRC_SUCCESS) {
            ESP_LOGE("qr", "failed to decode QR %d: %d %s", i, err, quirc_strerror(err));
            continue;
        }
        printf("QR %d: ver %d ecc %d data-type %d payload: %*s\n",
            i, data->version, data->ecc_level, data->data_type, data->payload_len, data->payload);
    }

    delete code;
    delete data;
}

static void qr_recognize_task(void*) {
    ESP_ERROR_CHECK(esp_camera_init(&camera_config));

    struct quirc *quirc = NULL;

    while(1) {
        vTaskDelay(pdMS_TO_TICKS(10));

        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) {
            ESP_LOGE("camera", "Camera Capture Failed");
            continue;
        }

        if(quirc == NULL) {
            quirc = quirc_new();
            if(quirc_resize(quirc, fb->width, fb->height) != 0) {
                ESP_LOGE("camera", "failed to quirc_resize!");
                abort();
            }
        }

        printf("quirc begin\n");

        auto *qr_buf = quirc_begin(quirc, NULL, NULL);
        memcpy(qr_buf, fb->buf, fb->len);
        esp_camera_fb_return(fb);
        quirc_end(quirc);

        
        printf("detected codes: %d\n", quirc_count(quirc));
        extract_codes(quirc);
    }
}


extern "C" void app_main()
{
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
    }


    if(camera_config.pin_pwdn != -1) {
        gpio_set_direction((gpio_num_t)camera_config.pin_pwdn, GPIO_MODE_OUTPUT);
        gpio_set_level((gpio_num_t)camera_config.pin_pwdn, 0);
    }

    xTaskCreate(qr_recognize_task, "qrcode", 96*1024, NULL, 5, NULL);

    while(1) {
        vTaskDelay(1000);
    }
}
