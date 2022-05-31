#include <driver/uart.h>
#include <driver/gpio.h>
#include <array>
#include <string.h>
#include <algorithm>
#include <functional>
#include <atomic>
#include <freertos/queue.h>
#include <driver/i2c.h>

#include "mt6701.h"

#include "gridui.h"
#include "rbprotocol.h"
#include "rbwebserver.h"
#include "rbwifi.h"

#define GRIDUI_LAYOUT_DEFINITION
#include "layout.hpp"
using namespace gridui;
using namespace rb;

static MT6701_RegisterFile gRegisters;
static TickType_t gWriteEepromConfirmStart = 0;

static MT6701_I2C setupEnc() {
    const i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = 14,
        .scl_io_num = 13,
        .sda_pullup_en = true,
        .scl_pullup_en = true,
        .master = {
            .clk_speed = 500000,
        },
        .clk_flags = 0,
    };

    ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_0, &i2c_conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 64, 0, 0));

    gpio_set_direction(GPIO_NUM_12, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_12, 1);
    return MT6701_I2C(I2C_NUM_0);
}

static void resetRegistersInUi() {
    Layout.abzResolution.setText(std::to_string(gRegisters.abzResolution()));
    Layout.outputAbz.setChecked(!gRegisters.abzMux());
    Layout.outputUvw.setChecked(gRegisters.abzMux());
}

static void applyRegistersFromUi(MT6701_I2C& enc) {
    gRegisters.setAbzResolution(atoi(Layout.abzResolution.text().c_str()));
    gRegisters.setAbzMux(Layout.outputUvw.checked());

    auto err = enc.writeRegisterFile(gRegisters);
    if(err != ESP_OK) {
        UI.protocol()->send_log("Failed to write register file: 0x%x\n", err);
    } else {
        UI.protocol()->send_log("Registers written to MT6701\n");
    }

    gRegisters = enc.readRegisterFile();
    resetRegistersInUi();
}

static void setupUi(MT6701_I2C& enc) {
    auto prot = new Protocol("FrantaFlinta", "MT6701 Workbench", "Compiled at " __DATE__ " " __TIME__,
        [](const std::string& cmd, rbjson::Object* pkt) {
            UI.handleRbPacket(cmd, pkt);
        });
    prot->start();

    rb_web_start(80);
    UI.begin(prot);

    auto builder = Layout.begin();

    builder.abzResolution
        .onChanged([=](Input &in) {
            auto valStr = in.text();
            const char *end = valStr.c_str();

            auto val = strtol(valStr.c_str(), (char**)&end, 10);
            if(valStr.size() == 0 || *end != 0 || val < 0 || val > 1023) {
                UI.protocol()->send_log("Invalid ABZ resolution value, must be number between <0; 1023>\n");
                in.setText(std::to_string(gRegisters.abzResolution()));
            }
        });

    builder.outputAbz
        .onChanged([](Checkbox &b) { Layout.outputUvw.setChecked(!b.checked()); });
    builder.outputUvw
        .onChanged([](Checkbox &b) { Layout.outputAbz.setChecked(!b.checked()); });

    builder.btnReset.onPress([](Button&) {
        resetRegistersInUi();
    });

    builder.btnSave.onPress([&](Button&) {
        applyRegistersFromUi(enc);
    });

    builder.btnWriteEeprom.onPress([](Button& b) {
        if(gWriteEepromConfirmStart == 0) {
            b.setText("Tap again to confirm");
            gWriteEepromConfirmStart = xTaskGetTickCount();
        } else {
            b.setText("Write to EEPROM");
            gWriteEepromConfirmStart = 0;
            // TODO: write
        }
    });

    builder.commit();

    resetRegistersInUi();
}

extern "C" void app_main()
{
    auto enc = setupEnc();
    gRegisters = enc.readRegisterFile();

    WiFi::connect("Anthrophobia", "Ku1ata2elvA");
    setupUi(enc);


    while(true) {
        const auto now = xTaskGetTickCount();
        if(gWriteEepromConfirmStart != 0 && now - gWriteEepromConfirmStart > pdMS_TO_TICKS(3000)) {
            Layout.btnWriteEeprom.setText("Write to EEPROM");
            gWriteEepromConfirmStart = 0;
        }

        Layout.angle.setValue(enc.readAngle());
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
