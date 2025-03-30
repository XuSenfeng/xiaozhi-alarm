#include "camera.h"
#include "esp_log.h"
#include "esp_camera.h"

// 处理流 
static size_t jpg_encode_stream(void * arg, size_t index, const void* data, size_t len){
    jpg_chunking_t *j = (jpg_chunking_t *)arg;
    if(!index){
        j->len = 0;
    }
    // 发送这个图片http响应
    if(httpd_resp_send_chunk(j->req, (const char *)data, len) != ESP_OK){
        return 0;
    }
    j->len += len;
    return len;
}

esp_err_t jpg_httpd_handler(httpd_req_t *req){
    bsp_camera_init();
    vTaskDelay(100 / portTICK_PERIOD_MS);
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;

    fb = esp_camera_fb_get(); // 第一帧的数据不使用
    esp_camera_fb_return(fb); // 处理结束以后把这部分的buf返回
    fb = esp_camera_fb_get();
    if (!fb) {
        ESP_LOGE("Camera", "Camera capture failed");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    res = httpd_resp_set_type(req, "image/jpeg");
    if(res == ESP_OK){
        res = httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");
    }

    if(res == ESP_OK){
            jpg_chunking_t jchunk = {req, 0}; // 输入输出参数
            res = frame2jpg_cb(fb, 80, jpg_encode_stream, &jchunk)?ESP_OK:ESP_FAIL;
            httpd_resp_send_chunk(req, NULL, 0);
    }
    esp_camera_fb_return(fb); // 处理结束以后把这部分的buf返回
    esp_camera_deinit();
    return res;
}

httpd_uri_t uri_handler_jpg = {
    .uri = "/jpg",
    .method = HTTP_GET,
    .handler = jpg_httpd_handler};

httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Start the httpd server
    ESP_LOGI("Camera", "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK)
    {
        // Set URI handlers
        ESP_LOGI("Camera", "Registering URI handlers");
        httpd_register_uri_handler(server, &uri_handler_jpg);
        return server;
    }

    ESP_LOGI("Camera", "Error starting server!");
    return NULL;
}


// 摄像头硬件初始化
void bsp_camera_init(void)
{
    extern Pca9557* pca9557_;
    pca9557_->SetOutputState(2, 0);

    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_1;  // LEDC通道选择  用于生成XCLK时钟 但是S3不用
    config.ledc_timer = LEDC_TIMER_1; // LEDC timer选择  用于生成XCLK时钟 但是S3不用
    config.pin_d0 = CAMERA_PIN_D0;
    config.pin_d1 = CAMERA_PIN_D1;
    config.pin_d2 = CAMERA_PIN_D2;
    config.pin_d3 = CAMERA_PIN_D3;
    config.pin_d4 = CAMERA_PIN_D4;
    config.pin_d5 = CAMERA_PIN_D5;
    config.pin_d6 = CAMERA_PIN_D6;
    config.pin_d7 = CAMERA_PIN_D7;
    config.pin_xclk = CAMERA_PIN_XCLK;
    config.pin_pclk = CAMERA_PIN_PCLK;
    config.pin_vsync = CAMERA_PIN_VSYNC;
    config.pin_href = CAMERA_PIN_HREF;
    config.pin_sccb_sda = -1;   // 这里写-1 表示使用已经初始化的I2C接口
    config.pin_sccb_scl = CAMERA_PIN_SIOC;
    config.sccb_i2c_port = 1;
    config.pin_pwdn = CAMERA_PIN_PWDN;
    config.pin_reset = CAMERA_PIN_RESET;
    config.xclk_freq_hz = XCLK_FREQ_HZ;
    config.pixel_format = PIXFORMAT_RGB565;
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;

    // camera init
    esp_err_t err = esp_camera_init(&config); // 配置上面定义的参数
    if (err != ESP_OK)
    {
        ESP_LOGE("Camera", "Camera init failed with error 0x%x", err);
        return;
    }

    sensor_t *s = esp_camera_sensor_get(); // 获取摄像头型号

    if (s->id.PID == GC0308_PID) {
        s->set_hmirror(s, 1);  // 这里控制摄像头镜像 写1镜像 写0不镜像
    }
}




