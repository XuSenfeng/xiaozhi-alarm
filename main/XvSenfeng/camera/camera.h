#ifndef CAMERA_H
#define CAMERA_H

#include "esp_camera.h"
#include "esp_http_server.h"
/***********************************************************/
/****************    摄像头 ↓   ****************************/
#define CAMERA_PIN_PWDN -1
#define CAMERA_PIN_RESET -1
#define CAMERA_PIN_XCLK 5
#define CAMERA_PIN_SIOD 1
#define CAMERA_PIN_SIOC 2

#define CAMERA_PIN_D7 9
#define CAMERA_PIN_D6 4
#define CAMERA_PIN_D5 6
#define CAMERA_PIN_D4 15
#define CAMERA_PIN_D3 17
#define CAMERA_PIN_D2 8
#define CAMERA_PIN_D1 18
#define CAMERA_PIN_D0 16
#define CAMERA_PIN_VSYNC 3
#define CAMERA_PIN_HREF 46
#define CAMERA_PIN_PCLK 7


#define XCLK_FREQ_HZ 24000000

void bsp_camera_init(void);


typedef struct {
    httpd_req_t *req;
    size_t len;
} jpg_chunking_t;

class Camera{
public:
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
        return res;
    }

    
};

/********************    摄像头 ↑   *************************/
/***********************************************************/

#endif