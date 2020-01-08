
#include <thread>

// Z Arduino frameworku
#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>

// Async web server knihovna
#include <ESPAsyncWebServer.h>

// Zkompilovaná index.html - koukněte na genpage.py
#include <webpage.h>

// Dokumentace http knihovny:
// https://github.com/me-no-dev/ESPAsyncWebServer#request-variables


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

    // U web serveru je třeba nejdříve registrovat handlery
    // pro jednotlivé cesty:

    gServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        // Odešle jako odpověď string s PROGMEM atributtem.
        // To znamená, že string je uložený pouze ve flash paměti a né v RAM,
        // kam se načte až když je potřeba.
        request->send_P(200, "text/html", index_html);
    });

    // http://192.168.42.23/debug
    gServer.on("/debug", HTTP_GET, [](AsyncWebServerRequest *request){
        // "Streamovaná" odpověď, zapisuje se průběžně.
        AsyncResponseStream *response = request->beginResponseStream("text/plain");

        response->printf("Http verze: %d\n", (int)request->version());
        response->printf("Metoda: %d\n", request->method());
        response->printf("url: %s\n", request->url().c_str());
        response->printf("Host: %s\n", request->host().c_str());
        response->printf("contentType: %s\n", request->contentType().c_str());
        response->printf("contentLength: %u\n", request->contentLength());
        response->printf("isMultipart: %d\n", (int)request->multipart());

        // projdeme všechny hlavičky požadavku:
        response->printf("Hlavicky:\n");
        for(size_t i = 0; i < request->headers(); ++i) {
            auto *h = request->getHeader(i);
            response->printf("    %s: %s\n", h->name().c_str(), h->value().c_str());
        }

        // Jedna hlavička podle jména
        if(request->hasHeader("User-Agent")) {
            auto h = request->header("User-Agent");
            response->printf("User-Agent: %s\n", h.c_str());
        }

        response->printf("\nParametry:\n");

        // Projdeme všechny příchozí parametry
        for(size_t i = 0; i < request->params(); ++i){
            auto* p = request->getParam(i);
            if(p->isFile()){ // upload souboru
                response->printf("    FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
            } else if(p->isPost()){ // POST parametr
                response->printf("    POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
            } else { // GET parametr
                response->printf("    GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
            }
        }

        // GET parametr podle jména
        if(request->hasParam("akce")) {
            auto *p = request->getParam("akce");
            response->printf("GET akce %s\n", p->value().c_str());
        }

        // POST podle jména
        if(request->hasParam("akcePost", true)) {
            auto *p = request->getParam("akcePost", true);
            response->printf("POST akce %s\n", p->value().c_str());
        }

        // soubor podle jména
         if(request->hasParam("file", true, true)) {
            auto *p = request->getParam("file", true, true);
            response->printf("POST file jméno %s\n", p->value().c_str());
        }

        // ukončení odpovědi
        request->send(response);
     });


    std::vector<uint8_t> full_file;
    // Nahrávání souboru: Zavolají se oba handlery, první na uploady, potom na request
    gServer.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request){
        // Odpověď je normální string
        request->send(200, "text/plain", "Upload dokoncen!\n");
    },
    [&](AsyncWebServerRequest *request, const String& filename, size_t offset, uint8_t *data, size_t len, bool final) {
        // Tento handler je zavolán několikrát pro každý soubor, vždy s jeho částí.
        if(offset == 0) {
            printf("upload souboru %s\n", filename.c_str());
            full_file.clear();
        }

        full_file.insert(full_file.end(), data, data + len);

        if(final) {
            full_file.push_back(0);
            printf("soubor %s prijat, obsah: %s\n", filename.c_str(), full_file.data());
        }
    });

    // Zapne web server
    gServer.begin();
}

void loop() {
    // DNS server není Async, musí se takto zpracovávat.
    gDnsServer.processNextRequest();

    usleep(1000);
}
