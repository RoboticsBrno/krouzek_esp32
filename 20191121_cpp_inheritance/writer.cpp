#include <driver/uart.h>
#include <driver/i2c.h>
#include <cstdarg>
#include <algorithm>

#include <RBControl.hpp>

// Základní třída. Vypadá normálně, až na...
class Writer {
public:
    Writer() { }
    virtual ~Writer() { }

    // ...virtual metodu, která má navíc za sebou = 0!
    // virtual si vysvětlíme níže, = 0 znamená, že funkce nemá žádnou
    // implementaci. Writer je tedy neúplná třída, a nelze vytvořit její instanci,
    // lze ji pouze použít jako rodiče dalších tříd.
    virtual esp_err_t write(const char *data, size_t size) = 0;

    esp_err_t writeByte(uint8_t byte) {
        return write((char*)&byte, 1);
    }

    esp_err_t printf(const char *fmt, ...) {
        char buffer[256];
        va_list args;
        va_start(args, fmt);
        size_t len = vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);
        return write(buffer, std::min((size_t)sizeof(buffer), len));
    }
};

// Toto je podtřída Writeru. Implementuje virtuální metodu write
// a "přenechává" si ostatní metody z Writeru
class UartWriter : public Writer {
public:
    UartWriter(uart_port_t port) : Writer() {
        m_port = port;
    }

    virtual esp_err_t write(const char *data, size_t size) {
        return uart_write_bytes(m_port, data, size);
    }

private:
    uart_port_t m_port;
};

class FileWriter : public Writer {
public:
    FileWriter(FILE *file) : Writer() {
        m_file = file;
    }

    virtual esp_err_t write(const char *data, size_t size) {
        if(fwrite(data, 1, size, m_file) != size)
            return ESP_FAIL;
        return ESP_OK;
    }

    esp_err_t writeByte(uint8_t byte) {
        return ESP_FAIL;
    }

private:
    FILE *m_file;
};

static void testInheritance() {
    // Writer w; // - nezkompiluje se, Writer samotný nejde vytvořit

    // Musíme vytvořit až pod-třídy Writeru
    UartWriter uart(UART_NUM_1);
    FileWriter file(stdout);

    // Použijeme metody z Writeru, i když máme instance jeho "dětí"
    uart.printf("test uart %f\n", 4.2);
    file.printf("test file %d\n", 4);

    writeBatteryStatus(uart);
    writeBatteryStatus(file);

    file.writeByte(4); // Zde se použije implementace z FileWriteru místo Writeru.
}

// Zde máme jako parametr Writer. Můžeme ale použít UartWriter a FileWriter,
// protože jsou postavené na Writeru.
static void writeBatteryStatus(Writer& w) {
    // Přestože máme jako typ pouze Writer, metoda printf vevnitř
    // zavolá write() z FileWriteru nebo UartWriteru.
    // Právě to zajišťuje slovíčko virtual, vždy se zavolá metoda typu,
    // kterým proměná opravdu je, i když je zrovna přetypovaný na předka.
    w.printf("baterie: %d\n", rb::Manager::get().battery().pct());

    // Zde se opět volá write z FileWriteru nebo UartWriteru
    const char *test = "testovaci string";
    w.write(test, strlen(test));

    // Oprotim tomu writeByte virtuální není, takže se zde vždy zavolá ta z Writeru.
    // Implementace ve FileWriter se ignoruje.
    w.writeByte(4);
}
