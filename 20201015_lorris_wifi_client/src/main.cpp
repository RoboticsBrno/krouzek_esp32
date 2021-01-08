#include <Arduino.h>
#include <WiFi.h>

#include <thread>
#include <mutex>
#include <math.h>
#include <atomic>

#include "packet.h"

#define DEST_ADDRESS "192.168.0.126"
#define DEST_PORT 4090

enum
{
    CMD_SET_PARAMS = 0,
    CMD_SIN = 1,
    CMD_COS = 2,
    CMD_TAN = 3,
};

struct data_params
{
    uint32_t tick_rate_ms;
    double step;
};

static std::mutex gParamsMutex;
static data_params gParams = {
    .tick_rate_ms = 100,
    .step = 0.1,
};

static PacketParser gParser;

static void dataThread();
static void processPacket(const Packet &pkt);

static WiFiClient wifiClient;

// Vyberte jeden
//static Stream &IO = Serial;
static Stream &IO = wifiClient;

void setup()
{
    Serial.begin(115200);

    Serial.print("Attempting to connect Wifi\n");
    int status = WiFi.begin("Anthrophobia", "Ku1ata2elvA");
    while (status != WL_CONNECTED)
    {
        delay(500);
        status = WiFi.status();
    }
    Serial.print("WiFi connected.\n");

    while (!wifiClient.connect(DEST_ADDRESS, DEST_PORT))
    {
        Serial.printf("Failed to connect to %s:%d, retrying.\n", DEST_ADDRESS, DEST_PORT);
        delay(1000);
    }

    std::thread(dataThread)
        .detach();
}

void loop()
{
    while (Serial.available() > 0)
    {
        if (gParser.addByte(IO.read()))
        {
            processPacket(gParser.packet());
        }
    }
    delay(10);
}

static void processPacket(const Packet &pkt)
{
    switch (pkt.command())
    {
    case CMD_SET_PARAMS:
    {
        gParamsMutex.lock();
        gParams.tick_rate_ms = pkt.read<uint32_t>();
        gParams.step = pkt.read<double>();
        gParamsMutex.unlock();
        break;
    }
    }
}

static void dataThread()
{
    double value = 0;
    Packet pkt;
    while (true)
    {
        gParamsMutex.lock();
        const data_params params = gParams;
        gParamsMutex.unlock();

        pkt.reset(CMD_SIN);
        pkt.write(sin(value));
        IO.write(pkt.raw().data(), pkt.raw().size());

        pkt.reset(CMD_COS);
        pkt.write(cos(value));
        IO.write(pkt.raw().data(), pkt.raw().size());

        pkt.reset(CMD_TAN);
        pkt.write(tan(value));
        IO.write(pkt.raw().data(), pkt.raw().size());

        value += params.step;
        delay(params.tick_rate_ms);
    }
}
