#include <stdio.h>
#include <string.h>

#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_mesh.h"
#include "esp_mesh_internal.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

#define SSID "Anthrophobia"
#define PASS "Ku1ata2elvA"

#define MESH_TAG "Mesh"

static esp_netif_t* gNetifSta = NULL;

#define RX_SIZE (1500)
#define TX_SIZE (1460)

/*******************************************************
 *                Variable Definitions
 *******************************************************/
static uint8_t rx_buf[RX_SIZE] = {
    0,
};
static bool is_running = true;
static bool is_mesh_connected = false;
static mesh_addr_t mesh_parent_addr;
static int mesh_layer = -1;

static esp_err_t _http_event_handle(esp_http_client_event_t* evt) {
    switch (evt->event_id) {
    case HTTP_EVENT_ON_DATA:
        if (!esp_http_client_is_chunked_response(evt->client)) {
            printf("%.*s", evt->data_len, (char*)evt->data);

            mesh_data_t data = { 0 };
            data.data = evt->data;
            data.size = evt->data_len;

            esp_err_t err = esp_mesh_send(evt->user_data, &data, MESH_DATA_FROMDS, NULL, 0);
            printf("resend %d\n", err);
        }
        break;
    default:
        break;
    }
    return ESP_OK;
}

const char* URL = "http://34.231.30.52/uuid";

static void esp_mesh_p2p_tx_main(void* arg) {
    is_running = true;

    while (is_running) {
        ESP_LOGI(MESH_TAG, "layer:%d, rtableSize:%d, %s", mesh_layer,
            esp_mesh_get_routing_table_size(),
            (is_mesh_connected && esp_mesh_is_root()) ? "ROOT"
                : is_mesh_connected                   ? "NODE"
                                                      : "DISCONNECT");

        mesh_addr_t addr;
        IP4_ADDR(&addr.mip.ip4, 34, 231, 30, 52);
        addr.mip.port = 80;

        mesh_data_t data = { 0 };
        data.data = URL;
        data.size = strlen(URL);
        esp_err_t res = esp_mesh_send(&addr, &data, MESH_DATA_TODS, NULL, 0);
        printf("send res: %d\n", res);

        vTaskDelay(10 * 1000 / portTICK_RATE_MS);
    }
    vTaskDelete(NULL);
}

static void esp_mesh_p2p_rx_main(void* arg) {
    esp_err_t err;
    mesh_addr_t from;
    mesh_data_t data;
    int flag = 0;
    data.data = rx_buf;
    data.size = RX_SIZE;
    is_running = true;

    while (is_running) {
        data.size = RX_SIZE;
        err = esp_mesh_recv(&from, &data, portMAX_DELAY, &flag, NULL, 0);
        if (err != ESP_OK || !data.size) {
            ESP_LOGE(MESH_TAG, "err:0x%x, size:%d", err, data.size);
            continue;
        }

        printf("%.*s\n", data.size, data.data);
    }
    vTaskDelete(NULL);
}

