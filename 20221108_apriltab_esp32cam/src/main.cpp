#include <driver/uart.h>
#include <driver/gpio.h>
#include <array>
#include <string.h>
#include <algorithm>
#include <functional>
#include <mutex>
#include <math.h>
#include <vector>

#include "esp_vfs.h"
#include "esp_spiffs.h"
#include "esp_log.h"
#include "esp_system.h"

#include "apriltag/apriltag.h"
#include "apriltag/tag16h5.h"
//#include "apriltag/tag36h11.h"
//#include "apriltag/apriltag_pose.h"

#include "esp_camera.h"

#include "esp_jpg_decode.h"

#include "gridui.h"
#include "rbprotocol.h"
#include "rbwebserver.h"
#include "rbwifi.h"


using namespace rb;
using namespace gridui;

static Protocol* gProt = nullptr;

static const camera_config_t camera_config = {
    .pin_pwdn  = 32,
    .pin_reset = -1,
    .pin_xclk = 0,
    .pin_sscb_sda = 26,
    .pin_sscb_scl = 27,

    .pin_d7 = 35,
    .pin_d6 = 34,
    .pin_d5 = 39,
    .pin_d4 = 36,
    .pin_d3 = 21,
    .pin_d2 = 19,
    .pin_d1 = 18,
    .pin_d0 = 5,
    .pin_vsync = 25,
    .pin_href = 23,
    .pin_pclk = 22,

    .xclk_freq_hz = 20000000, //EXPERIMENTAL: Set to 16MHz on ESP32-S2 or ESP32-S3 to enable EDMA mode
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG, //YUV422,GRAYSCALE,RGB565,JPEG
    .frame_size = FRAMESIZE_QVGA, //QQVGA-QXGA Do not use sizes above QVGA when not JPEG

    .jpeg_quality = 8, //0-63 lower number means higher quality

    .fb_count = 2, //if more than one, i2s runs in continuous mode.
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,//CAMERA_GRAB_LATEST. Sets when buffers should be filled
};

struct code_detected {
    uint8_t id;
    uint8_t hamming;
    float margin;
    float x, y;
    float corners[4][2];
    float H[9];
} __attribute__((__packed__));

struct jpeg_ctx {
    camera_fb_t *fb;
    uint32_t w;
    uint32_t h;
    uint8_t *grey_frame;
};

static uint32_t _jpg_read(void * arg, size_t index, uint8_t *buf, size_t len)
{
    jpeg_ctx * ctx = (jpeg_ctx *)arg;
    if(buf) {
        memcpy(buf, ctx->fb->buf + index, len);
    }
    return len;
}

static uint8_t *grey_frame = nullptr;

static bool _jpg_write(void * arg, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t *data)
{
    jpeg_ctx * ctx = (jpeg_ctx *)arg;
    if(!data){
        if(x == 0 && y == 0){
            //write start
            ctx->w = w;
            ctx->h = h;
            //if output is null, this is BMP
            if(!ctx->grey_frame){
                ctx->grey_frame = grey_frame;//(uint8_t *)
                if(!ctx->grey_frame){
                    return false;
                }
            }
        } else {
            //write end
        }
        return true;
    }

    size_t jw = ctx->w*3;
    size_t jw2 = ctx->w;
    size_t t = y * jw;
    size_t t2 = y * jw2;
    size_t b = t + (h * jw);
    size_t l = x;
    uint8_t *out = ctx->grey_frame;
    uint8_t *o = out;
    size_t iy, iy2, ix, ix2;

    w = w * 3;

    for(iy=t, iy2=t2; iy<b; iy+=jw, iy2+=jw2) {
        o = out+iy2+l;
        for(ix2=ix=0; ix<w; ix+= 3, ix2 +=1) {
            const uint8_t r = data[ix];
            const uint8_t g = data[ix+1];
            const uint8_t b = data[ix+2];
            float px_linear = r*0.3f + g*0.59f + b*0.11f;
            o[ix2] = std::min(px_linear, 255.f);

            //o[ix2] = (r+g+b)/3;
 #if 0
            const float px_linear = (r*0.2126f + g*0.7152f + b*0.0722f)/255.f;
            // Unnecessary cond?
           /* if (px_linear <= 0.0031308f) {
                o[ix2] = 12.92f * px_linear * 255.f;
            } else */{
                o[ix2] = (1.055f * powf(px_linear, 1.f/2.4f) - 0.055f) * 255.f;
            }
#endif
        }
        data+=w;
    }
    return true;
}


