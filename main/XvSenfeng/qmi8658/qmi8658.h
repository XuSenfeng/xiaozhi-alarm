#ifndef QMI8658_H
#define QMI8658_H

#include "i2c_device.h"
#include "math.h"
/*******************************************************************************/
/***************************  姿态传感器 QMI8658 ↓   ****************************/
#define  QMI8658_SENSOR_ADDR       0x6A   // QMI8658 I2C地址
#if CONFIG_USE_QMI8658
// QMI8658寄存器地址
enum qmi8658_reg
{
    QMI8658_WHO_AM_I,
    QMI8658_REVISION_ID,
    QMI8658_CTRL1,
    QMI8658_CTRL2,
    QMI8658_CTRL3,
    QMI8658_CTRL4,
    QMI8658_CTRL5,
    QMI8658_CTRL6,
    QMI8658_CTRL7,
    QMI8658_CTRL8,
    QMI8658_CTRL9,
    QMI8658_CATL1_L,
    QMI8658_CATL1_H,
    QMI8658_CATL2_L,
    QMI8658_CATL2_H,
    QMI8658_CATL3_L,
    QMI8658_CATL3_H,
    QMI8658_CATL4_L,
    QMI8658_CATL4_H,
    QMI8658_FIFO_WTM_TH,
    QMI8658_FIFO_CTRL,
    QMI8658_FIFO_SMPL_CNT,
    QMI8658_FIFO_STATUS,
    QMI8658_FIFO_DATA,
    QMI8658_STATUSINT = 45,
    QMI8658_STATUS0,
    QMI8658_STATUS1,
    QMI8658_TIMESTAMP_LOW,
    QMI8658_TIMESTAMP_MID,
    QMI8658_TIMESTAMP_HIGH,
    QMI8658_TEMP_L,
    QMI8658_TEMP_H,
    QMI8658_AX_L,
    QMI8658_AX_H,
    QMI8658_AY_L,
    QMI8658_AY_H,
    QMI8658_AZ_L,
    QMI8658_AZ_H,
    QMI8658_GX_L,
    QMI8658_GX_H,
    QMI8658_GY_L,
    QMI8658_GY_H,
    QMI8658_GZ_L,
    QMI8658_GZ_H,
    QMI8658_COD_STATUS = 70,
    QMI8658_dQW_L = 73,
    QMI8658_dQW_H,
    QMI8658_dQX_L,
    QMI8658_dQX_H,
    QMI8658_dQY_L,
    QMI8658_dQY_H,
    QMI8658_dQZ_L,
    QMI8658_dQZ_H,
    QMI8658_dVX_L,
    QMI8658_dVX_H,
    QMI8658_dVY_L,
    QMI8658_dVY_H,
    QMI8658_dVZ_L,
    QMI8658_dVZ_H,
    QMI8658_TAP_STATUS = 89,
    QMI8658_STEP_CNT_LOW,
    QMI8658_STEP_CNT_MIDL,
    QMI8658_STEP_CNT_HIGH,
    QMI8658_RESET = 96
};

// 倾角结构体
typedef struct{
    int16_t acc_x;
	int16_t acc_y;
	int16_t acc_z;
	int16_t gyr_x;
	int16_t gyr_y;
	int16_t gyr_z;
	float AngleX;
	float AngleY;
	float AngleZ;
}t_sQMI8658;

class QMI8658 : public I2cDevice {
public:
    void qmi8658_init(void){
        uint8_t id = 0; // 芯片的ID号

        id = ReadReg(QMI8658_WHO_AM_I); // 读芯片的ID号
        while (id != 0x05)  // 判断读到的ID号是否是0x05
        {
            vTaskDelay(1000 / portTICK_PERIOD_MS);  // 延时1秒
            id = ReadReg(QMI8658_WHO_AM_I); // 读取ID号
            ESP_LOGI("QMI8658", "QMI8658 re ID: %02x", id);  // 打印ID号
        }
        ESP_LOGI("QMI8658", "QMI8658 OK!");  // 打印信息
    
        WriteReg(QMI8658_RESET, 0xb0);  // 复位  
        vTaskDelay(10 / portTICK_PERIOD_MS);  // 延时10ms
        WriteReg(QMI8658_CTRL1, 0x40); // CTRL1 设置地址自动增加
        WriteReg(QMI8658_CTRL7, 0x03); // CTRL7 允许加速度和陀螺仪
        WriteReg(QMI8658_CTRL2, 0x95); // CTRL2 设置ACC 4g 250Hz
        WriteReg(QMI8658_CTRL3, 0xd5); // CTRL3 设置GRY 512dps 250Hz 
    }

    QMI8658(i2c_master_bus_handle_t i2c_bus, uint8_t addr) : I2cDevice(i2c_bus, addr){
        qmi8658_init();
    }

