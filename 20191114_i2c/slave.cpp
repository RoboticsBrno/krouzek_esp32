#include <driver/i2c.h>

static void testI2CMaster() {
    // Konfigurace driveru - slave, pullupy zakázané
    i2c_config_t cfg = {
        .mode = I2C_MODE_SLAVE,
        .sda_io_num = GPIO_NUM_16,
        .sda_pullup_en = GPIO_PULLUP_DISABLE,
        .scl_io_num = GPIO_NUM_17,
        .scl_pullup_en = GPIO_PULLUP_DISABLE,
    };
    cfg.slave.addr_10bit_en = 0; // chceme normální 7bit adresy
    cfg.slave.slave_addr = 0x01; // naše adresa je 0x01

    esp_err_t err = i2c_param_config(I2C_NUM_1, &cfg);
    ESP_ERROR_CHECK(err);

    // Zapnutí driveru
    // slave RX/TX buffer hodnoty jsou používany na velikost bufferů v paměti, do který se uloží
    // data před tím, než je přečtete z driveru ve svém kódu.
    //                                  mode             slave RX buffer  slave TX buffer   interrupt flags
    err = i2c_driver_install(I2C_NUM_1, I2C_MODE_SLAVE, 32,              32,                0);
    ESP_ERROR_CHECK(err);

    // HW filtrování vysokofrekvenčního šumu - pokud s ním máte problémy, můžete experimentovat s tímto.
    // Druhý parametr je číslo <0;7) , určuje max délku vyfiltrovaných záchvěvů.
    // i2c_filter_enable(I2C_NUM_0, 1);

    // Počkáme 5s na to, zda přijde <= 5 bytů. V readBytesNum bude kolik jich doopravdy přišlo.
    uint8_t read_buf[5];
    int readBytesNum = i2c_slave_read_buffer(I2C_NUM_1, read_buf, sizeof(read_buf), pdMS_TO_TICKS(5000));

    // Zapíšeme do bufferu data, aby si je master mohl vyčíst. Budou tam čekat tak dlouho, dokud master
    // nevydá READ příkaz.
    uint8_t write_data[] = { 4, 8, 15, 16, 23 };
    i2c_slave_write_buffer(I2C_NUM_1, write_data, sizeof(write_data), portMAX_DELAY);


    intr_handle_t handle;
    i2c_isr_register(I2C_NUM_0, isrHandler, NULL, 0, &handle);

    i2c_isr_free(handle);
}


static void isrHandler(void *cookie) {
    if(i2cREGISTR & BIT_PRISEL_START_PRIKAZ)


    https://github.com/espressif/esp-idf/blob/master/components/driver/i2c.c#L427
}
