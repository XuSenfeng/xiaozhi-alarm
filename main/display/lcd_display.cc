/*
 * @Descripttion: 
 * @Author: Xvsenfeng helloworldjiao@163.com
 * @LastEditors: Xvsenfeng helloworldjiao@163.com
 * Copyright (c) 2025 by helloworldjiao@163.com, All Rights Reserved. 
 */
#include "lcd_display.h"

#include <font_awesome_symbols.h>
#include <esp_log.h>
#include <esp_err.h>
#include <driver/ledc.h>
#include <vector>
#include <esp_lvgl_port.h>
#include <esp_timer.h>
#include "assets/lang_config.h"

#include "board.h"
#include "settings.h"
#include "application.h"

#define TAG "LcdDisplay"
#define LCD_LEDC_CH LEDC_CHANNEL_0

LV_FONT_DECLARE(font_awesome_30_4);

SpiLcdDisplay::SpiLcdDisplay(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_handle_t panel,
                           gpio_num_t backlight_pin, bool backlight_output_invert,
                           int width, int height, int offset_x, int offset_y, bool mirror_x, bool mirror_y, bool swap_xy,
                           DisplayFonts fonts)
    : LcdDisplay(panel_io, panel, backlight_pin, backlight_output_invert, fonts) {
    width_ = width;
    height_ = height;

    // 创建背光渐变定时器
    const esp_timer_create_args_t timer_args = {
        .callback = [](void* arg) {
            LcdDisplay* display = static_cast<LcdDisplay*>(arg);
            display->OnBacklightTimer();
        },
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "backlight_timer",
        .skip_unhandled_events = true,
    };
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &backlight_timer_));
    InitializeBacklight(backlight_pin);

    // draw white
    std::vector<uint16_t> buffer(width_, 0xFFFF);
    for (int y = 0; y < height_; y++) {
        esp_lcd_panel_draw_bitmap(panel_, 0, y, width_, y + 1, buffer.data());
    }

    // Set the display to on
    ESP_LOGI(TAG, "Turning display on");
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_, true));

    ESP_LOGI(TAG, "Initialize LVGL library");
    lv_init();

    ESP_LOGI(TAG, "Initialize LVGL port");
    lvgl_port_cfg_t port_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_port_init(&port_cfg);

    ESP_LOGI(TAG, "Adding LCD screen");
    const lvgl_port_display_cfg_t display_cfg = {
        .io_handle = panel_io_,
        .panel_handle = panel_,
        .control_handle = nullptr,
        .buffer_size = static_cast<uint32_t>(width_ * 10),
        .double_buffer = false,
        .trans_size = 0,
        .hres = static_cast<uint32_t>(width_),
        .vres = static_cast<uint32_t>(height_),
        .monochrome = false,
        .rotation = {
            .swap_xy = swap_xy,
            .mirror_x = mirror_x,
            .mirror_y = mirror_y,
        },
        .color_format = LV_COLOR_FORMAT_RGB565,
        .flags = {
            .buff_dma = 1,
            .buff_spiram = 0,
            .sw_rotate = 0,
            .swap_bytes = 1,
            .full_refresh = 0,
            .direct_mode = 0,
        },
    };

    display_ = lvgl_port_add_disp(&display_cfg);
    // lv_disp_set_rotation(display_, LV_DISP_ROTATION_90);
    if (display_ == nullptr) {
        ESP_LOGE(TAG, "Failed to add display");
        return;
    }

    if (offset_x != 0 || offset_y != 0) {
        lv_display_set_offset(display_, offset_x, offset_y);
    }

    SetupUI();

    SetBacklight(brightness_);
}

