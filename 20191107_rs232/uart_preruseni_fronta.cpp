#include <driver/uart.h>


void uart_nastaveni_preruseni() {
    // Nastaveni UARTu.
    // ESP32 ma 3 uarty, UART0 je ve výchozím stavu připojený piny 1 a 3,
    // a vyvedený ven přes USB. Vypisuje na něj printf() funkce.
    // Použijeme tady tedy UART2.
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    esp_err_t err = uart_param_config(UART_NUM_2, &uart_config);
    ESP_ERROR_CHECK(err); // Shodí program a vypíše hlášku na UART0 pokud err != ESP_OK

    // Parametry jdou změnit i po výchozí nastavení:
    // uart_set_baudrate, uart_set_stop_bits(), uart_set_parity ...

    // Nastavení pinů
    //                       TX           RX           RTS                 CTS
    uart_set_pin(UART_NUM_2, GPIO_NUM_26, GPIO_NUM_27, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    // Nainstalování driveru - všimněte si eventQueue
    QueueHandle_t eventQueue;
    uart_driver_install(
        UART_NUM_2,
        256,           // Velikost SW RX bufferu
        256,           // Velikost SW TX bufferu - tx_buffer_size
        32,            // queue size - velikost bufferu pro přerušení     <--------
        &eventQueue,   // queue - fronta, kam se budou posílat přerušení   <-------
        0);            // interrupt flags - ESP_INTR_FLAG_* https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/system/intr_alloc.html#macros

    // ESP nyní bude všechy přerušení posílat do FreeRTOS queue - fronty.
    // Fronta je kontejner, ze kterého můžeme vyčítat ze předu a přidávat
    // prvky dozadu - FIFO.
    // Dokumentace fronty: https://www.freertos.org/a00018.html
    // Takto se z ní čte:
    uart_event_t uart_event; // https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/peripherals/uart.html#_CPPv412uart_event_t
    for(;;) {
        if(xQueueReceive(eventQueue, &uart_event, pdMS_TO_TICKS(100))) { // posledni parametr je timeout
            printf("Prijaty event %d o velikosti %u\n", uart_event.type, uart_event.size);
            switch(uart_event.type) { // typy eventu: https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/peripherals/uart.html#_CPPv417uart_event_type_t
                case UART_DATA:
                    printf("Prave prislo do RX bufferu %u bytu\n", uart_event.size);
                    break;
                // detekce patternu v datech se nastavuje pres uart_enable_pattern_det_baud_intr(
                // https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/peripherals/uart.html#_CPPv433uart_enable_pattern_det_baud_intr11uart_port_tc7uint8_tiii
                case UART_PATTERN_DET:
                    printf("Na pozici %u v RX bufferu byl detekovan vzor!\n", uart_event.size);
                    break;
            }
        }
    }
    // Z queue musíte dostatečně rychle číst, jinak ztratíte eventy
}
