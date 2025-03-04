#ifndef ALARMCLOCK_H
#define ALARMCLOCK_H

#include <string>
#include <vector>
#include <esp_log.h>
#include <esp_timer.h>
#include "time.h"
#include <mutex>
#include "settings.h"
#include <atomic>
#if CONFIG_USE_ALARM
struct Alarm {
    std::string name;
    int time;
};

class AlarmManager {
public:
    AlarmManager();
    ~AlarmManager();

    // 设置闹钟
    void SetAlarm(int seconde_from_now, std::string alarm_name);
    // 取消闹钟
    void CancelAlarm(std::string alarm_name);
    // 获取闹钟列表
    std::string GetAlarmsStatus();
    void ClearOverdueAlarm(time_t now);
    Alarm *GetProximateAlarm(time_t now);
    void OnAlarm();
    bool IsRing(){ return ring_flag; };
    void ClearRing(){ESP_LOGI("Alarm", "clear");ring_flag = false;};

private:
    std::vector<Alarm> alarms_; // 闹钟列表
    std::mutex mutex_; // 互斥锁
    esp_timer_handle_t timer_; // 定时器

    std::atomic<bool> ring_flag{false}; 
    std::atomic<bool> running_flag{false};
};
#endif
#endif