#ifndef WEATHER_H
#define WEATHER_H

#include <string>
#include <mutex>

class Weather {
private: 
    int  city_id_;
    std::string city_name_;
    std::string weather_;
    std::string wind_direction_;
    std::string wind_power_;
    int humidity_;
    int temperature_;
    bool gettting_weather_;
    std::mutex mutex_; // 互斥锁
public:
    Weather();
    ~Weather();
    void flashWeather();
    std::string getWeatherDescription();
    void GetIPLocal();
};



#endif 