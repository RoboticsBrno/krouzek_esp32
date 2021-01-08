#include <Arduino.h>
#include <esp_err.h>
#include <nvs_flash.h>
#include <mutex>

#include <stdexcept>

// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/error-handling.html#c-exceptions


std::mutex gMutex;

class MyException : public std::exception {
public:
    const char *what() const noexcept {
        return "Moje chyba!";
    }
};

class EspException : public std::runtime_error {
public:
    EspException(esp_err_t err, const std::string& message = "") : std::runtime_error(getMessage(err, message)) {
        m_error = err;
    }

    esp_err_t error() const {
        return m_error;
    }

private:
    static std::string getMessage(esp_err_t err, const std::string& message) {
        char buf[128];
        snprintf(buf, sizeof(buf), "%s (%d)", esp_err_to_name(err), err);
        if(message.empty()) {
            return buf;
        } else {
            return std::string(buf) + ": " + message;
        }
    }

    esp_err_t m_error;
};


static void setupNvsFlash() {
    std::lock_guard<std::mutex> l(gMutex);

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        err = nvs_flash_erase();
        if(err != ESP_OK) {
            throw std::runtime_error("failed to erase nvs flash");
        }

        err = nvs_flash_init();
    }

    err = ESP_ERR_FLASH_BASE;

    if(err != ESP_OK) {
        throw EspException(err, "failed to init nvs flash");
    }
}


static void setupEverything() {
    try {
        setupNvsFlash();
    } catch(const std::exception& exception) {
        printf("Caught exception: %s\n", exception.what());
        throw std::runtime_error("sadsad");
    } catch(...) {
        // catch anything
    }

    printf("Setup everything done.\n");
}

void setup() {
    delay(500);
    printf("\n\n");

    try {
        setupEverything();
    } catch(const EspException& e) {
        printf("Caught exception: %s\n", e.what());
    } catch(int x) {
        printf("caught int %d\n", x);
    }
}

void loop() {

}