// RGB LCD实现
RgbLcdDisplay::RgbLcdDisplay(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_handle_t panel,
                           gpio_num_t backlight_pin, bool backlight_output_invert,
                           int width, int height, int offset_x, int offset_y,
                           bool mirror_x, bool mirror_y, bool swap_xy,
                           DisplayFonts fonts)
    : LcdDisplay(panel_io, panel, backlight_pin, backlight_output_invert, fonts) {
    width_ = width;
    height_ = height;

        // 创建背光渐变定时器
    const esp_timer_create_args_t timer_args = {
        .callback = [](void* arg) {
            LcdDisplay* display = static_cast<LcdDisplay*>(arg);
            display->OnBacklightTimer();
        },
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "backlight_timer",
        .skip_unhandled_events = true,
    };
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &backlight_timer_));
    InitializeBacklight(backlight_pin);
    
    // draw white
    std::vector<uint16_t> buffer(width_, 0xFFFF);
    for (int y = 0; y < height_; y++) {
        esp_lcd_panel_draw_bitmap(panel_, 0, y, width_, y + 1, buffer.data());
    }

    ESP_LOGI(TAG, "Initialize LVGL library");
    lv_init();

    ESP_LOGI(TAG, "Initialize LVGL port");
    lvgl_port_cfg_t port_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_port_init(&port_cfg);

    ESP_LOGI(TAG, "Adding LCD screen");
    const lvgl_port_display_cfg_t display_cfg = {
        .io_handle = panel_io_,
        .panel_handle = panel_,
        .buffer_size = static_cast<uint32_t>(width_ * 10),
        .double_buffer = true,
        .hres = static_cast<uint32_t>(width_),
        .vres = static_cast<uint32_t>(height_),
        .rotation = {
            .swap_xy = swap_xy,
            .mirror_x = mirror_x,
            .mirror_y = mirror_y,
        },
        .flags = {
            .buff_dma = 1,
            .swap_bytes = 0,
            .full_refresh = 1,
            .direct_mode = 1,
        },
    };

    const lvgl_port_display_rgb_cfg_t rgb_cfg = {
        .flags = {
            .bb_mode = true,
            .avoid_tearing = true,
        }
    };
    
    display_ = lvgl_port_add_disp_rgb(&display_cfg, &rgb_cfg);
    if (display_ == nullptr) {
        ESP_LOGE(TAG, "Failed to add RGB display");
        return;
    }
    
    if (offset_x != 0 || offset_y != 0) {
        lv_display_set_offset(display_, offset_x, offset_y);
    }

    SetupUI();

    SetBacklight(brightness_);
}

LcdDisplay::~LcdDisplay() {
    if (backlight_timer_ != nullptr) {
        esp_timer_stop(backlight_timer_);
        esp_timer_delete(backlight_timer_);
    }
    // 然后再清理 LVGL 对象
    if (content_ != nullptr) {
        lv_obj_del(content_);
    }
    if (status_bar_ != nullptr) {
        lv_obj_del(status_bar_);
    }
    if (side_bar_ != nullptr) {
        lv_obj_del(side_bar_);
    }
    if (container_ != nullptr) {
        lv_obj_del(container_);
    }
    if (display_ != nullptr) {
        lv_display_delete(display_);
    }

    if (panel_ != nullptr) {
        esp_lcd_panel_del(panel_);
    }
    if (panel_io_ != nullptr) {
        esp_lcd_panel_io_del(panel_io_);
    }
}

void LcdDisplay::InitializeBacklight(gpio_num_t backlight_pin) {
    if (backlight_pin == GPIO_NUM_NC) {
        return;
    }

    // Setup LEDC peripheral for PWM backlight control
    const ledc_channel_config_t backlight_channel = {
        .gpio_num = backlight_pin,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LCD_LEDC_CH,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0,
        .flags = {
            .output_invert = backlight_output_invert_,
        }
    };
    const ledc_timer_config_t backlight_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 20000, //背光pwm频率需要高一点，防止电感啸叫
        .clk_cfg = LEDC_AUTO_CLK,
        .deconfigure = false
    };

    ESP_ERROR_CHECK(ledc_timer_config(&backlight_timer));
    ESP_ERROR_CHECK(ledc_channel_config(&backlight_channel));
}

void LcdDisplay::OnBacklightTimer() {
    if (current_brightness_ < brightness_) {
        current_brightness_++;
    } else if (current_brightness_ > brightness_) {
        current_brightness_--;
    }
    
    // LEDC resolution set to 10bits, thus: 100% = 1023
    uint32_t duty_cycle = (1023 * current_brightness_) / 100;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LCD_LEDC_CH, duty_cycle);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LCD_LEDC_CH);
    
    if (current_brightness_ == brightness_) {
        esp_timer_stop(backlight_timer_);
    }
}

