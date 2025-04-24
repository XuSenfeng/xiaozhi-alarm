#include "camera.h"
#include "esp_log.h"
#include "esp_camera.h"
#include "application.h"


#include "esp_http_client.h"

typedef struct {
    uint8_t* buffer;       // 动态增长的缓冲区
    size_t buffer_size;    // 当前已分配缓冲区大小
    size_t data_length;    // 实际存储的数据长度
} jpeg_buffer_t;

// 流式编码回调（累积数据到缓冲区）
static size_t jpg_collect_stream(void* arg, size_t index, const void* data, size_t len) {
    jpeg_buffer_t* buf = (jpeg_buffer_t*)arg;
    
    // 动态扩容（每次增加1KB）
    if (buf->data_length + len > buf->buffer_size) {
        size_t new_size = buf->buffer_size + 1024;
        uint8_t* new_buf = (uint8_t *)realloc(buf->buffer, new_size);
        if (!new_buf) return 0;
        buf->buffer = new_buf;
        buf->buffer_size = new_size;
    }
    
    memcpy(buf->buffer + buf->data_length, data, len);
    buf->data_length += len;
    return len;
}

int parse_ws_url(const char *ws_url, char *ip_buf, int buf_size) {
    // 检查协议头
    if (strncmp(ws_url, "ws://", 5) != 0) {
        return -1; // 协议格式错误
    }

    const char *host_start = ws_url + 5; // 跳过 ws://
    if (*host_start == '\0') {
        return -2; // 空主机地址
    }

    // 查找分隔符 (: 或 /)
    const char *colon = strchr(host_start, ':');
    const char *slash = strchr(host_start, '/');

    // 确定 IP 结束位置
    const char *ip_end = NULL;
    if (colon) ip_end = colon;
    else if (slash) ip_end = slash;
    else ip_end = host_start + strlen(host_start);

    // 计算 IP 长度
    int ip_len = ip_end - host_start;
    if (ip_len <= 0 || ip_len >= buf_size) {
        return -3; // IP 长度无效
    }

    // 提取 IP 地址
    strncpy(ip_buf, host_start, ip_len);
    ip_buf[ip_len] = '\0';

    // 验证 IP 格式 (简单校验)
    if (strchr(ip_buf, '.') == NULL) {
        return -4; // 无效的 IPv4 格式
    }

    return 0;
}

// 修改后的上传处理函数
esp_err_t upload_image_to_server() {
    camera_fb_t *fb = NULL;
    esp_err_t err;
    auto& app = Application::GetInstance();
    if(app.camera_flag == false) {
        ESP_LOGW("Camera", "Camera don't open, please open it by long press the button");
        return ESP_FAIL;
    }

    // 丢弃第一帧（通常包含噪点）
    // 初始化摄像头
    bsp_camera_init();
    vTaskDelay(100 / portTICK_PERIOD_MS);
    fb = esp_camera_fb_get();
    esp_camera_fb_return(fb);
    fb = esp_camera_fb_get();
    
    if (!fb) {
        ESP_LOGE("CAMERA", "Capture failed");
        return ESP_FAIL;
    }
     // 先初始化HTTP客户端
    jpeg_buffer_t jpeg_buf = {
        .buffer = NULL,
        .buffer_size = 0,
        .data_length = 0
    };
    
    if (!frame2jpg_cb(fb, 80, jpg_collect_stream, &jpeg_buf)) {
        ESP_LOGE("UPLOAD", "JPEG encoding failed");
        if (jpeg_buf.buffer) free(jpeg_buf.buffer);
        return ESP_FAIL;
    }
    esp_camera_fb_return(fb);
    esp_camera_deinit();

    // 阶段2：初始化HTTP客户端
    char ip[16] = {0}; // IPv4 最大长度 15
    int ret = parse_ws_url(CONFIG_WEBSOCKET_URL, ip, sizeof(ip));
    if (ret != 0) {
        fprintf(stderr, "解析失败，错误码: %d\n", ret);
        return 1;
    }
    // 生成新 URL
    char http_url[100];
    snprintf(http_url, sizeof(http_url), "http://%s:8003/upload", ip);
    ESP_LOGI("UPLOAD", "HTTP URL: %s", http_url);
    esp_http_client_config_t config = {
        .url = http_url,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 15000,
        .disable_auto_redirect = true,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    
    // 设置精确的Content-Length
    char content_length[16];
    snprintf(content_length, sizeof(content_length), "%zu", jpeg_buf.data_length);
    esp_http_client_set_header(client, "Content-Type", "image/jpeg");
    esp_http_client_set_header(client, "Content-Length", content_length);

    // 发送数据
    err = esp_http_client_open(client, jpeg_buf.data_length);
    if (err == ESP_OK) {
        int written = esp_http_client_write(client, 
            (const char*)jpeg_buf.buffer, jpeg_buf.data_length);
        ESP_LOGI("UPLOAD", "Sent %d/%d bytes", written, jpeg_buf.data_length);
    }
    
    // 清理资源
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    if (jpeg_buf.buffer) free(jpeg_buf.buffer);
    return err;
}

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
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    auto& app = Application::GetInstance();
    if(app.camera_flag == false) {
        ESP_LOGW("Camera", "Camera don't open, please open it by long press the button");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    bsp_camera_init();
    vTaskDelay(100 / portTICK_PERIOD_MS);
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
    jpg_chunking_t jchunk = {req, 0}; // 输入输出参数
    if(res == ESP_OK){
            
            res = frame2jpg_cb(fb, 80, jpg_encode_stream, &jchunk)?ESP_OK:ESP_FAIL;
            httpd_resp_send_chunk(req, NULL, 0);
    }
    ESP_LOGI("Camera", "Send len: %d", jchunk.len);
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




