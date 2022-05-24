#include <driver/uart.h>
#include <driver/gpio.h>
#include <array>
#include <string.h>
#include <algorithm>
#include <functional>
#include <atomic>
#include <freertos/queue.h>

#include <driver/pcnt.h>

struct PcntEvent {
    bool threshold_0;
};

static std::atomic<int32_t> gEncoderOverflown;
static QueueHandle_t gPcntEventQueue;

static void handlePctn0Isr(void*) {
    uint32_t status = 0;
    ESP_ERROR_CHECK(pcnt_get_event_status(PCNT_UNIT_0, &status));

    if(status & PCNT_EVT_H_LIM) {
        gEncoderOverflown.fetch_add(INT16_MAX);
    } else if(status & PCNT_EVT_L_LIM) {
        gEncoderOverflown.fetch_add(INT16_MIN);
    }

    if(status & PCNT_EVT_THRES_0) {
        ets_printf("Threshold 0\n");
    }
    if(status & PCNT_EVT_THRES_1) {
        ets_printf("Threshold 1\n");
    }
}

static void handlePcntGlobalIsr(void *) {
    uint32_t intr_status = PCNT.int_st.val;

    for (int i = 0; i < PCNT_UNIT_MAX; i++) {
        if((intr_status & (1 << i)) == 0) {
            continue;
        }

        if(PCNT.status_unit[i].h_lim_lat) {
            gEncoderOverflown.fetch_add(INT16_MAX);
        } else if(PCNT.status_unit[i].h_lim_lat) {
            gEncoderOverflown.fetch_add(INT16_MIN);
        }

        if(PCNT.status_unit[i].thres0_lat) {
            struct PcntEvent ev = {
                .threshold_0 = true,
            };
            xQueueSendFromISR(gPcntEventQueue, &ev, NULL);
        }


        PCNT.int_clr.val |= (1 << i);
    }
}

extern "C" void app_main()
{
    gPcntEventQueue = xQueueCreate(8, sizeof(PcntEvent));

    const pcnt_config_t cfg = {
        .pulse_gpio_num = GPIO_NUM_13,
        .ctrl_gpio_num = GPIO_NUM_14,
        .lctrl_mode = PCNT_MODE_KEEP,
        .hctrl_mode = PCNT_MODE_REVERSE,
        .pos_mode = PCNT_COUNT_INC,
        .neg_mode = PCNT_COUNT_DEC,
        .counter_h_lim = INT16_MAX,
        .counter_l_lim = INT16_MIN,
        .unit = PCNT_UNIT_0,
        .channel = PCNT_CHANNEL_0,
    };
    ESP_ERROR_CHECK(pcnt_unit_config(&cfg));

    //ESP_ERROR_CHECK(pcnt_isr_service_install(0));
    //ESP_ERROR_CHECK(pcnt_isr_handler_add(PCNT_UNIT_0, handlePctn0Isr, NULL));

    ESP_ERROR_CHECK(pcnt_intr_enable(PCNT_UNIT_0));
    ESP_ERROR_CHECK(pcnt_event_enable(PCNT_UNIT_0, PCNT_EVT_L_LIM));
    ESP_ERROR_CHECK(pcnt_event_enable(PCNT_UNIT_0, PCNT_EVT_H_LIM));
    ESP_ERROR_CHECK(pcnt_event_enable(PCNT_UNIT_0, PCNT_EVT_THRES_0));
    ESP_ERROR_CHECK(pcnt_event_enable(PCNT_UNIT_0, PCNT_EVT_THRES_1));

    pcnt_set_event_value(PCNT_UNIT_0, PCNT_EVT_THRES_0, 5000);
    pcnt_set_event_value(PCNT_UNIT_0, PCNT_EVT_THRES_1, -3000);


    // Nutné!
    pcnt_counter_pause(PCNT_UNIT_0);
    pcnt_counter_clear(PCNT_UNIT_0);
    pcnt_counter_resume(PCNT_UNIT_0);

    ESP_ERROR_CHECK(pcnt_isr_register(handlePcntGlobalIsr, NULL, 0, NULL));

    int16_t counter;
    struct PcntEvent ev;
    while (true) {
        if(xQueueReceive(gPcntEventQueue, &ev, 0)) {
            printf("Threshold 0 via event!\n");
        }

        ESP_ERROR_CHECK(pcnt_get_counter_value(PCNT_UNIT_0, &counter));
        printf("%d\n", gEncoderOverflown.load() + counter);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
