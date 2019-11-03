#include "RBControl.hpp"

using namespace rb;

void enkodery() {
    auto& man = Manager::get();

    // Enkodéry ve výchozím stavu nejsou inicializované.
    // Jakmile se na daný motor poprvé zavolá metoda encoder(),
    // drive() nebo driveToValue(), enkodér se inicializuje.
    // encoder() vrací vždy ten stejné Encoder.
    //
    // Piny enkodérů jsou vázané na motory, tedy když mám motor M1,
    // musím používat piny ENC1A a ENC1B.
    Encoder *enc = man.motor(MotorId::M1).encoder();


    // Ujede 1000 tiků enkodéru dopředu 100% rychlostí, potom vypíše
    // na seriovou linku text.
    // Kolik robot doopravdy ujede záleží na převodovce
    // a velikosti kol.
    enc->drive(1000, 100, [](Encoder& enc) {
        printf("Jsem tu, na hodnote %d!\n", enc.value());
    });
    // Následující zápis udělá to stejné, je jen pro příklad že to jde i jinak:
    man.motor(MotorId::M1).drive(1000, 100, jsemTu);


    // driveToValue, narozdíl od drive, operuje v abosolutních hodnotách. Místo
    // "ujeď o X ticků" znamená "ujeď na vzálenost X od začátku".
    // V tomto případě tedy pojedeme 600 ticků dozadu, protože v předchozím drive()
    // jsme jeli 2x1000 dopředu.
    enc->driveToValue(1400, 50, [](Encoder& enc) {
        printf("Jsem o neco zpet, na hodnote %d!\n", enc.value());
    });


    // Aktualni odometrie
    printf("Aktualni ujeta zvdalenost je %d a rychlost je %f t/s!\n",
        enc->value(), enc->speed());
}

static void jsemTu(Encoder& enc ) {
    printf("Jsem tu, na hodnote %d!\n", enc.value());
}
