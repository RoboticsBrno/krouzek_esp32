#include <Arduino.h>

#include "gridui.h"
#include "rbprotocol.h"
#include "rbwebserver.h"
#include "rbwifi.h"

using namespace rb;
using namespace gridui;

// You can include layout.hpp in many .cpp files,
// but ONE of those must have this define before it.
#define GRIDUI_LAYOUT_DEFINITION
#include "layout.hpp"

#include "SmartLeds.h"

static Protocol* gProt = nullptr;

static void onPacketReceived(const std::string& cmd, rbjson::Object* pkt) {
    // Let GridUI handle its packets
    if (UI.handleRbPacket(cmd, pkt))
        return;

    // ...any other non-GridUI packets
}

static SmartLed gRgbLed(LED_WS2812, 1, 48);

void setup() {
    gRgbLed[0] = Rgb(255, 0, 0);
    gRgbLed.show();

    WiFi::connect("Anthrophobia", "Ku1ata2elvA");

    // Initialize RBProtocol
    gProt = new Protocol("FrantaFlinta", "Robocop", "Compiled at " __DATE__ " " __TIME__, onPacketReceived);
    gProt->start();

    // Start serving the web page
    rb_web_start(80);

    // Initialize the UI builder
    UI.begin(gProt);

    // Build the UI widgets. Positions/props are set in the layout, so most of the time,
    // you should only set the event handlers here.
    auto builder = Layout.begin();

    builder.sliderR.onChanged([](Slider& s) {
        gRgbLed[0].r = s.value();
        gRgbLed.wait();
        gRgbLed.show();
    });

    builder.sliderG.onChanged([](Slider& s) {
        gRgbLed[0].g = s.value();
        gRgbLed.wait();
        gRgbLed.show();
    });

    builder.sliderB.onChanged([](Slider& s) {
        gRgbLed[0].b = s.value();
        gRgbLed.wait();
        gRgbLed.show();
    });

    // Commit the layout. Beyond this point, calling any builder methods on the UI is invalid.
    builder.commit();
}

void loop() {
    printf("IP: %x\n", WiFi::getIp());
    delay(1000);
}
