#include "robotka.h"
#include "states.hpp"

static State gState = State::START;

void switchState(State newState) {
    gState = newState;
}

// Funkce setup se zavolá vždy po startu robota.
void setup() {
    rkConfig cfg;
    // Upravte nastavení, například:
    // cfg.motor_max_power_pct = 30; // limit výkonu motorů na 30%
    rkSetup(cfg);

    while(true) {
        switch(gState) {
            case State::START:
                loopStart();
                break;
            case State::MOVE:
                loopMove();
                break;
        }

        vTaskDelay(0);
    }
}