void LcdDisplay::SetBacklight(uint8_t brightness) {
    if (backlight_pin_ == GPIO_NUM_NC) {
        return;
    }

    if (brightness > 100) {
        brightness = 100;
    }

    ESP_LOGI(TAG, "Setting LCD backlight: %d%%", brightness);
    // 停止现有的定时器（如果正在运行）
    esp_timer_stop(backlight_timer_);

    Display::SetBacklight(brightness);
    // 启动定时器，每 5ms 更新一次
    ESP_ERROR_CHECK(esp_timer_start_periodic(backlight_timer_, 5 * 1000));
}

bool LcdDisplay::Lock(int timeout_ms) {
    return lvgl_port_lock(timeout_ms);
}

void LcdDisplay::Unlock() {
    lvgl_port_unlock();
}

void type_change_button_cb(lv_event_t* event) {
    ESP_LOGI(TAG, "type_change_button_cb");
    Settings *settings = new Settings("protocol_", true);
    int old_type = settings->GetInt("protocol_type", 0);
    settings->SetInt("protocol_type", !old_type);
    ESP_LOGI(TAG, "change protocol type %d", !old_type);
    delete settings;
    auto& application = Application::GetInstance();
    application.Reboot();

}

struct ThemeColors {
    lv_color_t background;
    lv_color_t text;
    lv_color_t chat_background;
    lv_color_t user_bubble;
    lv_color_t assistant_bubble;
    lv_color_t system_bubble;
    lv_color_t system_text;
    lv_color_t border;
    lv_color_t low_battery;
};