static void onPacketReceived(const std::string& cmd, rbjson::Object* pkt) {
    // Let GridUI handle its packets
    if (UI.handleRbPacket(cmd, pkt))
        return;

    // ...any other non-GridUI packets
}

extern "C" void app_main()
{
    {
        uart_config_t uart_config = {
            .baud_rate = 921600,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        };
        ESP_ERROR_CHECK(uart_param_config(UART_NUM_0, &uart_config));
        ESP_ERROR_CHECK(uart_set_pin(UART_NUM_0, 1, 3, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
        ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0, 1024, 1024, 0, NULL, 0));
    }

    // Initialize WiFi
    WiFi::connect("Anthrophobia", "Ku1ata2elvA");

    if(camera_config.pin_pwdn != -1) {
        gpio_set_direction((gpio_num_t)camera_config.pin_pwdn, GPIO_MODE_OUTPUT);
        gpio_set_level((gpio_num_t)camera_config.pin_pwdn, 0);
    }

    while(true) {
        auto err = esp_camera_init(&camera_config);
        if(err == ESP_OK) {
            break;
        } else if(err != ESP_ERR_NOT_FOUND) {
            ESP_ERROR_CHECK(err);
        }
        gpio_set_level((gpio_num_t)camera_config.pin_pwdn, 1);
        vTaskDelay(pdMS_TO_TICKS(300));
        gpio_set_level((gpio_num_t)camera_config.pin_pwdn, 0);
    }


    // Initialize RBProtocol
    gProt = new Protocol("FrantaFlinta", "Robocop", "Compiled at " __DATE__ " " __TIME__, onPacketReceived);
    gProt->start();

    // Start serving the web page
    rb_web_start(80);

    cam_jpg_data web_jpg = {
        .frame = (uint8_t*)heap_caps_malloc(5*1024, MALLOC_CAP_SPIRAM),
        .len = 0,
        .capacity = 5*1024,
        .mutex = xSemaphoreCreateMutex(),
    };
    rb_set_cam_frame(&web_jpg);

    // Initialize the UI builder
    UI.begin(gProt);

    // Commit the layout. Beyond this point, calling any builder methods on the UI is invalid.
    UI.commit();



    auto *sensor = esp_camera_sensor_get();
    /*sensor->set_contrast(sensor, 1);
    sensor->set_sharpness(sensor, 5);
    sensor->set_gain_ctrl(sensor, 0);*/


    auto *family = &tag16h5_family;
    auto *detector = apriltag_detector_create();

    // Sets family and how many wrong bits to tolerate
    apriltag_detector_add_family_bits(detector, family, 0);

    detector->quad_decimate = 2; // Decimate input image by this factor
    detector->quad_sigma = 0.0;  // Apply low-pass blur to input; negative sharpens
    detector->nthreads = 1;
    detector->debug = false;
    detector->refine_edges = true; // Spend more time trying to align edges of tags

    auto start_program = xTaskGetTickCount();
    int frames = 0;

    grey_frame = (uint8_t*)heap_caps_malloc(320*240, MALLOC_CAP_SPIRAM);

    while (1)
    {
        auto start = xTaskGetTickCount();
        camera_fb_t *fb = esp_camera_fb_get();
        const int32_t diff_camm = xTaskGetTickCount() - start;
        if (!fb) {
            ESP_LOGE("camera", "Camera Capture Failed");
            continue;
        }

        xSemaphoreTake(web_jpg.mutex, portMAX_DELAY);
        if(fb->len > web_jpg.capacity) {
            free(web_jpg.frame);
            web_jpg.frame = (uint8_t*)heap_caps_malloc(fb->len, MALLOC_CAP_SPIRAM);
            web_jpg.capacity = fb->len;
        }
        memcpy(web_jpg.frame, fb->buf, fb->len);
        web_jpg.len = fb->len;
        xSemaphoreGive(web_jpg.mutex);

        jpeg_ctx dec_ctx = { .fb = fb };
        esp_jpg_decode(fb->len, JPG_SCALE_NONE, _jpg_read, _jpg_write, &dec_ctx);

        esp_camera_fb_return(fb);


        /*image_u8_t im = {
            .width = (int32_t)fb->width,
            .height = (int32_t)fb->height,
            .stride = (int32_t)fb->width,
            .buf = fb->buf,
        };*/

        image_u8_t im = {
            .width = (int32_t)dec_ctx.w,
            .height = (int32_t)dec_ctx.h,
            .stride = (int32_t)dec_ctx.w,
            .buf = dec_ctx.grey_frame,
        };

        //printf("Got frame\n");

        char header[] = {
            0xFF,
            0x10,
            0x00,
            0x80
        };
        uint8_t cmd = 1;
        /*uart_write_bytes(UART_NUM_0, header, sizeof(header));
        uart_write_bytes(UART_NUM_0, &cmd, 1);
        uart_write_bytes(UART_NUM_0, &fb->width, sizeof(fb->width));
        uart_write_bytes(UART_NUM_0, &fb->height, sizeof(fb->height));
        uart_write_bytes(UART_NUM_0, &fb->len, sizeof(fb->len));
        uart_write_bytes(UART_NUM_0, fb->buf, fb->len);*/

        /*uint32_t len = dec_ctx.w * dec_ctx.h;
        uart_write_bytes(UART_NUM_0, header, sizeof(header));
        uart_write_bytes(UART_NUM_0, &cmd, 1);
        uart_write_bytes(UART_NUM_0, &dec_ctx.w, sizeof(fb->width));
        uart_write_bytes(UART_NUM_0, &dec_ctx.h, sizeof(fb->height));
        uart_write_bytes(UART_NUM_0, &len, sizeof(fb->len));
        uart_write_bytes(UART_NUM_0, dec_ctx.grey_frame, len);*/

        start = xTaskGetTickCount();
        zarray_t *detections = apriltag_detector_detect(detector, &im);
        const int32_t diff_detector = xTaskGetTickCount() - start;


        //printf("Got detections %d in %d ms\n", zarray_size(detections), stop - start);

        cmd = 2;
        uart_write_bytes(UART_NUM_0, header, sizeof(header));
        uart_write_bytes(UART_NUM_0, &cmd, 1);
        uart_write_bytes(UART_NUM_0, &diff_detector, sizeof(diff_detector));
        uart_write_bytes(UART_NUM_0, &diff_camm, sizeof(diff_camm));
        
        uint32_t def_min = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL);
        uint32_t spiram_min = heap_caps_get_minimum_free_size(MALLOC_CAP_SPIRAM);
        uart_write_bytes(UART_NUM_0, &def_min, sizeof(def_min));
        uart_write_bytes(UART_NUM_0, &spiram_min, sizeof(spiram_min));

        float fps = float(frames) / (float(xTaskGetTickCount() - start_program)/1000);
        uart_write_bytes(UART_NUM_0, &fps, sizeof(fps));

        for (int i = 0; i < zarray_size(detections); i++)
        {
            apriltag_detection_t *det;
            zarray_get(detections, i, &det);

            /*apriltag_detection_info_t pose_info = {
                .det = det,
                .tagsize = 0.15f,
                .fx = 182,
                .fy = 137,
                .cx = 320/2,
                .cy = 240/2,
            };

            apriltag_pose_t pose_res = {};
            float pose_err = estimate_tag_pose(&pose_info, &pose_res);*/

            //printf("   ID %d, err %d/%f at %fx%f\n", det->id, det->hamming, det->decision_margin, det->c[0], det->c[1]);

            struct code_detected tx_det = {
                .id = (uint8_t)det->id,
                .hamming = (uint8_t)det->hamming,
                .margin = det->decision_margin,
                .x = (float)det->c[0],
                .y = (float)det->c[1],
                .corners = {
                    { (float)det->p[0][0], (float)det->p[0][1] },
                    { (float)det->p[1][0], (float)det->p[1][1] },
                    { (float)det->p[2][0], (float)det->p[2][1] },
                    { (float)det->p[3][0], (float)det->p[3][1] },
                },
            };
            memcpy(tx_det.H, det->H->data, 9*sizeof(float));
            cmd = 3;
            uart_write_bytes(UART_NUM_0, header, sizeof(header));
            uart_write_bytes(UART_NUM_0, &cmd, 1);
            uart_write_bytes(UART_NUM_0, &tx_det, sizeof(tx_det));
        }
        apriltag_detections_destroy(detections);

        ++frames;
        vTaskDelay(0);
    }
}
