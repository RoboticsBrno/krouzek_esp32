#include <driver/uart.h>
#include <driver/gpio.h>
#include <array>
#include <string.h>
#include <algorithm>
#include <functional>

extern "C" {
#include "ssd1306.h"
};

extern "C" void app_main()
{

    SSD1306_t disp = {};

    i2c_master_init(&disp, CONFIG_SDA_GPIO, CONFIG_SCL_GPIO, 0);
    ssd1306_init(&disp, 128, 64);

    ssd1306_clear_screen(&disp, false);
    ssd1306_display_text(&disp, 0, "Ahoj!", 5, false);
    ssd1306_display_text(&disp, 1, "Ahoj!", 5, false);
    ssd1306_display_text(&disp, 5, "Ahoj!", 5, true);

    uint8_t image[8] = {
        (1 << 0),
        (1 << 1),
        (1 << 2),
        (1 << 3),
        (1 << 4),
        (1 << 5),
        (1 << 6),
        (1 << 7),
      };

    int row = 0, col = 0;
    while(row < 8) {
      ssd1306_display_image(&disp, row, col, image, sizeof(image));
      ++row;
      col += 8;
    }

    ssd1306_hardware_scroll(&disp, SCROLL_DOWN);

    while(1) {
      vTaskDelay(100);
    }
}
