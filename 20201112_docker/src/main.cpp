#include "robotka.h"

#define GRIDUI_LAYOUT_DEFINITION
#include "layout.h"

void setup() {
    rkConfig cfg;
    cfg.owner = "FrantaFlinta"; // Ujistěte se, že v aplikace RBController máte nastavené stejné
    cfg.name = "SuperRobotka";

    // Ve výchozím stavu lze WiFi na robotovi nastavit pomocí Android aplikace
    // RBController (verze 1.0 nebo novější) přes Bluetooth.
    // Robot si toto nastavení pamatuje, a znovu ho použije při dalším zapnutí.

    // Můžete WiFi i přímo nastavit zde v kódu, v takovém případě se nastavení
    // nedá z aplikace změnit.

    // Zde v kódu můžete BUĎTO připojit robota na WiFi...
    //cfg.wifi_name = "RukoKraj";
    //cfg.wifi_password = "PlnoRukou";

    // A NEBO vytvořit vlastní WiFi (odkomentovat další dva řádky)
    //cfg.wifi_default_ap = true;
    //cfg.wifi_ap_password = "flusflus";

    cfg.motor_enable_failsafe = true;
    cfg.rbcontroller_app_enable = true;
    rkSetup(cfg);

    // Začátek registrace kódu pro zpracovávání událostí z aplikace
    auto builder = Layout.begin();

    // Zpracování události o změně polohy Joysticku v aplikaci
    builder.Joystick1
        .onPositionChanged([&](Joystick& joy) {
            rkMotorsJoystick(joy.x(), joy.y());
        });

    // Událost o stisknutí tlačítka pro přepnutí červené LED
    builder.redToggle.onPress([](Button&) {
        Layout.red.setOn(!Layout.red.on());
        rkLedRed(Layout.red.on());
    });

    // Událost o stisknutí tlačítka pro přepnutí modré LED
    builder.blueToggle.onPress([](Button&) {
        Layout.blue.setOn(!Layout.blue.on());
        rkLedBlue(Layout.blue.on());
    });

    // Konec a uložení kódu na zpracování událostí
    builder.commit();

    // Pošle text do sériového portu (zkuste zapnout "PlatformIO: Serial monitor"
    // ve VS Code na dolní liště).
    fmt::print("{}'s Robotka '{}' started!\n", cfg.owner, cfg.name);

    for (int i = 0; true; ++i) {
        // Pošle text do terminálu v aplikaci RBController
        rkControllerSendLog(fmt::format("Tick #{}, battery at {}%, {}mv\n",
            i, rkBatteryPercent(), rkBatteryVoltageMv()));
        delay(1000);
    }
}
