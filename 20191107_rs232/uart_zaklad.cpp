#include <driver/uart.h>
#include <esp_err.h>

void rs232_vychozi() {
    // Nastaveni UARTu.
    // ESP32 ma 3 uarty, UART0 je ve výchozím stavu připojený piny 1 a 3,
    // a vyvedený ven přes USB. Vypisuje na něj printf() funkce.
    // Použijeme tady tedy UART1.
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    esp_err_t err = uart_param_config(UART_NUM_1, &uart_config);
    ESP_ERROR_CHECK(err); // Shodí program a vypíše hlášku na UART0 pokud err != ESP_OK

    // Parametry jdou změnit i po výchozí nastavení:
    // uart_set_baudrate, uart_set_stop_bits(), uart_set_parity ...

    // Nastavení pinů
    //                       TX           RX           RTS                 CTS
    uart_set_pin(UART_NUM_1, GPIO_NUM_32, GPIO_NUM_33, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    // Nainstalování driveru
    uart_driver_install(
        UART_NUM_1,
        256,           // Velikost SW RX bufferu
        256,           // Velikost SW TX bufferu
        0,             // queue size - více později
        NULL,          // queue - více později
        0);            // interrupt flags - ESP_INTR_FLAG_* https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/system/intr_alloc.html#macros
}
