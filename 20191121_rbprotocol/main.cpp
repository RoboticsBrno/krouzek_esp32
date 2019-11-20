

#include <memory>

#include "rbprotocol.h"
#include "rbwebserver.h"

#include "RBControl.hpp"

static rb::Protocol *gProt = nullptr;

static void jakobySetup() {
    rb::Manager::get().install();

    // Spustí web server na portu 80, který vrací soubory uložené
    // ve SPIFFS paměti na ESP32. Dostanou se tam tak, že je dáte do složky
    // "data" v kořenu projektu (vedle složky src, ve které teď právě jsme)
    // a pak zavoláte 'pio run -t uploadfs'. Šablona pro Roboruku tohleto všechno dělá za vás.
    // Pro příklad správně nastaveného projektu koukněte na rbcontroller pro Roboruku, zvlášte soubory:
    // * /data/
    // * /post_extra_script.py
    // * /platformio.ini ve kterém se volá  post_extra_script.py
    // https://github.com/RoboticsBrno/roboruka-examples/tree/master/rbcontroller-android-app
    rb_web_start(80);

    // Vytvoření & zapnutí RBProtocolu
    gProt = new rb::Protocol("FrantaFlinta", "Roboruka", "Nej roboruka", onPacketReceived);
    gProt->start();

    for(;;) {
        printf("Telefon připojen: %d\n", int(gProt->is_possessed()));
        sleep(3);
    }
}

static void onPacketReceived(const std::string& command, rbjson::Object *pkt) {
    // Tato metoda se zavolá, když přijde nějaký příkaz. Příkazy "discover" a "possess"
    // zpracuje knihovna a sem se už nedostanou.

    if(command == "joy") {
        // Součásti RBProtocol knihovny je parsování & vytváření JSONů.
        // Tady si vybereme z příchozího packetu pole.
        rbjson::Array *data = pkt->getArray("data");
        for(size_t i = 0; i < data->size(); ++i) {
            // když pracujeme s JSONem, musíme vědět, jaké datové typy nám přijdou.
            printf("joy data[%d] == %d\n", i, data->getInt(i));
        }
    } else if(command == "getinfo") {
        // Příklad konstrukce nového JSON objektu pro odpověď.
        // když používáme send() (viz níže), jsme zodpovědní za uvolnění
        // paměti po Objectu. Zde tedy využijeme alokace na stacku, aby se uvolnil sám.
        rbjson::Object odpoved;
        odpoved.set("name", "FrantaFlinta");
        odpoved.set("age", 14.2);
        odpoved.set("cool", true);

        rbjson::Object *podObjekt = new rbjson::Object();
        podObjekt->set("usi", 2);
        podObjekt->set("nos", "jeden");

        // Jakmile nastavíme objekt do jiného pomocí set, stává se ten druhý jeho
        // "rodičem" a zodpovídá za správu paměti. Tady tedy musíme alokovat na heapu,
        // a Objject "odpoved" sám smaže podObjekt při své destrukci.
        odpoved.set("hlava", podObjekt);

        // Odeslání odpovědi, bez jistoty, že dojde.
        gProt->send("info", &odpoved);
     } else if(command == "getinfo2") {
        // Příklad konstrukce nového JSON objektu pro odpověď, která musí přijít.
        // když používáme send_mustarrive() (viz níže), nejsme zodpovědní za
        // správu paměti datového Objectu.
        // Zde použijeme std::unique_ptr, který zaručuje, že paměť po Objectu bude
        // uvolněna, dokud nezavoláme dole .release();. V komentáři na dalším řádu a u returnu
        // uvádím, jak by to vypadalo bez použití unique_ptr.
        std::unique_ptr<rbjson::Object> odpoved(new rbjson::Object()); // rbjson::Object *odpoved = new rbjson::Object();
        odpoved->set("name", "FrantaFlinta");
        odpoved->set("age", 14.2);
        odpoved->set("cool", true);

        if(rb::Manager::get().battery().pct() < 50) {
            // delete odpoved; -- není potřeba díky std::unique_ptr
            return;
        }

        std::unique_ptr<rbjson::Object> podObjekt(new rbjson::Object());
        podObjekt->set("usi", 2);
        podObjekt->set("nos", "jeden");

        // Po .set() opět přestáváme být zodpovědní za paměť podObjektu,
        // musíme tedy na unique_ptr zavolat .release().
        odpoved->set("hlava", podObjekt.release());

        // Odeslání odpovědi za použití mechanismu potvrzeného doručení.
        gProt->send_mustarrive("info", odpoved.release());
    }
}
