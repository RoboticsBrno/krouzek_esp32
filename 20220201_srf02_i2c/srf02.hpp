#pragma once

#include <driver/i2c.h>
#include <tuple>
#include <freertos/FreeRTOS.h>


class SRF02 {
public:
    static SRF02 withoutBusInit(i2c_port_t bus_num, uint8_t address);
    static std::tuple<SRF02, esp_err_t> withBusInit(i2c_port_t bus_num, uint8_t address,
        gpio_num_t sda_pin, gpio_num_t scl_pin, uint32_t speed_hz = 500000);

    SRF02(SRF02&&) = default; // enable default move-constructor
    ~SRF02();

    esp_err_t triggerRangingCm(TickType_t i2c_tx_timeout = pdTICKS_TO_MS(10));
    std::tuple<uint16_t, esp_err_t> readRange(TickType_t i2c_tx_timeout = pdTICKS_TO_MS(10));

private:
    SRF02(i2c_port_t bus_num, uint8_t address);
    SRF02(const SRF02&) = delete; // disable copy-constructor

    i2c_port_t m_bus_num;
    uint8_t m_address;
};
