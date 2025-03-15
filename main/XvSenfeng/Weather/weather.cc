#include "weather.h"
#include "esp_log.h"
#include "cJSON.h"
#include "esp_http_client.h"
#include "esp_err.h"

#define TAG "Weather"
#if CONFIG_USE_WEATHER
Weather::Weather(){
    city_id_ = -1;
    weather_ = "";
    temperature_ = -1;
    gettting_weather_ = false;
    flashWeather();
    ESP_LOGI("Weather", "Weather::Weather");
}

Weather::~Weather(){
    ESP_LOGI("Weather", "Weather::~Weather");
}

void Weather::GetIPLocal(){
    std::string url = "https://apis.map.qq.com/ws/location/v1/ip?key=" CONFIG_WEATHER_API_KEY;
    // 从网络获取IP地址
    char *data_buf = NULL; 

    esp_http_client_config_t config = {
        .url = url.c_str(),  // 这里替换成自己的GroupId
        .timeout_ms = 1000,
        .buffer_size_tx = 1024  // 默认是512 minimax_key很长 512不够 这里改成1024
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    // POST
    esp_http_client_set_method(client, HTTP_METHOD_GET);

    if (esp_http_client_open(client, 0) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open connection");
        esp_http_client_cleanup(client);
        return;
    }
    int write_len = esp_http_client_write(client, 0, 0);
    ESP_LOGI(TAG, "Need to write %d, written %d", 0, write_len);
    //获取信息长度
    int data_length = esp_http_client_fetch_headers(client);
    if (data_length <= 0) {
        esp_http_client_cleanup(client);
        return;
    }
    //分配空间
    data_buf = (char *)malloc(data_length + 1);
    if(data_buf == NULL) {
        esp_http_client_cleanup(client);
        return;
    }
    data_buf[data_length] = '\0';
    int rlen = esp_http_client_read(client, data_buf, data_length);
    data_buf[rlen] = '\0';
    ESP_LOGI(TAG, "read = %s", data_buf);
    //解析信息
    cJSON *root = cJSON_Parse(data_buf);
    /*
    {
        'status': 0, 
        'message': 'Success', 
        'request_id': 'bf07adca91004d308fcb137738df7146', 
        'result': 
            {
                'ip': '1.202.126.59', 
                'location': {
                    'lat': 39.90469, 
                    'lng': 116.40717
                }, 
                'ad_info': {
                    'nation': '中国', 
                    'province': '北京市', 
                    'city': '北京市', 
                    'district': '', 
                    'adcode': 110000, 
                    'nation_code': 156}
                }
            }
    }
    */
    cJSON *status = cJSON_GetObjectItem(root, "status");
    if (status->valueint != 0) {
        ESP_LOGE(TAG, "status error");
        cJSON_Delete(root);
        free(data_buf);
        esp_http_client_cleanup(client);
    }

    cJSON *result = cJSON_GetObjectItem(root, "result");
    cJSON *ad_info = cJSON_GetObjectItem(result, "ad_info");
    cJSON *adcdoe = cJSON_GetObjectItem(ad_info, "adcode");
    cJSON *district = cJSON_GetObjectItem(ad_info, "district");
    city_name_ = district->valuestring;
    if(city_name_ == ""){
        cJSON *city = cJSON_GetObjectItem(ad_info, "city");
        city_name_ = city->valuestring;
        if(city_name_ == ""){
            cJSON *province = cJSON_GetObjectItem(ad_info, "province");
            city_name_ = province->valuestring;
        }
    }
    city_id_ = adcdoe->valueint;

    cJSON_Delete(root);
    free(data_buf);
    esp_http_client_cleanup(client);

}

void Weather::flashWeather(){
    std::lock_guard<std::mutex> lock(mutex_);
    // 从网络获取天气信息
    // 更新天气信息
    ESP_LOGI("Weather", "flashWeather");
    if (city_id_ == -1) {
        GetIPLocal();
        if(city_id_ == -1){
            return;
        }
    }
    // url = f'https://apis.map.qq.com/ws/weather/v1/?key=IQ7BZ-P7X6W-2I4RK-3BWCR-GN3CZ-PSFQC&adcode={adcode}'
    std::string url = "https://apis.map.qq.com/ws/weather/v1/?key=" + std::string(CONFIG_WEATHER_API_KEY) + "&adcode=" + std::to_string(city_id_);
    // 从网络天气
    char *data_buf = NULL;

    esp_http_client_config_t config = {
        .url = url.c_str(),  // 这里替换成自己的GroupId
        .timeout_ms = 1000,
        .buffer_size_tx = 1024  // 默认是512 minimax_key很长 512不够 这里改成1024
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    // POST
    esp_http_client_set_method(client, HTTP_METHOD_GET);

    if (esp_http_client_open(client, 0) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open connection");
        esp_http_client_cleanup(client);
        return;
    }
    int write_len = esp_http_client_write(client, 0, 0);
    ESP_LOGI(TAG, "Need to write %d, written %d", 0, write_len);

    //获取信息长度
    int data_length = esp_http_client_fetch_headers(client);
    if (data_length <= 0) {
        esp_http_client_cleanup(client);
        return;
    }
    //分配空间
    data_buf = (char *)malloc(data_length + 1);
    if(data_buf == NULL) {
        esp_http_client_cleanup(client);
        return;
    }
    data_buf[data_length] = '\0';
    int rlen = esp_http_client_read(client, data_buf, data_length);
    data_buf[rlen] = '\0';
    ESP_LOGI(TAG, "read = %s", data_buf);

    /*
    {
        'status': 0, 'message': 'Success', 'request_id': '3b1bffe43a194b0b875046a288e46896', 
        'result': {
            'realtime': [
                {
                    'province': '北京市', 
                    'city': '', 'district': '', 
                    'adcode': 110000,  
                    'update_time': '2025-03-05 17:55', 
                    'infos': {
                        'weather': '晴天', 
                        'temperature': 11, 
                        'wind_direction': '西南风', 
                        'wind_power': '4-5级', 
                        'humidity': 30
                    }
                }
            ]
        }
    }
    */
    cJSON *root = cJSON_Parse(data_buf);
    cJSON *status = cJSON_GetObjectItem(root, "status");
    if (status->valueint != 0) {
        ESP_LOGE(TAG, "status error");
        cJSON_Delete(root);
        free(data_buf);
        esp_http_client_cleanup(client);
    }

    cJSON *result = cJSON_GetObjectItem(root, "result");
    cJSON *realtime = cJSON_GetArrayItem(cJSON_GetObjectItem(result, "realtime"), 0);
    cJSON *infos = cJSON_GetObjectItem(realtime, "infos");
    cJSON *weather = cJSON_GetObjectItem(infos, "weather");
    cJSON *temperature = cJSON_GetObjectItem(infos, "temperature");
    cJSON *wind_direction = cJSON_GetObjectItem(infos, "wind_direction");
    cJSON *wind_power = cJSON_GetObjectItem(infos, "wind_power");
    cJSON *humidity = cJSON_GetObjectItem(infos, "humidity");


    weather_ = weather->valuestring;
    temperature_ = temperature->valueint;
    wind_direction_ = wind_direction->valuestring;
    wind_power_ = wind_power->valuestring;
    humidity_ = humidity->valueint;

    cJSON_Delete(root);
    free(data_buf);
    esp_http_client_cleanup(client);
    gettting_weather_ = true;
}

std::string Weather::getWeatherDescription(){
    std::lock_guard<std::mutex> lock(mutex_);
    if( gettting_weather_ ){
        return city_name_ + "\n" + weather_ + " " + std::to_string(temperature_) + "℃" + "\n" 
        + wind_direction_ + " " + wind_power_ + "\n" + "湿度: " +std::to_string(humidity_) + "%";
    }else
        return "天气未获取";
}
#endif