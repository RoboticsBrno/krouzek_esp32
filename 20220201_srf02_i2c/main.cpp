#include <driver/i2c.h>
#include <driver/gpio.h>

#include "srf02.hpp"

extern "C" void app_main() {
    auto initRes = SRF02::withBusInit(I2C_NUM_0, 0xF0, GPIO_NUM_33, GPIO_NUM_32);
    ESP_ERROR_CHECK(std::get<1>(initRes));
    auto srf02 = std::move(std::get<0>(initRes));

    while(true) {
        ESP_ERROR_CHECK(srf02.triggerRangingCm());
        vTaskDelay(pdMS_TO_TICKS(70));

        uint16_t rangeCm;
        esp_err_t err;
        std::tie(rangeCm, err) = srf02.readRange();
        ESP_ERROR_CHECK(err);

        printf("dist: %d\n", (int)rangeCm);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

static void rawI2CCommands()
{

  i2c_config_t conf = {
      .mode = I2C_MODE_MASTER,
      .sda_io_num = GPIO_NUM_33,
      .sda_pullup_en = GPIO_PULLUP_ENABLE,
      .scl_io_num = GPIO_NUM_32,
      .scl_pullup_en = GPIO_PULLUP_ENABLE,
      .master = {
        .clk_speed = 1000000
      },
  };

  ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_0, &conf));
  ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0));

  const int addr = 0xF0;
  while(true) {

    uint8_t buf[4] = {};
    auto cmd = i2c_cmd_link_create();
    ESP_ERROR_CHECK(i2c_master_start(cmd));
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (addr << 1), true));
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, 0x00, true));
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, 0x51, true));
    ESP_ERROR_CHECK(i2c_master_stop(cmd));

    ESP_ERROR_CHECK(i2c_master_cmd_begin(I2C_NUM_0, cmd, 50));
    i2c_cmd_link_delete(cmd);


    vTaskDelay(10);


    cmd = i2c_cmd_link_create();
    ESP_ERROR_CHECK(i2c_master_start(cmd));
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (addr << 1), true));
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, 0x02, true));
    ESP_ERROR_CHECK(i2c_master_stop(cmd));

    ESP_ERROR_CHECK(i2c_master_cmd_begin(I2C_NUM_0, cmd, 50));
    i2c_cmd_link_delete(cmd);

    cmd = i2c_cmd_link_create();
    ESP_ERROR_CHECK(i2c_master_start(cmd));
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (addr << 1) | 1, true));
    ESP_ERROR_CHECK(i2c_master_read_byte(cmd, &buf[0], I2C_MASTER_ACK));
    ESP_ERROR_CHECK(i2c_master_read_byte(cmd, &buf[1], I2C_MASTER_LAST_NACK));
    ESP_ERROR_CHECK(i2c_master_stop(cmd));

    ESP_ERROR_CHECK(i2c_master_cmd_begin(I2C_NUM_0, cmd, 50));
    i2c_cmd_link_delete(cmd);

    uint16_t val = (uint16_t(buf[0]) << 8) | buf[1];
    printf("dist: %d\n", (int)val);

  }
}
