#include <utility>

#include "srf02.hpp"

#define RETURN_IF_ERR(x) do {                                         \
        esp_err_t __err_rc = (x);                                       \
        if (__err_rc != ESP_OK) {                                       \
            return __err_rc;                                            \
        }                                                               \
    } while(0)


class I2cCmdHolder {
public:
    I2cCmdHolder() {
        m_cmd = i2c_cmd_link_create();
    }

    ~I2cCmdHolder() {
        i2c_cmd_link_delete(m_cmd);
    }

    i2c_cmd_handle_t get() const { return m_cmd; }

private:
    i2c_cmd_handle_t m_cmd;
};


SRF02 SRF02::withoutBusInit(i2c_port_t bus_num, uint8_t address) {
    return SRF02(bus_num, address);
}

std::tuple<SRF02, esp_err_t> SRF02::withBusInit(i2c_port_t bus_num, uint8_t address,
        gpio_num_t sda_pin, gpio_num_t scl_pin, uint32_t speed_hz) {

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = sda_pin,
        .scl_io_num = scl_pin,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master = {
            .clk_speed = speed_hz
        },
        .clk_flags = 0,
    };

    auto instance = SRF02::withoutBusInit(bus_num, address);

    esp_err_t err = i2c_param_config(bus_num, &conf);
    if(err != ESP_OK) {
        return std::make_tuple(std::move(instance), err);
    }

    err = i2c_driver_install(bus_num, I2C_MODE_MASTER, 0, 0, 0);
    if(err != ESP_OK) {
        return std::make_tuple(std::move(instance), err);
    }

    return std::make_tuple(std::move(instance), ESP_OK);
}

SRF02::SRF02(i2c_port_t bus_num, uint8_t address) : m_bus_num(bus_num), m_address(address) {
    
}

SRF02::~SRF02() {

}

esp_err_t SRF02::triggerRangingCm(TickType_t i2c_tx_timeout) {
    I2cCmdHolder holder;
    auto cmd = holder.get();

    RETURN_IF_ERR(i2c_master_start(cmd));
    RETURN_IF_ERR(i2c_master_write_byte(cmd, m_address << 1, true));
    RETURN_IF_ERR(i2c_master_write_byte(cmd, 0x00, true)); // register address
    RETURN_IF_ERR(i2c_master_write_byte(cmd, 0x51, true)); // command id
    RETURN_IF_ERR(i2c_master_stop(cmd));
    RETURN_IF_ERR(i2c_master_cmd_begin(m_bus_num, cmd, i2c_tx_timeout));

    return ESP_OK;
}

std::tuple<uint16_t, esp_err_t> SRF02::readRange(TickType_t i2c_tx_timeout) {

    esp_err_t err;

    // Set address for reading
    {
        I2cCmdHolder holder;
        auto cmd = holder.get();

        err = i2c_master_start(cmd);
        if(err != ESP_OK) return std::make_tuple(0, err);

        err = i2c_master_write_byte(cmd, m_address << 1 | 0, true);
        if(err != ESP_OK) return std::make_tuple(0, err);
        
        err = i2c_master_write_byte(cmd, 0x02, true); // register address
        if(err != ESP_OK) return std::make_tuple(0, err);

        err = i2c_master_stop(cmd);
        if(err != ESP_OK) return std::make_tuple(0, err);
        
        err = i2c_master_cmd_begin(m_bus_num, cmd, i2c_tx_timeout);
        if(err != ESP_OK) return std::make_tuple(0, err);
    }

    uint8_t range_regs[2] = { };

    {
        I2cCmdHolder holder;
        auto cmd = holder.get();
        err = i2c_master_start(cmd);
        if(err != ESP_OK) return std::make_tuple(0, err);

        err = i2c_master_write_byte(cmd, m_address << 1 | 1, true);
        if(err != ESP_OK) return std::make_tuple(0, err);

        err = i2c_master_read(cmd, range_regs, 2, I2C_MASTER_LAST_NACK);
        if(err != ESP_OK) return std::make_tuple(0, err);

        err = i2c_master_stop(cmd);
        if(err != ESP_OK) return std::make_tuple(0, err);

        err = i2c_master_cmd_begin(m_bus_num, cmd, i2c_tx_timeout);
        if(err != ESP_OK) return std::make_tuple(0, err);
    }

    return std::make_tuple((range_regs[0] << 8) | range_regs[1], ESP_OK);
}
