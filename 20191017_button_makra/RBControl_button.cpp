#include "RBControl_button.hpp"


namespace rb {


Button::Button(Adafruit_MCP23017& expander, uint8_t pin) :
    m_expander(expander)
{
    m_pin = pin;
}

bool Button::isPressed() const {
    return m_expander.digitalRead(m_pin) == 0;
}

}; // namespace rb