static void ds_forward(void* arg) {
    esp_err_t err;
    mesh_addr_t from, to;
    mesh_data_t data;
    int flag = 0;
    data.data = rx_buf;
    data.size = RX_SIZE;
    is_running = true;

    while (is_running) {
        if(!esp_mesh_is_root()) {
            vTaskDelay(1000);
            continue;
        }

        data.size = RX_SIZE;
        err = esp_mesh_recv_toDS(
            &from, &to, &data, portMAX_DELAY, &flag, NULL, 0);
        if (err != ESP_OK || !data.size) {
            ESP_LOGE(MESH_TAG, "err:0x%x, size:%d", err, data.size);
            continue;
        }

        printf("FORWARD %.*s\n", data.size, data.data);
        data.data[data.size] = 0;

        esp_http_client_config_t config = {
            .url = (char*)data.data,
            .event_handler = _http_event_handle,
            .user_data = &from,
        };
        esp_http_client_handle_t client = esp_http_client_init(&config);
        esp_err_t err = esp_http_client_perform(client);

        if (err == ESP_OK) {
            ESP_LOGI("http", "Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
        }
        esp_http_client_cleanup(client);
    }
    vTaskDelete(NULL);
}

static esp_err_t startFunctions(void) {
    static bool is_comm_p2p_started = false;
    if (!is_comm_p2p_started) {
        is_comm_p2p_started = true;
        xTaskCreate(esp_mesh_p2p_tx_main, "MPTX", 3072, NULL, 5, NULL);
        xTaskCreate(esp_mesh_p2p_rx_main, "MPRX", 3072, NULL, 5, NULL);
        xTaskCreate(ds_forward, "MPRX", 3072, NULL, 5, NULL);
    }
    return ESP_OK;
}

static void mesh_event_handler(void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data) {

    mesh_addr_t id = {
        0,
    };
    static uint16_t last_layer = 0;

    switch (event_id) {
    case MESH_EVENT_STARTED: {
        esp_mesh_get_id(&id);
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_MESH_STARTED>ID:" MACSTR "",
            MAC2STR(id.addr));
        is_mesh_connected = false;
        mesh_layer = esp_mesh_get_layer();
    } break;
    case MESH_EVENT_STOPPED: {
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_STOPPED>");
        is_mesh_connected = false;
        mesh_layer = esp_mesh_get_layer();
    } break;
    case MESH_EVENT_CHILD_CONNECTED: {
        mesh_event_child_connected_t* child_connected
            = (mesh_event_child_connected_t*)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_CHILD_CONNECTED>aid:%d, " MACSTR "",
            child_connected->aid, MAC2STR(child_connected->mac));
    } break;
    case MESH_EVENT_CHILD_DISCONNECTED: {
        mesh_event_child_disconnected_t* child_disconnected
            = (mesh_event_child_disconnected_t*)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_CHILD_DISCONNECTED>aid:%d, " MACSTR "",
            child_disconnected->aid, MAC2STR(child_disconnected->mac));
    } break;
    case MESH_EVENT_ROUTING_TABLE_ADD: {
        mesh_event_routing_table_change_t* routing_table
            = (mesh_event_routing_table_change_t*)event_data;
        ESP_LOGW(MESH_TAG,
            "<MESH_EVENT_ROUTING_TABLE_ADD>add %d, new:%d, layer:%d",
            routing_table->rt_size_change, routing_table->rt_size_new,
            mesh_layer);
    } break;
    case MESH_EVENT_ROUTING_TABLE_REMOVE: {
        mesh_event_routing_table_change_t* routing_table
            = (mesh_event_routing_table_change_t*)event_data;
        ESP_LOGW(MESH_TAG,
            "<MESH_EVENT_ROUTING_TABLE_REMOVE>remove %d, new:%d, layer:%d",
            routing_table->rt_size_change, routing_table->rt_size_new,
            mesh_layer);
    } break;
    case MESH_EVENT_NO_PARENT_FOUND: {
        mesh_event_no_parent_found_t* no_parent
            = (mesh_event_no_parent_found_t*)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_NO_PARENT_FOUND>scan times:%d",
            no_parent->scan_times);
    }
    /* TODO handler for the failure */
    break;
    case MESH_EVENT_PARENT_CONNECTED: {
        mesh_event_connected_t* connected = (mesh_event_connected_t*)event_data;
        esp_mesh_get_id(&id);
        mesh_layer = connected->self_layer;
        memcpy(&mesh_parent_addr.addr, connected->connected.bssid, 6);
        ESP_LOGI(MESH_TAG,
            "<MESH_EVENT_PARENT_CONNECTED>layer:%d-->%d, parent:" MACSTR
            "%s, ID:" MACSTR ", duty:%d",
            last_layer, mesh_layer, MAC2STR(mesh_parent_addr.addr),
            esp_mesh_is_root()      ? "<ROOT>"
                : (mesh_layer == 2) ? "<layer2>"
                                    : "",
            MAC2STR(id.addr), connected->duty);
        last_layer = mesh_layer;
        is_mesh_connected = true;
        if (esp_mesh_is_root()) {
            esp_netif_dhcpc_start(gNetifSta);
        }
        startFunctions();
    } break;
    case MESH_EVENT_PARENT_DISCONNECTED: {
        mesh_event_disconnected_t* disconnected
            = (mesh_event_disconnected_t*)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_PARENT_DISCONNECTED>reason:%d",
            disconnected->reason);
        is_mesh_connected = false;
        mesh_layer = esp_mesh_get_layer();
    } break;
    case MESH_EVENT_LAYER_CHANGE: {
        mesh_event_layer_change_t* layer_change
            = (mesh_event_layer_change_t*)event_data;
        mesh_layer = layer_change->new_layer;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_LAYER_CHANGE>layer:%d-->%d%s",
            last_layer, mesh_layer,
            esp_mesh_is_root()      ? "<ROOT>"
                : (mesh_layer == 2) ? "<layer2>"
                                    : "");
        last_layer = mesh_layer;
    } break;
    case MESH_EVENT_ROOT_ADDRESS: {
        mesh_event_root_address_t* root_addr
            = (mesh_event_root_address_t*)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROOT_ADDRESS>root address:" MACSTR "",
            MAC2STR(root_addr->addr));
    } break;
    case MESH_EVENT_VOTE_STARTED: {
        mesh_event_vote_started_t* vote_started
            = (mesh_event_vote_started_t*)event_data;
        ESP_LOGI(MESH_TAG,
            "<MESH_EVENT_VOTE_STARTED>attempts:%d, reason:%d, rc_addr:" MACSTR
            "",
            vote_started->attempts, vote_started->reason,
            MAC2STR(vote_started->rc_addr.addr));
    } break;
    case MESH_EVENT_VOTE_STOPPED: {
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_VOTE_STOPPED>");
        break;
    }
    case MESH_EVENT_ROOT_SWITCH_REQ: {
        mesh_event_root_switch_req_t* switch_req
            = (mesh_event_root_switch_req_t*)event_data;
        ESP_LOGI(MESH_TAG,
            "<MESH_EVENT_ROOT_SWITCH_REQ>reason:%d, rc_addr:" MACSTR "",
            switch_req->reason, MAC2STR(switch_req->rc_addr.addr));
    } break;
    case MESH_EVENT_ROOT_SWITCH_ACK: {
        /* new root */
        mesh_layer = esp_mesh_get_layer();
        esp_mesh_get_parent_bssid(&mesh_parent_addr);
        ESP_LOGI(MESH_TAG,
            "<MESH_EVENT_ROOT_SWITCH_ACK>layer:%d, parent:" MACSTR "",
            mesh_layer, MAC2STR(mesh_parent_addr.addr));
    } break;
    case MESH_EVENT_TODS_STATE: {
        mesh_event_toDS_state_t* toDs_state
            = (mesh_event_toDS_state_t*)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_TODS_REACHABLE>state:%d", *toDs_state);
    } break;
    case MESH_EVENT_ROOT_FIXED: {
        mesh_event_root_fixed_t* root_fixed
            = (mesh_event_root_fixed_t*)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_ROOT_FIXED>%s",
            root_fixed->is_fixed ? "fixed" : "not fixed");
    } break;
    case MESH_EVENT_ROOT_ASKED_YIELD: {
        mesh_event_root_conflict_t* root_conflict
            = (mesh_event_root_conflict_t*)event_data;
        ESP_LOGI(MESH_TAG,
            "<MESH_EVENT_ROOT_ASKED_YIELD>" MACSTR ", rssi:%d, capacity:%d",
            MAC2STR(root_conflict->addr), root_conflict->rssi,
            root_conflict->capacity);
    } break;
    case MESH_EVENT_CHANNEL_SWITCH: {
        mesh_event_channel_switch_t* channel_switch
            = (mesh_event_channel_switch_t*)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_CHANNEL_SWITCH>new channel:%d",
            channel_switch->channel);
    } break;
    case MESH_EVENT_SCAN_DONE: {
        mesh_event_scan_done_t* scan_done = (mesh_event_scan_done_t*)event_data;
        ESP_LOGI(
            MESH_TAG, "<MESH_EVENT_SCAN_DONE>number:%d", scan_done->number);
    } break;
    case MESH_EVENT_NETWORK_STATE: {
        mesh_event_network_state_t* network_state
            = (mesh_event_network_state_t*)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_NETWORK_STATE>is_rootless:%d",
            network_state->is_rootless);
    } break;
    case MESH_EVENT_STOP_RECONNECTION: {
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_STOP_RECONNECTION>");
    } break;
    case MESH_EVENT_FIND_NETWORK: {
        mesh_event_find_network_t* find_network
            = (mesh_event_find_network_t*)event_data;
        ESP_LOGI(MESH_TAG,
            "<MESH_EVENT_FIND_NETWORK>new channel:%d, router BSSID:" MACSTR "",
            find_network->channel, MAC2STR(find_network->router_bssid));
    } break;
    case MESH_EVENT_ROUTER_SWITCH: {
        mesh_event_router_switch_t* router_switch
            = (mesh_event_router_switch_t*)event_data;
        ESP_LOGI(MESH_TAG,
            "<MESH_EVENT_ROUTER_SWITCH>new router:%s, channel:%d, " MACSTR "",
            router_switch->ssid, router_switch->channel,
            MAC2STR(router_switch->bssid));
    } break;
    case MESH_EVENT_PS_PARENT_DUTY: {
        mesh_event_ps_duty_t* ps_duty = (mesh_event_ps_duty_t*)event_data;
        ESP_LOGI(MESH_TAG, "<MESH_EVENT_PS_PARENT_DUTY>duty:%d", ps_duty->duty);
    } break;
    case MESH_EVENT_PS_CHILD_DUTY: {
        mesh_event_ps_duty_t* ps_duty = (mesh_event_ps_duty_t*)event_data;
        ESP_LOGI(MESH_TAG,
            "<MESH_EVENT_PS_CHILD_DUTY>cidx:%d, " MACSTR ", duty:%d",
            ps_duty->child_connected.aid - 1,
            MAC2STR(ps_duty->child_connected.mac), ps_duty->duty);
    } break;
    default:
        ESP_LOGI(MESH_TAG, "unknown id:%d", event_id);
        break;
    }
}

