#include <Arduino.h>
#include <esp_err.h>
#include <nvs_flash.h>
#include <memory>

#include <stdexcept>
#include <vector>

// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/error-handling.html#c-exceptions


// RAII
// Resource Acquisition is initialization

class NvsFlash {
public:
    NvsFlash() {
        esp_err_t err = nvs_flash_init();
        if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            err = nvs_flash_erase();
            if(err != ESP_OK) {
                throw std::runtime_error("failed to erase nvs flash");
            }
            err = nvs_flash_init();
        }

        //err = ESP_ERR_FLASH_BASE;

        if(err != ESP_OK) {
            throw std::runtime_error("failed to init nvs flash");
        }

        m_initialized = true;
    }

    // copy constructor
    NvsFlash(const NvsFlash& other) = delete;

    NvsFlash& operator=(const NvsFlash& other) = delete;

    // move constructor
    NvsFlash(NvsFlash&& other) {
        m_initialized = other.m_initialized;
        other.m_initialized = false;

        m_vector = std::move(other.m_vector);

        std::unique_ptr<int> x(new int(4));

        m_vector.emplace_back(std::move(x));

        printf("neco: %d\n", x.get()); // chybný kód, z x bylo přesunuto pryč
    };

    NvsFlash& operator=(NvsFlash&& other) {
        /*delete m_ptr;
        m_ptr = other.m_ptr;
        other.m_ptr = nullptr;*/
    }

    ~NvsFlash() {
        printf("In destructor\n");
        if(m_initialized) {
            printf("Deinitializing flash\n");
            nvs_flash_deinit();
        }
    }

private:
    bool m_initialized;
    std::vector<std::unique_ptr<int> > m_vector;
};

#include <mutex>
std::mutex gMutex;

static NvsFlash initFlash() {
    NvsFlash flash;
    return flash;
}





class SuperString {
public:
    SuperString(const char* value) : m_value(value) {

    }

    size_t length() const {
        std::lock_guard<std::mutex> l(m_mutex);
        return strlen(m_value);
    }

    void setValue(const char* x) {
        std::lock_guard<std::mutex> l(m_mutex);
        m_value = x;
    }

private:
    const char * m_value;
    const int * x;

    mutable std::mutex m_mutex;

};

void setup() {
    delay(500);
    printf("\n\n");

    //SuperString s = "nececop";
    //SuperString s2("nececop");

    std::unique_ptr<int> i(new int);
    std::lock_guard<std::mutex> l(gMutex);

    try {
        NvsFlash f = initFlash();

        NvsFlash f2;
        f2 = std::move(f);

        printf("End of try block\n");
    } catch(const std::exception& e) {
        printf("Caught exception: %s\n", e.what());
    }
}

void loop() {

}
