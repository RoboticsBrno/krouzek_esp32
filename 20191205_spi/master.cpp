#include <driver/spi_master.h>
#include <Arduino.h>

// Na reálné použití můžete kouknout v driveru pro senzor na čáru:
// https://github.com/RoboticsBrno/Esp32-Mcp3008-LineSensor/blob/master/src/mcp3008_driver.cpp#L17

static void setupSpiMaster() {

    // Nastavení jedné sběrnice,
    spi_bus_config_t buscfg = { 0 }; // { 0 } == všechno uvnitř structu bude nastaveno na 0
    buscfg.miso_io_num = GPIO_NUM_23;
    buscfg.mosi_io_num = GPIO_NUM_33;
    buscfg.sclk_io_num = GPIO_NUM_25;
    // Extra piny pro nějaký 4bit mód - -1 == nepoužíváme
    buscfg.quadwp_io_num = -1;
    buscfg.quadhd_io_num = -1;

    //                                 VSPI_HOST           DMA kanál
    esp_err_t err = spi_bus_initialize(HSPI_HOST, &buscfg, 0);
    ESP_ERROR_CHECK(err);


    // Přidáme slave zařízení - můžete udělat víckrát, ESP32 podporuje max 3,
    // jinak musíte SS pin ovládat sami.
    spi_device_interface_config_t devcfg = { 0 };
    devcfg.address_bits = 0;
    devcfg.command_bits = 0;
    devcfg.clock_speed_hz = 10000000;
    devcfg.mode = 0; // záleží na zařízení, https://en.wikipedia.org/wiki/Serial_Peripheral_Interface#Mode_numbers
    devcfg.spics_io_num = GPIO_NUM_10;
    devcfg.queue_size = 8; // velikost fronty pro transakce "v letu"
    devcfg.pre_cb = spiMasterPreTrans;
    devcfg.post_cb = spiMasterPostTrans;

    spi_device_handle_t dev = NULL; // objekt zařízení - používá se pro adresování transakcí
    err = spi_bus_add_device(HSPI_HOST, &devcfg, &dev);
    ESP_ERROR_CHECK(err);

    // Dva možné přístupy - nekombinovat!
    spiMasterTransBlocking(dev);
    spiMasterTransAsync(dev);
}

// je uvnitř přerušení
static void IRAM_ATTR spiMasterPreTrans(spi_transaction_t *trans) {
    printf("Pred odeslanim %dbit transakce\n", trans->length);
}

// je uvnitř přerušení
static void IRAM_ATTR spiMasterPostTrans(spi_transaction_t *trans) {
    printf("Po odeslaní %dbit transakce\n", trans->length);
    // Tady nejde použít spi_device_get_trans_result
}

static void spiMasterTransBlocking(spi_device_handle_t dev) {
    char tx[4] = { 1, 2, 3, 4 };
    uint32_t rx;

    spi_transaction_t trans = { 0 };
    trans.length = 4 * 8; // délka transakce v BITECH!
    trans.rx_buffer = (void*)&rx;
    trans.tx_buffer = tx;

    // Odešle transakci, a počká na její dokončení
    esp_err_t err = spi_device_transmit(dev, &trans);
    ESP_ERROR_CHECK(err);

    printf("Vysledek: %u\n", rx);




    spi_transaction_t trans = { 0 };
    trans.length = 4 * 8;
    trans.rx_buffer = NULL; // NULL = čtení je přeskočeno
    trans.tx_buffer = tx;
    err = spi_device_transmit(dev, &trans);
    ESP_ERROR_CHECK(err);

    printf("Vysledek: %u\n", rx);
}

static void spiMasterTransAsync(spi_device_handle_t dev) {
    char tx[4] = { 1, 2, 3, 4 };
    uint32_t rx;

    spi_transaction_t trans1 = { 0 };
    trans1.length = 4 * 8; // délka transakce v BITECH!
    trans1.rx_buffer = (void*)&rx;
    trans1.tx_buffer = tx;
    esp_err_t err = spi_device_queue_trans(dev, &trans1, portMAX_DELAY);
    ESP_ERROR_CHECK(err);

    spi_transaction_t trans2 = { 0 };
    trans2.length = 2 * 8; // délka transakce v BITECH!
    trans2.flags = SPI_TRANS_USE_RXDATA | SPI_TRANS_USE_TXDATA; // použij pole uvnitř spi_transaction_t místo vlastních
    trans2.tx_data[0] = 2; // tx_data a rx_data jsou char[4]
    trans2.tx_data[1] = 0;
    //trans2.rx_buffer = (void*)&rx; // NELZE použít s SPI_TRANS_USE_RXDATA
    //trans2.tx_buffer = tx; // NELZE použít s SPI_TRANS_USE_TXDATA

    esp_err_t err = spi_device_queue_trans(dev, &trans2, portMAX_DELAY);
    ESP_ERROR_CHECK(err);




    // Tady je nějaký další kód který něco dělá....
    delay(2130);
    // trans1, trans2, tx ani rx NESMÍ být uvolněny z paměti




    // počkáme na výsledek první transakce
    spi_transaction_t *res_trans = nullptr;
    err = spi_device_get_trans_result(dev, &res_trans, portMAX_DELAY);
    ESP_ERROR_CHECK(err);

    // platí res_trans == &trans1
    printf("Vysledek 1: %u\n", rx);

    // počkáme na výsledek druhé transakce
    err = spi_device_get_trans_result(dev, &res_trans, portMAX_DELAY);
    ESP_ERROR_CHECK(err);

    // platí res_trans == &trans2
    printf("Vysledek 2: %d %d\n", (int)res_trans->rx_data[0], (int)res_trans->rx_data[1]);
}
