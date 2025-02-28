#ifndef ALARMCLOCK_H
#define ALARMCLOCK_H

#include <string>
#include <vector>
#include <esp_log.h>
#include <esp_timer.h>
#include "time.h"
#include <mutex>
#include "settings.h"

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
    void GetProximateAlarm(time_t now);
    void OnAlarm();

private:
    std::vector<Alarm> alarms_; // 闹钟列表
    Alarm *current_alarm_; // 当前闹钟
    std::mutex mutex_; // 互斥锁
    esp_timer_handle_t timer_; // 定时器
};

#endif