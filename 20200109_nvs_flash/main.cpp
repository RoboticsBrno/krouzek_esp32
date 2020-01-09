#include <esp_err.h>
#include <nvs_flash.h>
#include <vector>

static void setupNvsFlash() {
    // Inicializace NVS partition v paměti. Tato funkce použije partition
    // pojmenovanou 'nvs', jinou lze inicializovat pomocí nvs_flash_init_partition.
    // Pro šifrování, koukněte na nvs_flash_secure_init
    esp_err_t err = nvs_flash_init();

    // Je možné, že NVS partition má poškozená data, nebo byla naformátovana starou verzí
    // NVS filesystému. V takovém případě je potřeba ji smazat, a inicializovat znova.
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());

        err = nvs_flash_init();
        ESP_ERROR_CHECK(err);
    }

    // Otevřeme namespace v NVS flashce
    nvs_handle nvs;
    //             max 15 znaků     nebo NVS_READONLY
    err = nvs_open("MujNamespace", NVS_READWRITE, &nvs);
    ESP_ERROR_CHECK(err);



    // Nastavování hodnot, jsou pro všechny integer typy, string a binární bloby
    // Klíč může mít maximálně 15 znaků!
    ESP_ERROR_CHECK( nvs_set_u32(nvs, "32bitcislo", 0xFFFFFFFF) );
    ESP_ERROR_CHECK( nvs_set_i8(nvs, "8bitcislo", -2) );

    // String hodnota může mít max 4000 znaků
    ESP_ERROR_CHECK( nvs_set_str(nvs, "string", "SuperUzasnyStringPlnyKrasnychSlov!") );

    // Blob může mít max ~500 kb
    const uint8_t blobHodnota[] = { 0, 1, 2, 3, 4, 5 };
    ESP_ERROR_CHECK( nvs_set_blob(nvs, "blobHodnota", blobHodnota, sizeof(blobHodnota)) );



    // Zapsání změn na disk - musíte zavolat, jinak změny mohou zůstat pouze v paměti!
    ESP_ERROR_CHECK( nvs_commit(nvs) );



    // Čtení hodnot - je potřeba mít vytvořenou proměnnou pro vyčtení, protože funkce vrací chybový status
    uint32_t out32;
    ESP_ERROR_CHECK( nvs_get_u32(nvs, "32bitcislo", &out32) );
    printf("32bitcislo = %u\n", out32);

    int8_t out8;
    ESP_ERROR_CHECK( nvs_get_i8(nvs, "8bitcislo", &out8) );
    printf("8bitcislo = %d\n", (int)out8);



    // Čtení stringu a blobu je složitější. Může mít totiž jakoukoliv délku.
    // Pokud ji neznáte, je potřeba ji nejdříve přečíst tak, že jako 3 parametr dáte NULL:
    size_t required_size;
    ESP_ERROR_CHECK( nvs_get_str(nvs, "string", NULL, &required_size) );

    // Pokud ji znáte, stačí místo toho do required_size nastavit předpokládanou NEBO větší velikost.
    // Počítejte i s koncovým \0 znakem.

    // Teď data vyčteme do vectoru:
    std::vector<char> string_data(required_size);
    ESP_ERROR_CHECK( nvs_get_str(nvs, "string", string_data.data(), &required_size) );
    printf("string = %s\n", string_data.data());



    // Blob nvs_get_blob funguje stějně jako stringy (až na to, že nemá koncovou \0)
    // V tomto případě známe velikost - je to 6. Pokud by byla špatně, fce vrátí ESP_ERR_NVS_INVALID_LENGTH
    required_size = 6;
    uint8_t blobBuff[6];
    ESP_ERROR_CHECK( nvs_get_blob(nvs, "blobHodnota", blobBuff,  &required_size) );



    // Smazání jedné hodnoty:
    ESP_ERROR_CHECK( nvs_erase_key(nvs, "string") );

    // Smazání všeho v namespacu:
    ESP_ERROR_CHECK( nvs_erase_all(nvs) );


    // Commitnutí smazání hodnot
    ESP_ERROR_CHECK( nvs_commit(nvs) );


    // Zavření nvs handle - nenahrazuje nvs_commit!
    nvs_close(nvs);

    // Zavře celý NVS driver, obvyklle nepotřebujete
    nvs_flash_deinit();
}