    // 读取加速度和陀螺仪寄存器值
    void qmi8658_Read_AccAndGry(t_sQMI8658 *p)
    {
        uint8_t status, data_ready=0;
        int16_t buf[6];

        status = ReadReg(QMI8658_STATUS0); // 读状态寄存器 
        if (status & 0x03) // 判断加速度和陀螺仪数据是否可读
            data_ready = 1;
        
        if (data_ready == 1){  // 如果数据可读
            data_ready = 0;
            ReadRegs(QMI8658_AX_L, (uint8_t *)buf, 12); // 读加速度和陀螺仪值
            p->acc_x = buf[0];
            p->acc_y = buf[1];
            p->acc_z = buf[2];
            p->gyr_x = buf[3];
            p->gyr_y = buf[4];
            p->gyr_z = buf[5];
        }else{
            qmi8658_init(); // 如果数据不可读 重新初始化
        }
    }
    // 获取XYZ轴的倾角值
    void qmi8658_fetch_angleFromAcc(t_sQMI8658 *p)
    {
        float temp;

        qmi8658_Read_AccAndGry(p); // 读取加速度和陀螺仪的寄存器值
        // 根据寄存器值 计算倾角值 并把弧度转换成角度
        temp = (float)p->acc_x / sqrt( ((float)p->acc_y * (float)p->acc_y + (float)p->acc_z * (float)p->acc_z) );
        p->AngleX = atan(temp)*57.29578f; // 180/π=57.29578
        temp = (float)p->acc_y / sqrt( ((float)p->acc_x * (float)p->acc_x + (float)p->acc_z * (float)p->acc_z) );
        p->AngleY = atan(temp)*57.29578f; // 180/π=57.29578
        temp = sqrt( ((float)p->acc_x * (float)p->acc_x + (float)p->acc_y * (float)p->acc_y) ) / (float)p->acc_z;
        p->AngleZ = atan(temp)*57.29578f; // 180/π=57.29578
    }
    // 显示QMI8658的数据
    void deal_qmi8658_data(t_sQMI8658 *p)
    {
        auto& app = Application::GetInstance();

        if(app.device_state_ == kDeviceStateSpeaking || app.device_state_ == kDeviceStateListening){
            float X_tmp = 0, Y_tmp = 0;
            for(int i = 0 ;i < 5; i++){
                qmi8658_fetch_angleFromAcc(p);
                // 输出XYZ轴的倾角
                // ESP_LOGI("QMI8658", "angle_x = %.1f  angle_y = %.1f",p->AngleX, p->AngleY);
                X_tmp += p->AngleX;
                Y_tmp += p->AngleY;
                vTaskDelay(10 / portTICK_PERIOD_MS);
            }
            X_tmp /= 5;
            Y_tmp /= 5;

            if(X_tmp >= 60 && app.position_l == false){
                if (app.device_state_ == kDeviceStateSpeaking) {
                    app.AbortSpeaking(kAbortReasonWakeWordDetected);
                }
                if(app.position_r == true){
                    app.position_l = true;
                    std::string text = "把小智晃晕了";
                    app.protocol_->SendWakeWordDetected(text);
                }else{
                    app.position_l = true;
                    std::string text = "摸摸头";
                    app.protocol_->SendWakeWordDetected(text);
                }
            }
            if(X_tmp <= -60 && app.position_r == false){
                if (app.device_state_ == kDeviceStateSpeaking) {
                    app.AbortSpeaking(kAbortReasonWakeWordDetected);
                }
                // if(app.position_l == true){
                //     app.position_r = true;
                //     std::string text = "把小智晃晕了";
                //     app.protocol_->SendWakeWordDetected(text);
                // }else{
                    app.position_r = true;
                    std::string text = "捏捏脸";
                    app.protocol_->SendWakeWordDetected(text);
                // }
            }
            if(Y_tmp >= 60 && app.position_f == false){
                if (app.device_state_ == kDeviceStateSpeaking) {
                    app.AbortSpeaking(kAbortReasonWakeWordDetected);
                }
                app.position_f = true;
                std::string text = "弹脑瓜";
                app.protocol_->SendWakeWordDetected(text);
            }
            if(Y_tmp <= -60 && app.position_b == false){
                if (app.device_state_ == kDeviceStateSpeaking) {
                    app.AbortSpeaking(kAbortReasonWakeWordDetected);
                }
                app.position_b = true;
                std::string text = "拍拍肚子";
                app.protocol_->SendWakeWordDetected(text);
            }
            if(X_tmp < 30 && X_tmp > -30){
                app.position_l = false;
                app.position_r = false;
            }
            if(Y_tmp < 30 && Y_tmp > -30){
                app.position_f = false;
                app.position_b = false;
            }
        }
    }
};
#endif
#endif