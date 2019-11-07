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
        256,           // Velikost SW TX bufferu - tx_buffer_size
        0,             // queue size - více později
        NULL,          // queue - více později
        0);            // interrupt flags - ESP_INTR_FLAG_* https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/system/intr_alloc.html#macros


    // Poslání dat do UARTu. Používá buffery, takže se data neposílají hned ven, pokud
    // není místo.
    // Pokud je tx_buffer_size v uart_driver_install 0, čeká, dokud se vše nezapíše do
    // HW bufferu nebo nepošle.
    char data[] = { 1, 2, 3, 4 };
    int sentBytes = uart_write_bytes(UART_NUM_1, data, sizeof(data));
    if(sentBytes < 0)
        printf("Chyba pri posilani!\n");

    // Druhá funkce na poslání dat, ignoruje SW buffery. Pokud v HW bufferu není místo (posíláte moc rychle),
    // tak data zahodí(!) (můžete zjistit podle návratové hodnoty). Doporučuju používat jen uart_write_bytes.
    int sentBytes = uart_tx_chars(UART_NUM_1, data, sizeof(data));
    if(sentBytes < 0)
        printf("Chyba pri posilani!\n");

    // Pokud potřebujete, můžete pomocí této fce počkat, dokud nebude vše odeslané. Druhý parametr
    // je maximální doba čekání.
    err = uart_wait_tx_done(UART_NUM_1, pdMS_TO_TICKS(500)); // 500ms
    if(err == ESP_ERR_TIMEOUT)
        printf("Vsechno se nestihlo za 500ms odeslat!\n");



    // Tato funkce vrací, kolik je zrovna v příchozím bufferu bytů:
    size_t rx_available = 0;
    err = uart_get_buffered_data_len(UART_NUM_1, &rx_available);
    ESP_ERROR_CHECK(err);
    printf("Muzu precist %u bytu\n", rx_available);

    // Čtení z uartu. Funkce může přečíst méně bytů, než kolik má buffer, je třeba zkontrolovat návratovou hodnotu!
    // Poslední parametr je čas, jak dlouho se má čekat na další data pokud jich v RX bufferech není dost.
    uint8_t rx_buf[32];
    int readByteCount = uart_read_bytes(UART_NUM_1, rx_buf, sizeof(rx_buf), pdMS_TO_TICKS(100));
    if(readByteCount < 0)
        printf("Chyba pri cteni!\n");
    else
        printf("Precteno %d bytu.\n", readByteCount);


    // Vymaze obsah SW & HW RX (prichozich) bufferu
    uart_flush(UART_NUM_1);

}
