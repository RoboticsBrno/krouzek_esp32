#include <Arduino.h>                //include basic Arduino library
#include <WiFi.h>                   //basic library for control WiFi on ESP32
#include <AsyncTCP.h>               //must have library for Asynchronous Web Server maked by: https://github.com/me-no-dev
#include <ESPAsyncWebServer.h>      //library for Asynchronous Web Server on ESP32 maked by: https://github.com/me-no-dev
#include <webpage.h>                //header file with web page, which will ESP send to user device
#include <DNSServer.h>


static const char *SSID = "Esp32WiFi";
static const char *PASSWORD = "flusflus";

static AsyncWebServer gServer(80);
static DNSServer gDnsServer;

void setup() {
    // Zapne WiFi v režimu access point
    WiFi.softAP(SSID, PASSWORD);

    // Nastaví DNS server tak, aby jakákoliv adresa resolvovala
    // na tohle ESP
    gDnsServer.start(53, "*", WiFi.softAPIP());

    gServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", index_html);
    });

     gServer.on("/debug", HTTP_GET, [](AsyncWebServerRequest *request){
        printf("Http verze: %d\n", (int)request->version());
        printf("Metoda: %s\n", request->method());
        printf("url: %s\n", request->url().c_str());
        printf("Host: %s\n", request->host().c_str());
        printf("contentType: %s\n", request->contentType().c_str());
        printf("contentLength: %u\n", request->contentLength());
        printf("isMultipart: %d\n", (int)request->multipart());

        // projdeme všechny hlavičky požadavku:
        printf("Hlavicky:\n");
        for(size_t i = 0; i < request->headers(); ++i) {
            auto *h = request->getHeader(i);
            printf("    %s: %s\n", h->name().c_str(), h->value().c_str());
        }

        // Jedna hlavička podle jména
        if(request->hasHeader("User-Agent")) {
            auto h = request->header("User-Agent");
            printf("User-Agent: %s", h.c_str());
        }
     });

    // Zapne web server
    gServer.begin();
}

void loop() {
    gDnsServer.processNextRequest();
}
