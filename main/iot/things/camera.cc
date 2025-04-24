/*
 * @Descripttion: 
 * @Author: Xvsenfeng helloworldjiao@163.com
 * @LastEditors: Xvsenfeng helloworldjiao@163.com
 * Copyright (c) 2025 by helloworldjiao@163.com, All Rights Reserved. 
 */
/*
 * @Descripttion: 
 * @Author: Xvsenfeng helloworldjiao@163.com
 * @LastEditors: Xvsenfeng helloworldjiao@163.com
 * Copyright (c) 2025 by helloworldjiao@163.com, All Rights Reserved. 
 */
#include "iot/thing.h"
#include "board.h"
#include "display/lcd_display.h"
#include "camera.h"
#include <esp_log.h>

#define TAG "Camera"

namespace iot {

// 这里仅定义 Camera 的属性和方法，不包含具体的实现
class Camera : public Thing {
public:
    Camera() : Thing("Camera", "照相机") {

        // 定义设备可以被远程执行的指令
        methods_.AddMethod("take_photo", "无", ParameterList(), [this](const ParameterList& parameters) {
            ESP_LOGI(TAG, "take_photo");
            upload_image_to_server();
        });
    }
};

} // namespace iot

DECLARE_THING(Camera);