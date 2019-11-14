#include <driver/i2c.h>

static void testI2CMaster() {
    // Konfigurace driveru - master, 100kHz, pullupy povolené
    i2c_config_t cfg = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = GPIO_NUM_14,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = GPIO_NUM_15,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
    };
    cfg.master.clk_speed = 100000; // 100kHz

    esp_err_t err = i2c_param_config(I2C_NUM_0, &cfg);
    ESP_ERROR_CHECK(err);

    // Zapnutí driveru
    //                                  mode             slave RX buffer  slave TX buffer   interrupt flags
    err = i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0,               0,                0);
    ESP_ERROR_CHECK(err);

    // HW filtrování vysokofrekvenčního šumu - pokud s ním máte problémy, můžete experimentovat s tímto.
    // Druhý parametr je číslo <0;7) , určuje max délku vyfiltrovaných záchvěvů.
    // i2c_filter_enable(I2C_NUM_0, 1);


    // Komunikace po i2c probíhá pomocí těchto příkazů. Když vytvoříte i2c_cmd_handle_t
    // pomocí i2c_cmd_link_create, musíte ho vždy smazat pomocí i2c_cmd_link_delete!
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd); // START signál
    i2c_master_write_byte(cmd, (0x01 << 1) | I2C_MASTER_WRITE, true); // Zápis na adresu 0x01. Poslední parametr znamená,
                                                               // že cílový slave musí poslat ACK bit, obvykle
                                                               // zde chcete mít vždy true.
                                                               // Adresu je potřeba posunout o jeden bit doleva, protože
                                                               // nejspodnější bit určije zda čteme nebo zapisujeme.
    i2c_master_write_byte(cmd, 42, true); // Odeslat datový byte s hodnotou 42 - slave ho přijme
    uint8_t data[] = { 1, 2, 3, 4 };
    i2c_master_write(cmd, data, sizeof(data), true); // Odeslat více bytů za sebou
    i2c_master_stop(cmd); // STOP signál
    i2c_master_cmd_begin(I2C_NUM_0, cmd, portMAX_DELAY); // Odešle celý cmd. Blokuje tak dlouho, dokud není odesláno
                                                         // nebo nevyprší timeout (poslední parametr)
    i2c_cmd_link_delete(cmd); // Smazání cmd. Musíte zavolat na konci odesílání!


    // Další cmd bude na čtení
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (0x01 << 1) | I2C_MASTER_READ, true); // Čtení z adresy 0x1

    uint8_t val;
    i2c_master_read_byte(cmd, &val, I2C_MASTER_ACK); // přečtení jednoho bytu, odešle se odezva ACK - úspěšně přečteno
    uint8_t buf[4];
    i2c_master_read(cmd, buf, sizeof(buf), I2C_MASTER_ACK); // přečtení více bytů

    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_NUM_0, cmd, portMAX_DELAY);
    i2c_cmd_link_delete(cmd);
}
