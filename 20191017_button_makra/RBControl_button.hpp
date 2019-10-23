#pragma once

#include "Adafruit_MCP23017.h"

namespace rb {

class Button {
public:
    Button(Adafruit_MCP23017& expander, uint8_t pin);

    bool isPressed() const;
    void waitForPress() const; // camelCase

    void wait_for_release(); // snake_case

private:
    uint8_t m_pin;
    Adafruit_MCP23017& m_expander;
};

};
