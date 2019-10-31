#include <Arduino.h>

#include "mcp3008_linesensor.h"

using namespace mcp3008;

// Instance sensoru, je potřeba ji někde držet - nejspíše jako globální proměnnou.
LineSensor gSensor;

void cara() {
    // výchozí hodnoty Config v dokumentaci:
    // https://roboticsbrno.github.io/Esp32-Mcp3008-LineSensor/structmcp3008_1_1Driver_1_1Config.html#aff88a4d4a3ec4addae63b3a085ab696f
    LineSensor::Config cfg;
    cfg.pin_cs = GPIO_NUM_25;
    // Pokud liště nefunguje nějaký ze senzorů, je možné ho vypnout, třeba tady vypnu dva prostřední:
    cfg.channels_mask = 0b11100111; // 0b == číslo ve dvojkové soustavě

    // Je potřeba zavolat metodu install, aby se inicializovala komunikace a nastavily piny
    if(gSensor.install(cfg) != ESP_OK) {
        printf("Chyba pri inicializaci senzoru!\n");
        return;
    }


    /**************** NEKALIBROVANÉ HODNOTY ****************/
    {
        // Vyčtení hodnoty jednoho senzoru. Jsou číslované <0;8), výsledná hodnota
        // je 10bit, tedy <0;1024). Při chybě vrátí 0xFFFF.
        uint16_t hodnota = gSensor.readChannel(2);

        // Vyčtení všech kanálů zároveň. Na konec vectoru přibude tolik elementů,
        // kolik je povolených senzorů. Ve výchozím stavu 8, v tomto příkladu 6, protože
        // jsme dva vypli pomocí cfg.channels_mask
        std::vector<uint16_t> hodnoty;
        gSensor.read(hodnoty);
        hodnoty.clear();  // Před dalším zavoláním je potřeba zavolat .clear protože jinak by se přidávalo na konec vectoru
        gSensor.read(hodnoty);
    }



    /**************** KALIBROVANÉ HODNOTY ****************/
    {
        // viz metoda dole. Pokud se LineSensor nezkalibruje, calibrated* metody se chovají stejně
        // jako ty nezkalibrované.
        kalibrace();

        // Vyčtení hodnoty jednoho senzoru. Jsou číslované <0;8), výsledná hodnota
        // je 10bit, tedy <0;1024). Při chybě vrátí 0xFFFF.
        uint16_t calHodnota = gSensor.calibratedReadChannel(0);

        // Vyčtení všech kanálů zároveň. Na konec vectoru přibude tolik elementů,
        // kolik je povolených senzorů. Ve výchozím stavu 8, v tomto příkladu 6, protože
        // jsme dva vypli pomocí cfg.channels_mask
        std::vector<uint16_t> calHodnoty;
        gSensor.calibratedRead(calHodnoty);
        calHodnoty.clear();  // Před dalším zavoláním je potřeba zavolat .clear protože jinak by se přidávalo na konec vectoru
        gSensor.calibratedRead(calHodnoty);

        // Tato funkce se pokouší najít černou čáru pod senzory.
        // Vrací hodnotu <-1;1>. Pokud je čára pod sensorem úplně vlevo je -1, úplně vpravo je 1 a uprostřed je 0.
        // Pokud čára nebyla nalezena, vrátí NaN.
        float pos = gSensor.readLine();
        if(isnan(pos)) {
            printf("Cara nebyla nalezena!\n");
        }
    }
}


void kalibrace() {
    // Začátek kalibrace
    auto kalibrator = gSensor.startCalibration();

    // record() je třeba zavolat několikrát za vteřinu během toho, co se všechny senzory
    // pohybují nad čárou i podkladem - typicky se robot otočí na místě sem a tam.
    // Implementace celé kalibrace v knihovně pro roboruku:
    // https://github.com/RoboticsBrno/RB3201-RBControl-Roboruka-library/blob/7df6af0217f88fc681446aaaca3a44721c42835a/src/roboruka.cpp#L187-L217

    for(int i = 0; i < 50; ++i) {
        kalibrator.record();
        delay(50);
    }

    // Uložit hodnoty do LineSensoru
    kalibrator.save();
    // LineSensor je teď zkalibrovaný

    // Kalibracni data je mozne nekam ulozit. Takto je dostanete do pole bytu:
    uint8_t uloziste[sizeof(LineSensor::CalibrationData)]; // sem se to ulozi
    LineSensor::CalibrationData kalibracniData = gSensor.getCalibration(); // "dej mi kalibracni data"
    memcpy(uloziste, &kalibracniData, sizeof(kalibracniData)); // kopirovani do uloziste

    // A teď načtení zpět do LineSensoru:
    LineSensor::CalibrationData nactenaData;
    memcpy(&nactenaData, uloziste, sizeof(nactenaData)); // načtení dat z úložiště
    if(!gSensor.setCalibration(nactenaData)) { // Načtění do LineSensoru
        printf("Nacteni kalibracnich dat selhalo!\n");
    }
}
