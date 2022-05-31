#pragma once

#include <mutex>
#include <driver/i2c.h>
#include <esp_log.h>
#include <array>

struct __attribute__((__packed__)) MT6701_RegisterFile {
    uint8_t x25;
    uint8_t x29;
    uint8_t x30;
    uint8_t x31;
    uint8_t x32;
    uint8_t x33;
    uint8_t x34;
    uint8_t x38;
    uint8_t x3E;
    uint8_t x3F;
    uint8_t x40;

    uint16_t abzResolution() const {
        return ((x30 & 0b11) << 8) | x31;
    }

    void setAbzResolution(uint16_t resolution) {
        x30 = (x30 & 0b11111100) | ((resolution >> 8) & 0b11);
        x31 = resolution & 0xFF;
    }

    bool abzMux() const {
        return (x29 & 0b1000000) != 0;
    }

    void setAbzMux(bool uvw) {
        x29 = (x29 & 0b10111111) | (uvw << 6);
    }
};

class MT6701_I2C {
public:
    static constexpr const uint8_t ADDRESS = 0x06;

    static constexpr const uint8_t REGISTER_FILE_ADDRESSES[] = {
        0x25,
        0x29,
        0x30,
        0x31,
        0x32,
        0x33,
        0x34,
        0x38,
        0x3E,
        0x3F,
        0x40,
    };
    static_assert(sizeof(REGISTER_FILE_ADDRESSES) == sizeof(MT6701_RegisterFile));

    MT6701_I2C(i2c_port_t port) : m_port(port) {}

    MT6701_I2C(MT6701_I2C&& o) {
        o.m_mutex.lock();
        this->m_port = o.m_port;
        o.m_port = -1;
        o.m_mutex.unlock();
    }
    ~MT6701_I2C();

    uint8_t readRegister(uint8_t reg_address) {
        uint8_t result = 0;

        auto cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, ADDRESS << 1, I2C_MASTER_ACK);
        i2c_master_write_byte(cmd, reg_address, I2C_MASTER_ACK);
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (ADDRESS << 1) | 1, I2C_MASTER_ACK);
        i2c_master_read_byte(cmd, &result, I2C_MASTER_LAST_NACK);
        i2c_master_stop(cmd);

        esp_err_t err;
        {
            std::lock_guard<std::mutex> l(m_mutex);
            err = i2c_master_cmd_begin(m_port, cmd, pdMS_TO_TICKS(500));
        }
        i2c_cmd_link_delete(cmd);

        if(err != ESP_OK) {
            ESP_LOGE("MT6701_I2C", "failed to read register %d: 0x%x", reg_address, err);
        }
        return result;
    }

    esp_err_t writeRegister(uint8_t reg_address, uint8_t value) {
        auto cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, ADDRESS << 1, I2C_MASTER_ACK);
        i2c_master_write_byte(cmd, reg_address, I2C_MASTER_ACK);
        i2c_master_write_byte(cmd, value, I2C_MASTER_ACK);
        i2c_master_stop(cmd);

        esp_err_t err;
        {
            std::lock_guard<std::mutex> l(m_mutex);
            err = i2c_master_cmd_begin(m_port, cmd, pdMS_TO_TICKS(500));
        }
        i2c_cmd_link_delete(cmd);

        if(err != ESP_OK) {
            ESP_LOGE("MT6701_I2C", "failed to write register %d: 0x%x", reg_address, err);
        }
        return err;
    }

    float readAngle() {
        auto pos1 = readRegister(3);
        auto pos2 = readRegister(4);
        return float((pos2>>2) | (pos1 << 6))/16383.f * 360.f;
    }

    MT6701_RegisterFile readRegisterFile() {
        MT6701_RegisterFile res = {};
        uint8_t *itr = (uint8_t*)&res;
        for(int i = 0; i < sizeof(REGISTER_FILE_ADDRESSES); ++i) {
            *itr = readRegister(REGISTER_FILE_ADDRESSES[i]);
            ++itr;
        }
        return res;
    }

    esp_err_t writeRegisterFile(const MT6701_RegisterFile& file) {
        const uint8_t * itr = (uint8_t*)&file;
        for(int i = 0; i < sizeof(REGISTER_FILE_ADDRESSES); ++i) {
            auto err = writeRegister(REGISTER_FILE_ADDRESSES[i], *itr);
            if(err != ESP_OK) {
                return err;
            }
            ++itr;
        }
        return ESP_OK;
    }

private:
    MT6701_I2C(const MT6701_I2C&) = delete;

    i2c_port_t m_port;
    mutable std::mutex m_mutex;
};
