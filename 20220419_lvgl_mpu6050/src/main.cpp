#include <driver/uart.h>
#include <driver/gpio.h>
#include <array>
#include <string.h>
#include <algorithm>
#include <functional>
#include <mutex>

#include "esp_vfs.h"
#include "esp_spiffs.h"
#include "esp_log.h"
#include "esp_system.h"

#include "lvgl.h"
#include "lvgl_helpers.h"

#include "lvglui.hpp"

#include "I2Cbus.hpp"
#include "MPU.hpp"        // main file, provides the class itself
#include "mpu/math.hpp"   // math helper for dealing with MPU data
#include "mpu/types.hpp"  // MPU data types and definitions


static void guiTask(void*);

struct UiContext {
    lv_obj_t *barX;
    lv_obj_t *barY;
    lv_obj_t *gyroX;
    lv_obj_t *chart;
    lv_chart_series_t *serX;
    lv_chart_series_t *serY;
    lv_chart_series_t *serZ;

    void createWidgets() {
        /* use a pretty small demo for monochrome displays */
        /* Get the current screen */
        lv_obj_t *scr = lv_disp_get_scr_act(NULL);

       /* this->barX = lv_bar_create(scr, NULL);
        lv_obj_set_width(this->barX, lv_obj_get_width(scr)-20);
        lv_obj_align(this->barX, NULL, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_local_bg_color(this->barX, LV_BAR_PART_INDIC, LV_STATE_DEFAULT, LV_COLOR_GREEN);

        this->barY = lv_bar_create(scr, NULL);
        lv_obj_set_width(this->barY, 10);
        lv_obj_set_height(this->barY, lv_obj_get_height(scr)-20);
        lv_obj_align(this->barY, NULL, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_local_bg_color(this->barY, LV_BAR_PART_INDIC, LV_STATE_DEFAULT, LV_COLOR_RED);

        this->gyroX = lv_arc_create(scr, NULL);
        lv_obj_set_width(this->gyroX, 40);
        lv_obj_set_height(this->gyroX, 40);
        lv_obj_align(this->gyroX, NULL, LV_ALIGN_IN_BOTTOM_LEFT, 10, -10);
        lv_obj_set_style_local_bg_color(this->gyroX, LV_ARC_PART_INDIC, LV_STATE_DEFAULT, LV_COLOR_BLUE);*/

        this->chart = lv_chart_create(scr, NULL);
        lv_obj_set_width(this->chart, lv_obj_get_width(scr));
        lv_obj_set_height(this->chart, lv_obj_get_height(scr));
        lv_chart_set_type(this->chart, LV_CHART_TYPE_LINE);
        lv_chart_set_y_range(this->chart, LV_CHART_AXIS_PRIMARY_Y, -4, 4);

        this->serX = lv_chart_add_series(this->chart, LV_COLOR_GREEN);
        this->serY = lv_chart_add_series(this->chart, LV_COLOR_RED);
        this->serZ = lv_chart_add_series(this->chart, LV_COLOR_BLUE);
    }
};

static LvglUiHelper<UiContext> gUi;

extern "C" void app_main()
{
    xTaskCreatePinnedToCore(guiTask, "gui", 4096*2, NULL, 0, NULL, 1);

    i2c0.begin(GPIO_NUM_13, GPIO_NUM_15, 400000);

    MPU_t mpu;
    mpu.setBus(i2c0);
    mpu.setAddr(mpud::MPU_I2CADDRESS_AD0_LOW);
    printf("MPU test: %d\n", mpu.testConnection());
    ESP_ERROR_CHECK(mpu.initialize());

    mpu.setSampleRate(60);
    mpu.setAccelFullScale(mpud::ACCEL_FS_4G);
    mpu.setGyroFullScale(mpud::GYRO_FS_500DPS);
    mpu.setDigitalLowPassFilter(mpud::DLPF_42HZ);  // smoother data

    mpud::selftest_t selftest = 0;
    mpu.selfTest(&selftest);
    printf("self test: %d\n", selftest);

    while(1) {
        vTaskDelay(10);

        mpud::raw_axes_t accelRaw;
        ESP_ERROR_CHECK(mpu.acceleration(&accelRaw));

        mpud::float_axes_t accelG = mpud::accelGravity(accelRaw, mpud::ACCEL_FS_4G);

        mpud::raw_axes_t gyroRaw;
        ESP_ERROR_CHECK(mpu.rotation(&gyroRaw));

        auto gyro = mpud::gyroDegPerSec(gyroRaw, mpud::GYRO_FS_500DPS);

        gUi.modify([&](UiContext& ctx) {
            lv_chart_set_next(ctx.chart, ctx.serX, accelG.x);
            lv_chart_set_next(ctx.chart, ctx.serY, accelG.y);
            lv_chart_set_next(ctx.chart, ctx.serZ, accelG.z);
            /*lv_bar_set_value(ctx.barX, 50 + (accelG.x*33), false);
            lv_bar_set_value(ctx.barY, 50 + (accelG.y*33), false);
            if(gyro.x < 0) {
                lv_arc_set_angles(ctx.gyroX, 360+gyro.x, 360);
            } else {
                lv_arc_set_angles(ctx.gyroX, 0, gyro.x);
            }*/
        }); 
    }
}

static void guiTask(void*) {
    lv_init();
    lvgl_driver_init();

    lv_color_t* buf1 = (lv_color_t*)heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
    lv_color_t* buf2 = (lv_color_t*)heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);

    lv_disp_buf_t disp_buf;
    uint32_t size_in_px = DISP_BUF_SIZE;

    lv_disp_buf_init(&disp_buf, buf1, buf2, size_in_px);

    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.flush_cb = disp_driver_flush;

    disp_drv.buffer = &disp_buf;
    lv_disp_drv_register(&disp_drv);

    gUi.modify([](UiContext& ctx) {
        ctx.createWidgets();
    });

    auto lastTicks = xTaskGetTickCount();
    while (1) {
        vTaskDelay(1);

        const auto curTicks = xTaskGetTickCount();
        lv_tick_inc(pdTICKS_TO_MS(curTicks - lastTicks));
        lastTicks = curTicks;

        gUi.modify([](UiContext&) {
            lv_task_handler();
        });
    }
}