void app_main() {
    ESP_ERROR_CHECK(nvs_flash_init());

    ESP_ERROR_CHECK(esp_netif_init());
    /*  event initialization */
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    /*  create network interfaces for mesh (only station instance saved for further manipulation, soft AP instance ignored */
    ESP_ERROR_CHECK(
        esp_netif_create_default_wifi_mesh_netifs(&gNetifSta, NULL));

    /*  wifi initialization */
    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&config));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    ESP_ERROR_CHECK(esp_wifi_start());

    /*  mesh initialization */
    ESP_ERROR_CHECK(esp_mesh_init());
    ESP_ERROR_CHECK(esp_event_handler_register(
        MESH_EVENT, ESP_EVENT_ANY_ID, &mesh_event_handler, NULL));
    /*  set mesh topology */
    ESP_ERROR_CHECK(esp_mesh_set_topology(MESH_TOPO_TREE));
    /*  set mesh max layer according to the topology */
    ESP_ERROR_CHECK(esp_mesh_set_max_layer(4));
    ESP_ERROR_CHECK(esp_mesh_set_vote_percentage(1));
    ESP_ERROR_CHECK(esp_mesh_set_xon_qsize(128));

    /* Disable mesh PS function */
    ESP_ERROR_CHECK(esp_mesh_disable_ps());
    ESP_ERROR_CHECK(esp_mesh_set_ap_assoc_expire(10));

    mesh_cfg_t cfg = MESH_INIT_CONFIG_DEFAULT();
    memcpy((uint8_t*)&cfg.mesh_id, "MujMesh", 6);
    cfg.allow_channel_switch = true;

    cfg.router.ssid_len = strlen(SSID);
    strcpy((char*)cfg.router.ssid, SSID);
    strcpy((char*)cfg.router.password, PASS);

    /* mesh softAP */
    ESP_ERROR_CHECK(esp_mesh_set_ap_authmode(WIFI_AUTH_WPA2_PSK));
    cfg.mesh_ap.max_connection = 2;
    strcpy((char*)cfg.mesh_ap.password, "Supertajne");
    ESP_ERROR_CHECK(esp_mesh_set_config(&cfg));
    /* mesh start */
    ESP_ERROR_CHECK(esp_mesh_start());

    while (true) {
        vTaskDelay(1000);
    }
}
