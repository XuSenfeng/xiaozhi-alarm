// #include "iot/thing.h"
// #include "board.h"
// #include "application.h"

// #include <esp_log.h>
// #include "display.h"
// #include "board.h"

// #define TAG "Weather"

// namespace iot {

// // 这里仅定义 Weather 的属性和方法，不包含具体的实现
// class Weather : public Thing {
// public:
//     Weather() : Thing("Weather", "使用屏幕显示当前的天气") {
//         // 定义设备的属性
//         properties_.AddStringProperty("weather", "当前天气", [this]() -> std::string {
//             auto& app = Application::GetInstance();
//             return app.weather_->getWeatherDescription();
//         });

//         // 定义设备可以被远程执行的指令
//         methods_.AddMethod("ShowWeather", "在屏幕显示天气信息", ParameterList(), [this](const ParameterList& parameters) {
//             auto display = Board::GetInstance().GetDisplay();
//             auto& app = Application::GetInstance();
//             ESP_LOGI(TAG, "ShowWeather");
//             display->ShowWeather(app.weather_->getWeatherDescription().c_str());
//         });

//         methods_.AddMethod("ShowChat", "显示对话, 关闭天气显示", ParameterList(), [this](const ParameterList& parameters) {
//             auto display = Board::GetInstance().GetDisplay();
//             ESP_LOGI(TAG, "ShowChat");
//             display->ShowChat();
//         });

//     }
// };

// } // namespace iot

// DECLARE_THING(Weather);