
#include "AlarmClock.h"
#include "assets/lang_config.h"
#include "board.h"
#include "display.h"
#define TAG "AlarmManager"

#if CONFIG_USE_ALARM
void AlarmManager::GetProximateAlarm(time_t now){
    current_alarm_ = nullptr;
    for(auto& alarm : alarms_){
        if(alarm.time > now && (current_alarm_ == nullptr || alarm.time < current_alarm_->time)){
            current_alarm_ = &alarm;
        }
    } 
}

void AlarmManager::ClearOverdueAlarm(time_t now){
    std::lock_guard<std::mutex> lock(mutex_);
    Settings settings_("alarm_clock", true); // 闹钟设置
    for(auto it = alarms_.begin(); it != alarms_.end();){
        if(it->time <= now){
            for (int i = 0; i < 10; i++){
                if(settings_.GetString("alarm_" + std::to_string(i)) == it->name && settings_.GetInt("alarm_time_" + std::to_string(i)) == it->time){
                    settings_.SetString("alarm_" + std::to_string(i), "");
                    settings_.SetInt("alarm_time_" + std::to_string(i), 0);
                    ESP_LOGI(TAG, "Alarm %s at %d is overdue", it->name.c_str(), it->time);
                }
            }
            it = alarms_.erase(it); // 删除过期的闹钟, 此时it指向下一个元素
        }else{
            it++;
        }
    }
    GetProximateAlarm(now);
}

AlarmManager::AlarmManager(){
    ESP_LOGI(TAG, "AlarmManager init");
    ring_flog = false;
    Settings settings_("alarm_clock", true); // 闹钟设置
    // 从Setting里面读取闹钟列表
    for(int i = 0; i < 10; i++){
        std::string alarm_name = settings_.GetString("alarm_" + std::to_string(i));
        if(alarm_name != ""){
            Alarm alarm;
            alarm.name = alarm_name;
            alarm.time = settings_.GetInt("alarm_time_" + std::to_string(i));
            alarms_.push_back(alarm);
            ESP_LOGI(TAG, "Alarm %s add agein at %d", alarm.name.c_str(), alarm.time);
        }
    }

    // 建立一个时钟
    esp_timer_create_args_t timer_args = {
        .callback = [](void* arg) {
            AlarmManager* alarm_manager = (AlarmManager*)arg;
            alarm_manager->OnAlarm(); // 闹钟响了
        },
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "alarm_timer"
    };
    esp_timer_create(&timer_args, &timer_);
    time_t now = time(NULL);
    // 获取最近的闹钟, 同时清除过期的闹钟
    printf("now: %lld\n", now);
    ClearOverdueAlarm(now);
    
    // 启动闹钟
    if(current_alarm_ != nullptr){
        int new_timer_time = current_alarm_->time - now;
        ESP_LOGI(TAG, "begin a alarm at %d", new_timer_time);
        esp_timer_start_once(timer_, new_timer_time * 1000000);
    }
}


AlarmManager::~AlarmManager(){
    if(timer_ != nullptr){
        esp_timer_stop(timer_);
        esp_timer_delete(timer_);
    }
}


void AlarmManager::SetAlarm(int seconde_from_now, std::string alarm_name){
    std::lock_guard<std::mutex> lock(mutex_);
    Settings settings_("alarm_clock", true); // 闹钟设置
    Alarm alarm;
    alarm.name = alarm_name;
    time_t now = time(NULL);
    alarm.time = now + seconde_from_now;
    alarms_.push_back(alarm);
    // 从设置里面找到第一个空闲的闹钟
    for(int i = 0; i < 10; i++){
        if(settings_.GetString("alarm_" + std::to_string(i)) == ""){
            settings_.SetString("alarm_" + std::to_string(i), alarm_name);
            settings_.SetInt("alarm_time_" + std::to_string(i), alarm.time);
            break;
        }
    }
    Alarm *alarm_first = nullptr;
    for(auto& alarm : alarms_){
        if(alarm_first == nullptr || alarm.time < alarm_first->time){
            alarm_first = &alarm;
        }
    }
    ESP_LOGI(TAG, "Alarm %s set at %d, now first %d", alarm.name.c_str(), alarm.time, alarm_first->time);
    if(current_alarm_ == nullptr || alarm.time < alarm_first->time){
        // 如果新设置的闹钟比当前闹钟早, 则重新设置定时器
        if(current_alarm_ != nullptr){
            esp_timer_stop(timer_);
        }
        current_alarm_ = &alarm;
        ESP_LOGI(TAG, "begin a alarm at %d", seconde_from_now);
        esp_timer_start_once(timer_, seconde_from_now * 1000000);
    }
}

void AlarmManager::OnAlarm(){
    ESP_LOGI(TAG, "=----ring----=");
    auto display = Board::GetInstance().GetDisplay();
    // 遍历闹钟
    Alarm *alarm_first = nullptr;
    for(auto& alarm : alarms_){
        if(alarm.time <= time(NULL)){
            alarm_first = &alarm;
            break;
        }
    }


    display->SetChatMessage("system", alarm_first->name.c_str());
    // // 闹钟响了
    time_t now = time(NULL);
    // 处理一下相同时间的闹钟
    ClearOverdueAlarm(now);
    if(current_alarm_ != nullptr){
        int new_timer_time = current_alarm_->time - now;
        ESP_LOGI(TAG, "begin a alarm at %d", new_timer_time);
        esp_timer_start_once(timer_, new_timer_time * 1000000);
    }
    ring_flog = true;
    // PLay_Music(Lang::Sounds::P3_6.data(), Lang::Sounds::P3_6.size());

}

void AlarmManager::CancelAlarm(std::string alarm_name){
    std::lock_guard<std::mutex> lock(mutex_);
    Settings settings_("alarm_clock", true); // 闹钟设置
    if(current_alarm_ != nullptr && current_alarm_->name == alarm_name){
        current_alarm_ = nullptr;
        esp_timer_stop(timer_); // 取消当前闹钟
        ClearOverdueAlarm(time(NULL)); // 清除过期的闹钟
        if(current_alarm_ != nullptr){
            // 重新设置定时器
            int new_timer_time = current_alarm_->time - time(NULL);
            ESP_LOGI(TAG, "begin a alarm at %d", new_timer_time);
            esp_timer_start_once(timer_, new_timer_time * 1000000);
        }
    }
    for(auto it = alarms_.begin(); it != alarms_.end();){
        if(it->name == alarm_name){
            it = alarms_.erase(it);
        }else{
            it++;
        }
    }
    for(int i = 0; i < 10; i++){
        if(settings_.GetString("alarm_" + std::to_string(i)) == alarm_name){
            settings_.SetString("alarm_" + std::to_string(i), "");
            settings_.SetInt("alarm_time_" + std::to_string(i), 0);
        }
    }
    
}


std::string AlarmManager::GetAlarmsStatus(){
    std::lock_guard<std::mutex> lock(mutex_);
    std::string status;
    for(auto& alarm : alarms_){
        status += alarm.name + " at " + std::to_string(alarm.time) + "\n";
    }
    return status;
}
#endif