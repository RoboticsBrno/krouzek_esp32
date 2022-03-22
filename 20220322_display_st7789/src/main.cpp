#include <driver/uart.h>
#include <driver/gpio.h>
#include <array>
#include <string.h>
#include <algorithm>
#include <functional>

#include "esp_vfs.h"
#include "esp_spiffs.h"
#include "esp_log.h"

extern "C" {
#include "st7789.h"
};


extern "C" void app_main()
{
    TFT_t disp = {};

    spi_master_init(&disp, 19, 18, 5, 16, 23, 4);

    lcdInit(&disp, 135, 240, 52, 40);

    lcdFillScreen(&disp, WHITE);
    lcdDrawFillRect(&disp, 0, 0, 135, 240, GREEN);

    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true
    };

    // Use settings defined above toinitialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is anall-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
      if (ret == ESP_FAIL) {
        ESP_LOGE("main", "Failed to mount or format filesystem");
      } else if (ret == ESP_ERR_NOT_FOUND) {
        ESP_LOGE("main", "Failed to find SPIFFS partition");
      } else {
        ESP_LOGE("main", "Failed to initialize SPIFFS (%s)",esp_err_to_name(ret));
      }
      return;
    }

    FontxFile fx24G[2];
    FontxFile fx32L[2];
    InitFontx(fx24G,"/spiffs/ILGH24XB.FNT",""); // 12x24Dot Gothic
    InitFontx(fx32L,"/spiffs/LATIN32B.FNT",""); // 16x32Dot Latin

    lcdDrawString(&disp, fx24G, 0, 50, (uint8_t*)"Hello world!", BLUE);


    while(1) {
      vTaskDelay(100);
      lcdDrawCircle(&disp, 135/2, 100, 20, RED);
    }
}
