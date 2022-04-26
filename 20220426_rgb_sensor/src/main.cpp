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


#include "I2Cbus.hpp"

#define TSC_ADDR 0x29

#define TSC_REG_INTEGRATION (0x80 | 0x01)
#define TSC_REG_CONTROL (0x80 | 0x0F)
#define TSC_REG_ENABLE (0x80 | 0x00)

#define TSC_REG_CDATA_L (0x80 | 0x14)

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



    i2c0.begin(GPIO_NUM_27, GPIO_NUM_14, 400000);


    // Set Integration time to 24ms
    ESP_ERROR_CHECK(i2c0.writeByte(TSC_ADDR, TSC_REG_INTEGRATION, 0xF6));
    // Set Gain to 4x
    ESP_ERROR_CHECK(i2c0.writeByte(TSC_ADDR, TSC_REG_CONTROL, 0x01));

    // power on (it starts in disabled mode)
    ESP_ERROR_CHECK(i2c0.writeByte(TSC_ADDR, TSC_REG_ENABLE, 0x01));
    ESP_ERROR_CHECK(i2c0.writeByte(TSC_ADDR, TSC_REG_ENABLE, 0x01 | 0x02)); // PON | AEN

    while(1) {
        vTaskDelay(10);

        uint16_t crgb[4] = {};
        ESP_ERROR_CHECK(i2c0.readBytes(TSC_ADDR, TSC_REG_CDATA_L, sizeof(crgb), (uint8_t*)&crgb));

        //printf("%d %d %d %d\n", crgb[0], crgb[1], crgb[2], crgb[3]);

        uint8_t r = 0;
        uint8_t g = 0;
        uint8_t b = 0;
        if(crgb[0] > 0) {
            r = (float)crgb[1] / crgb[0] * 255.f;
            g = (float)crgb[2] / crgb[0] * 255.f;
            b = (float)crgb[3] / crgb[0] * 255.f;
        }
        //printf("%d %d %d\n", r, g, b);

        uint8_t to_lorris[] = { 0xFF, 0x01, 0x03 + sizeof(crgb), r, g, b };
        uart_write_bytes(UART_NUM_0, to_lorris, sizeof(to_lorris));
        uart_write_bytes(UART_NUM_0, crgb, sizeof(crgb));
    }
}
