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
#include "settings.h"
#include "application.h"

#define TAG "Protocol"

namespace iot {

// 这里仅定义 Protocol 的属性和方法，不包含具体的实现
class Protocol : public Thing {
public:
    Protocol() : Thing("Protocol", "联网类型") {

        // 定义设备可以被远程执行的指令
        methods_.AddMethod("change_type", "设置连接的服务器类型", ParameterList({
            Parameter("type", "0:使用官方服务器MQTT协议, 1:使用开源服务器Websocket协议, -1切换协议", kValueTypeNumber, true)
        }), [this](const ParameterList& parameters) {
            int type = parameters["type"].number();
            ESP_LOGI(TAG, "change protocol type %d", type);
            Settings *settings = new Settings("protocol_", true);
            switch(type){
                case 0:
                    settings->SetInt("protocol_type", 0);
                    break;
                case 1:
                    settings->SetInt("protocol_type", 1);
                    break;
                default:
                    int old_type = settings->GetInt("protocol_type", 0);
                    settings->SetInt("protocol_type", !old_type);
                    ESP_LOGI(TAG, "change protocol type %d", !old_type);
            }
            delete settings;
            auto& application = Application::GetInstance();
            application.Reboot();
        });
    }
};

} // namespace iot

DECLARE_THING(Protocol);