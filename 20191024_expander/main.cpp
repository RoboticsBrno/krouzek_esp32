#include "RBControl.hpp"

using namespace rb;

void expander() {
    auto& expander = Manager::get().expander();

    // Přečtení stavu pinu, vrací 0 nebo 1
    if(expander.digitalRead(SW1) == 0) { // nebo EB0, EA2...
        printf("Tlacitko stisknuto!\n");
    }

    // Nastavení směru pinu
    expander.pinMode(EA0, GPIO_MODE_OUTPUT); // nebo GPIO_MODE_INPUT
    // Zapsání výstupní hodnoty pinu
    expander.digitalWrite(EA0, 1);

    // Nastavení EA1 jako input s pullupem
    expander.pinMode(EA1, GPIO_MODE_INPUT);
    expander.pullUp(EA1, 1);
 }
