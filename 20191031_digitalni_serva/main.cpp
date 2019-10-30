#include "RBControl.hpp"

using namespace rb;

void digitalniServo() {
    // Potřeba zavolat první, aby se serva nastavily. Parametry:
    // 1. počet serv v řetězu
    // 2. pin signálu serv
    // 3. který z UARTů na esp32 použít
    // Tady v příkladu jsou výchozí hodnoty pinu a uartu.
    auto& servoBus = Manager::get().initSmartServoBus(1, GPIO_NUM_32, UART_NUM_1);
    // Součástí inicializace je vyčtení aktuální pozice ze všech serv.
    // Pokud se to nepodaří, jsou na sériovou linku vypsány chyby
    // a posOffline (viz níže) vrací NaN.

    // Není potřeba předávat servo bus v proměnné, lze k němu odkudkoliv přistupovat
    // přes Manager::
    auto& servoBus2 = Manager::get().servoBus();
    // servoBus == servoBus2

    // Pošle příkaz na nastavení ID 0 na adresu 254, tj. všem připojeným servům
    servoBus.setId(0, 254);
    // Program na nastavování ID serv z tábora:
    // https://github.com/RoboticsBrno/RB3201-RBControl-testing-software/blob/master/Servos_setup

    // Nastaví SW limity na servu od 100 do 150 stupnu, do jiného rozsahu se
    // servo nepřesune. _deg za číslem potřebuje `using namespace rb;` nahoře!
    servoBus.limit(0, 100_deg, 150_deg);

    // Zapne automatické zastavení serva, pokud narazí na překážku.
    // Na roboruce se to používá na zastavení serva v klepetech.
    servoBus.setAutoStop(0, true);

    // Získání aktuální pozice serva. Odešle příkaz do serva a čeká na odpověď.
    // Trvá obvykle <50ms, v případě problému s komunikací až 300ms.
    const Angle posOnline = servoBus.pos(0);
    if(posOnline.isNaN()) {
        printf("Nepodarilo se zjistit pozici serva 0\n");
    } else {
        printf("%.2f deg\n", posOnline.deg());
    }

    // Získání aktuální pozice serva. Okamžitě vrátí pozici serva, na které si
    // knihovna myslí, že by servo mělo být. Pokud bylo manuálně pohnuto, bude
    // hodnota špatně.
    const Angle posOffline = servoBus.posOffline(0);
    if(posOffline.isNaN()) {
        printf("Nepodarilo se zjistit pozici serva 0\n");
    } else {
        printf("%.2f deg\n", posOffline.deg());
    }

    // Přesunutí serva na pozici.
    // 3. parametr je rychlost R, 4. parametr je zrychlení z 0 na rychlost R,
    // obvykle je nemusíte měnit.
    servoBus.set(0, 120_deg);
    servoBus.set(0, 180_deg, 180.f, 0.0015f);
}
