#include <driver/spi_slave.h>


static void setupSpiSlave() {
    spi_bus_config_t buscfg = { 0 }; // { 0 } == všechno uvnitř structu bude nastaveno na 0
    buscfg.miso_io_num = GPIO_NUM_23;
    buscfg.mosi_io_num = GPIO_NUM_33;
    buscfg.sclk_io_num = GPIO_NUM_25;
    // Extra piny pro nějaký 4bit mód - -1 == nepoužíváme
    buscfg.quadwp_io_num = -1;
    buscfg.quadhd_io_num = -1;

    spi_slave_interface_config_t slavecfg = { 0 };
    slavecfg.spics_io_num = GPIO_NUM_10;
    slavecfg.queue_size = 8; // fronta na transakce
    slavecfg.mode = 0; // záleží na zařízení, https://en.wikipedia.org/wiki/Serial_Peripheral_Interface#Mode_numbers
    slavecfg.post_setup_cb = spiSlavePostSetup;
    slavecfg.post_trans_cb = spiSlavePostTrans;

    esp_err_t err = spi_slave_initialize(VSPI_HOST, &buscfg, &slavecfg, 0);
    ESP_ERROR_CHECK(err);

    spiSlaveTransBlocking(VSPI_HOST);
    spiSlaveTransAsync(VSPI_HOST);
}

// je uvnitř přerušení
static void IRAM_ATTR spiSlavePostSetup(spi_slave_transaction_t *trans) {
    printf("Pred odeslanim %dbit transakce\n", trans->length);
}

// je uvnitř přerušení
static void IRAM_ATTR spiSlavePostTrans(spi_slave_transaction_t *trans) {
    printf("Po odeslanim %dbit transakce\n", trans->length);
    // Tady nejde použíz
}

static void spiSlaveTransBlocking(spi_host_device_t host) {
    uint16_t data = 42;

    spi_slave_transaction_t trans = { 0 };
    trans.length = 2*8; // délka dat v BITECH
    // trans.trans_len je vyplněna až po kompletaci transakce, je v ní kolik bitů bylo přijato z masteru
    trans.rx_buffer = NULL; // nechceme nic přijmout
    trans.tx_buffer = &data;

    // zařazí transakci do fronty a počká, dokud ji master nevyčte
    esp_err_t err = spi_slave_transmit(host, &trans, portMAX_DELAY);
    ESP_ERROR_CHECK(err);
}

static void spiSlaveTransAsync(spi_host_device_t host) {
    uint16_t data = 42;

    spi_slave_transaction_t trans = { 0 };
    trans.length = 2*8; // délka dat v BITECH
    // trans.trans_len je vyplněna až po kompletaci transakce, je v ní kolik bitů bylo přijato z masteru
    trans.rx_buffer = NULL; // nechceme nic přijmout
    trans.tx_buffer = &data;


    // Zařadí transakci do fronty. Blokuje pouze v případě, že v queue není místo -
    // viz queue_size v spi_slave_interface_config_t
    esp_err_t err = spi_slave_queue_trans(host, &trans, portMAX_DELAY);
    ESP_ERROR_CHECK(err);


    // Nějaký kód....
    //
    // trans ani data NESMÍ být uvolněny z paměti

    // počkáme, dokud master nevykoná transakci a dostaneme výsledek
    spi_slave_transaction_t *res_trans;
    esp_err_t err = spi_slave_get_trans_result(host, &res_trans, portMAX_DELAY);
    ESP_ERROR_CHECK(err);
    // platí res_trans == &trans
}
