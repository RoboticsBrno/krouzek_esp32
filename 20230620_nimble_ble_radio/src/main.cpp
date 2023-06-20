
#include "nvs_flash.h"

#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "host/ble_gap.h"
#include "host/ble_hs_adv.h"

static uint8_t own_addr_type;

static const char *to_adv = "Hello world";

static void start_periodic_adv(void) {

    printf("ble_gap_adv_set_data\n");

    struct ble_hs_adv_fields fields = {};
    fields.mfg_data = (const uint8_t*)to_adv;
    fields.mfg_data_len = strlen(to_adv);

    fields.name = (const uint8_t*)to_adv;
    fields.name_len = strlen(to_adv);
    fields.name_is_complete = 1;

    int res = ble_gap_adv_set_fields(&fields);

//    int res = ble_gap_adv_set_data((const uint8_t*)to_adv, sizeof(to_adv));
    printf("ble_gap_adv_set_data %d\n", res);

    struct ble_gap_adv_params adv_params = {
        .conn_mode = BLE_GAP_CONN_MODE_NON,
        .disc_mode = BLE_GAP_DISC_MODE_GEN,
        .itvl_min = 0,
        .itvl_max = 0,
        .channel_map = 0,
        .filter_policy = 0,
        .high_duty_cycle = 0,
    };

    res = ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER, &adv_params, NULL, NULL);
    printf("ble_gap_adv_start %d\n", res);
}

static void printHex(const uint8_t *data, size_t len) {
    for(size_t i = 0; i < len; ++i) {
        printf("%02x", (unsigned)data[i]);
    }
}

static int
periodic_sync_gap_event(ble_gap_event *event, void *arg)
{
    printf("gap event %d\n", event->type);
    switch (event->type) {
    case BLE_GAP_EVENT_DISC:
        // esp adress prefix: 30:ae:a4
        printf("got disc from ");
        printHex(event->disc.addr.val, 6);
        printf(" data: %.*s ", event->disc.length_data, event->disc.data);
        printHex(event->disc.data, event->disc.length_data);
        printf("\n");
        return 0;
    default:
        return 0;
    }
}


static void
periodic_adv_on_reset(int reason)
{
    MODLOG_DFLT(ERROR, "Resetting state; reason=%d\n", reason);
}

static void
periodic_adv_on_sync(void)
{
    int rc;
    rc = ble_hs_util_ensure_addr(0);
    assert(rc == 0);

    /* Figure out address to use while advertising (no privacy for now) */
    rc = ble_hs_id_infer_auto(0, &own_addr_type);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "error determining address type; rc=%d\n", rc);
        return;
    }

    /* Begin advertising. */
    start_periodic_adv();


    struct ble_gap_disc_params dparams = {};
    dparams.passive = 1;

    int res = ble_gap_disc(own_addr_type, BLE_HS_FOREVER, &dparams,
                      periodic_sync_gap_event, NULL);
    printf("ble_gap_disc %d\n", res);
}


static void periodic_adv_host_task(void *param)
{
    ESP_LOGI("ble", "BLE Host Task Started");
    /* This function will return only when nimble_port_stop() is executed */
    nimble_port_run();

    nimble_port_freertos_deinit();
}

extern "C" void app_main() {
    printf("start\n");
    ESP_ERROR_CHECK(nvs_flash_init());

    printf("nimble_port_init\n");

    nimble_port_init();

    ble_hs_cfg.reset_cb = periodic_adv_on_reset;
    ble_hs_cfg.sync_cb = periodic_adv_on_sync;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;
    /* Set the default device name. */
    int res = ble_svc_gap_device_name_set("nimble_periodic_adv");
    printf("ble_svc_gap_device_name_set %d\n", res);

    //ble_store_config_init();

    nimble_port_freertos_init(periodic_adv_host_task);

    vTaskDelay(pdMS_TO_TICKS(2000));

    while(1) {
        vTaskDelay(10);
    }

}

