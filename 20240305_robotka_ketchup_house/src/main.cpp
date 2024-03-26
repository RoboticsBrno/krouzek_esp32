#include "robotka.h"
#include "states.hpp"
#include "timer.hpp"
#include "debug.hpp"

static State gState = State::START;
static MillisTimer gDebugStateTimer(500);

void switchState(State newState) {
    gState = newState;
}

// Funkce setup se zavolá vždy po startu robota.
void setup() {
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
      ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0, 256, 0, 0, NULL, 0));
    }

    rkConfig cfg;
    // Upravte nastavení, například:
    // cfg.motor_max_power_pct = 30; // limit výkonu motorů na 30%
    rkSetup(cfg);

    gDebugStateTimer.start();

    while(true) {
        if(gDebugStateTimer.expired()) {
            debugSendHeader(DebugCommand::MAIN_STATE, sizeof(gState));
            uart_write_bytes(UART_NUM_0, (char*)&gState, sizeof(gState));
            gDebugStateTimer.start();
        }

        switch(gState) {
            case State::START:
                loopStart();
                break;
            case State::MOVE:
                loopMove();
                break;
        }

        debugPollUartRx();
        vTaskDelay(0);
    }
}
