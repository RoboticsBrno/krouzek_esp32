#include <Arduino.h>
#include "robotka.h"

#include "../pathfinding.hpp"
#include "../states.hpp"

static unsigned long gStartAt = 0;

void loopStart() {
    if(gStartAt != 0) {
        if(millis() > gStartAt) {
            switchState(State::MOVE);
        }
    } else if(rkButtonIsPressed(BTN_DOWN)) {
        gStartAt = millis() + 1000;
    }
}