void LcdDisplay::SetupUI() {
    DisplayLockGuard lock(this);
    struct ThemeColors LIGHT_THEME;
    LIGHT_THEME.background = lv_color_white();
    LIGHT_THEME.text = lv_color_black();
    LIGHT_THEME.chat_background = lv_color_hex(0xE0E0E0);
    LIGHT_THEME.user_bubble = lv_color_hex(0x95EC69);
    LIGHT_THEME.assistant_bubble = lv_color_white();
    LIGHT_THEME.system_bubble = lv_color_hex(0xE0E0E0);
    LIGHT_THEME.system_text = lv_color_hex(0x666666);
    LIGHT_THEME.border = lv_color_hex(0xE0E0E0);
    LIGHT_THEME.low_battery = lv_color_black();

    auto screen = lv_screen_active();
    lv_obj_set_style_text_font(screen, fonts_.text_font, 0);
    lv_obj_set_style_text_color(screen, LIGHT_THEME.text, 0);
    lv_obj_set_style_bg_color(screen, LIGHT_THEME.background, 0);

    /* Container */
    container_ = lv_obj_create(screen);
    lv_obj_set_size(container_, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_pad_all(container_, 0, 0); // 设置内边距为0
    lv_obj_set_style_border_width(container_, 0, 0); // 设置边框宽度为0
    lv_obj_set_style_pad_row(container_, 0, 0); // 设置行间距为0
    lv_obj_set_style_bg_color(container_, LIGHT_THEME.background, 0);
    lv_obj_set_style_border_color(container_, LIGHT_THEME.border, 0);

    /* Status bar */
    status_bar_ = lv_obj_create(container_);
    lv_obj_set_size(status_bar_, LV_HOR_RES, fonts_.text_font->line_height + 2);
    lv_obj_set_style_radius(status_bar_, 0, 0);
    lv_obj_set_style_bg_color(status_bar_, LIGHT_THEME.background, 0);
    lv_obj_set_style_text_color(status_bar_, LIGHT_THEME.text, 0);
    lv_obj_set_align(status_bar_, LV_ALIGN_TOP_MID);
    lv_obj_set_style_border_width(status_bar_, 0, 0); // 设置边框宽度为0
    
    /* Content */
    content_ = lv_obj_create(container_);
    lv_obj_set_scrollbar_mode(content_, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_radius(content_, 0, 0);
    lv_obj_set_width(content_, LV_HOR_RES);
    lv_obj_set_height(content_, LV_VER_RES - fonts_.text_font->line_height - 2); // 设置高度为屏幕高度减去状态栏高度
    lv_obj_set_style_pad_all(content_, 5, 0);
    lv_obj_set_style_bg_color(content_, LIGHT_THEME.chat_background, 0);
    lv_obj_set_style_border_color(content_, LIGHT_THEME.border, 0); // Border color for content
    lv_obj_set_style_border_width(content_, 0, 0); // Border width for content
    lv_obj_set_align(content_, LV_ALIGN_BOTTOM_MID); // Align content to the bottom of the screen

    emotion_label_ = lv_label_create(content_);
    lv_obj_set_style_text_font(emotion_label_, &font_awesome_30_4, 0);
    lv_obj_set_style_text_color(emotion_label_, LIGHT_THEME.text, 0);
    lv_label_set_text(emotion_label_, FONT_AWESOME_AI_CHIP);
    lv_obj_align(emotion_label_, LV_ALIGN_CENTER, 0, -32); // Center the label in the content area

    chat_message_label_ = lv_label_create(content_);
    lv_label_set_text(chat_message_label_, "");
    lv_obj_set_width(chat_message_label_, LV_HOR_RES * 0.9); // 限制宽度为屏幕宽度的 90%
    lv_obj_set_height(chat_message_label_, fonts_.text_font->line_height + 3); // 限制高度为字体高度的 2 倍
    lv_label_set_long_mode(chat_message_label_, LV_LABEL_LONG_SCROLL_CIRCULAR); // 设置为自动换行模式
    lv_obj_set_style_text_align(chat_message_label_, LV_TEXT_ALIGN_CENTER, 0); // 设置文本居中对齐
    lv_obj_set_style_text_color(chat_message_label_, LIGHT_THEME.text, 0);
    lv_obj_set_style_bg_color(chat_message_label_, lv_color_hex3(0x0099ff), 0);
    lv_obj_set_style_radius(chat_message_label_, 10, 0);
    lv_obj_set_style_bg_opa(chat_message_label_, 100, 0);
    lv_obj_set_style_text_align(chat_message_label_, LV_TEXT_ALIGN_CENTER, 0); // 设置文本居中对齐
    lv_obj_align_to(chat_message_label_, emotion_label_, LV_ALIGN_OUT_BOTTOM_MID, 0, 65);  // Align chat message label below the emotion label
    lv_obj_add_flag(chat_message_label_, LV_OBJ_FLAG_HIDDEN); // Enable scrolling when focused
    /* Status bar */
    lv_obj_set_style_pad_all(status_bar_, 0, 0);
    lv_obj_set_style_border_width(status_bar_, 0, 0);
    lv_obj_set_style_pad_column(status_bar_, 0, 0);
    lv_obj_set_style_pad_left(status_bar_, 2, 0);
    lv_obj_set_style_pad_right(status_bar_, 2, 0);

    network_label_ = lv_label_create(status_bar_);
    lv_label_set_text(network_label_, "");
    lv_obj_set_style_text_font(network_label_, fonts_.icon_font, 0);
    lv_obj_set_style_text_color(network_label_, LIGHT_THEME.text, 0);
    lv_obj_set_style_text_align(network_label_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_align(network_label_, LV_ALIGN_LEFT_MID); // Align to the left of the status bar

    notification_label_ = lv_label_create(status_bar_);
    lv_obj_set_style_text_align(notification_label_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(notification_label_, LIGHT_THEME.text, 0);
    lv_label_set_text(notification_label_, "");
    lv_obj_add_flag(notification_label_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_align_to(notification_label_, network_label_, LV_ALIGN_OUT_RIGHT_MID, 0, 0); // Align to the right of the network label
    lv_obj_align(notification_label_, LV_ALIGN_CENTER, 0, 0); // Align to the left of the status bar

    status_label_ = lv_label_create(status_bar_);
    lv_label_set_long_mode(status_label_, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_align(status_label_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(status_label_, LIGHT_THEME.text, 0);
    lv_label_set_text(status_label_, "Speaking");
    mute_label_ = lv_label_create(status_bar_);
    lv_label_set_text(mute_label_, "");
    lv_obj_set_style_text_font(mute_label_, fonts_.icon_font, 0);
    lv_obj_set_style_text_color(mute_label_, LIGHT_THEME.text, 0);
    lv_obj_align(status_label_, LV_ALIGN_CENTER, 0, 0); // Align to the center of the status bar

    battery_label_ = lv_label_create(status_bar_);
    lv_label_set_text(battery_label_, "");
    lv_obj_set_style_text_font(battery_label_, fonts_.icon_font, 0);
    lv_obj_set_style_text_color(battery_label_, LIGHT_THEME.text, 0);
    lv_obj_align(battery_label_, LV_ALIGN_RIGHT_MID, 0, 0); // Align to the right of the mute label

    // low_battery_popup_ = lv_obj_create(screen);
    // lv_obj_set_scrollbar_mode(low_battery_popup_, LV_SCROLLBAR_MODE_OFF);
    // lv_obj_set_size(low_battery_popup_, LV_HOR_RES * 0.9, fonts_.text_font.line_height * 2);
    // lv_obj_align(low_battery_popup_, LV_ALIGN_BOTTOM_MID, 0, 0);
    // lv_obj_set_style_bg_color(low_battery_popup_, LIGHT_THEME.low_battery, 0);
    // lv_obj_set_style_radius(low_battery_popup_, 10, 0);

    // low_battery_label_ = lv_label_create(low_battery_popup_);
    // lv_label_set_text(low_battery_label_, Lang::Strings::BATTERY_NEED_CHARGE);
    // lv_obj_set_style_text_color(low_battery_label_, lv_color_white(), 0);
    // lv_obj_center(low_battery_label_);
    // lv_obj_add_flag(low_battery_popup_, LV_OBJ_FLAG_HIDDEN);
    
    lv_obj_t *type_change_button_ = lv_btn_create(status_bar_);
    lv_obj_set_size(type_change_button_, 40, 60);
    lv_obj_set_style_radius(type_change_button_, 3, 0);
    lv_obj_set_style_bg_color(type_change_button_, lv_color_hex3(0xdddddd), 0);
    lv_obj_set_style_bg_opa(type_change_button_, 255, 0);
    lv_obj_set_style_border_width(type_change_button_, 0, 0);
    lv_obj_set_style_border_color(type_change_button_, LIGHT_THEME.border, 0);
    lv_obj_set_style_border_opa(type_change_button_, 255, 0);
    lv_obj_set_height(type_change_button_, 22);
    lv_obj_align(type_change_button_, LV_ALIGN_BOTTOM_RIGHT, -30, -3); // Align to the bottom right corner of the screen
    type_change_label_ = lv_label_create(type_change_button_);
    lv_obj_set_style_text_font(type_change_label_, fonts_.text_font, 0);
    lv_label_set_text(type_change_label_, "");
    lv_obj_set_height(type_change_label_, 20);
    lv_obj_align(type_change_label_, LV_ALIGN_CENTER, 0, 0); // Center the label in the button
    lv_obj_add_event_cb(type_change_button_, type_change_button_cb, LV_EVENT_CLICKED, NULL);

}

void LcdDisplay::SetEmotion(const char* emotion) {
    struct Emotion {
        const char* icon;
        const char* text;
    };

    static const std::vector<Emotion> emotions = {
        {"😶", "neutral"},
        {"🙂", "happy"},
        {"😆", "laughing"},
        {"😂", "funny"},
        {"😔", "sad"},
        {"😠", "angry"},
        {"😭", "crying"},
        {"😍", "loving"},
        {"😳", "embarrassed"},
        {"😯", "surprised"},
        {"😱", "shocked"},
        {"🤔", "thinking"},
        {"😉", "winking"},
        {"😎", "cool"},
        {"😌", "relaxed"},
        {"🤤", "delicious"},
        {"😘", "kissy"},
        {"😏", "confident"},
        {"😴", "sleepy"},
        {"😜", "silly"},
        {"🙄", "confused"}
    };
    
    // 查找匹配的表情
    std::string_view emotion_view(emotion);
    auto it = std::find_if(emotions.begin(), emotions.end(),
        [&emotion_view](const Emotion& e) { return e.text == emotion_view; });

    DisplayLockGuard lock(this);
    if (emotion_label_ == nullptr) {
        return;
    }

    // 如果找到匹配的表情就显示对应图标，否则显示默认的neutral表情
    lv_obj_set_style_text_font(emotion_label_, fonts_.emoji_font, 0);
    if (it != emotions.end()) {
        lv_label_set_text(emotion_label_, it->icon);
    } else {
        lv_label_set_text(emotion_label_, "😶");
    }
}

void LcdDisplay::SetIcon(const char* icon) {
    DisplayLockGuard lock(this);
    if (emotion_label_ == nullptr) {
        return;
    }
    lv_obj_set_style_text_font(emotion_label_, &font_awesome_30_4, 0);
    lv_label_set_text(emotion_label_, icon);
}
