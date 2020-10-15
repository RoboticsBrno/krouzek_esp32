#include <Arduino.h>
#include <WiFi.h>

#include <thread>
#include <mutex>
#include <math.h>

#include "packet.h"

#define LISTEN_PORT 4090

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

static void dataThread();
static void processPacket(const Packet &pkt);

static WiFiServer wifiServer(0, 4); // 4 == max pripojenych clientu zaraz

struct ClientInfo
{
    WiFiClient client;
    PacketParser parser;
};

static std::mutex clientsMutex;
static std::vector<ClientInfo> clients;

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

    Serial.printf("WiFi connected\nConnect to %s:%d\n", WiFi.localIP().toString().c_str(), LISTEN_PORT);

    wifiServer.begin(LISTEN_PORT);

    std::thread(dataThread)
        .detach();
}

void loop()
{
    {
        std::lock_guard<std::mutex> l(clientsMutex);

        while (wifiServer.hasClient())
        {
            auto client = wifiServer.available();
            if (client)
            {
                Serial.printf("Got a client from %s:%d\n", client.remoteIP().toString().c_str(), client.remotePort());
                clients.emplace_back(ClientInfo{
                    .client = client,
                });
            }
        }

        for (auto itr = clients.begin(); itr != clients.end();)
        {
            ClientInfo &ci = *itr;
            if (!ci.client.connected())
            {
                Serial.printf("Client %s:%d disconnected.\n", ci.client.remoteIP().toString().c_str(), ci.client.remotePort());
                itr = clients.erase(itr);
                continue;
            }

            while (ci.client.available() > 0)
            {
                if (ci.parser.addByte(ci.client.read()))
                {
                    processPacket(ci.parser.packet());
                }
            }

            ++itr;
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

static void writeToAllClients(const uint8_t *data, size_t size)
{
    std::lock_guard<std::mutex> l(clientsMutex);
    for (auto &ci : clients)
    {
        ci.client.write(data, size);
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
        writeToAllClients(pkt.raw().data(), pkt.raw().size());

        pkt.reset(CMD_COS);
        pkt.write(cos(value));
        writeToAllClients(pkt.raw().data(), pkt.raw().size());

        pkt.reset(CMD_TAN);
        pkt.write(tan(value));
        writeToAllClients(pkt.raw().data(), pkt.raw().size());

        value += params.step;
        delay(params.tick_rate_ms);
    }
}
