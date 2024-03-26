#pragma once

#include <driver/uart.h>

enum DebugCommand : uint8_t {
    MAIN_STATE = 0x00,
    SET_STATE = 0x01,
    TEST_PATHFINDING = 0x02,
    PATHFINDING_RESULT = 0x03,
};

void debugSendHeader(uint8_t command, uint8_t length);
void debugPollUartRx();
