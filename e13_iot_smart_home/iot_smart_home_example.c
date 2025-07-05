/*
 * Copyright (c) 2024 iSoftStone Education Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdbool.h>

#include "los_task.h"
#include "ohos_init.h"
#include "cmsis_os.h"
#include "config_network.h"
#include "smart_home.h"
#include "smart_home_event.h"
#include "su_03t.h"
#include "iot.h"
#include "lcd.h"
#include "picture.h"
#include "adc_key.h"
#include <stdio.h>

#include "iot_errno.h"
#include "iot_pwm.h"


#define ROUTE_SSID      "Wang"          // WiFi账号
#define ROUTE_PASSWORD "20050219"       // WiFi密码

#define MSG_QUEUE_LENGTH                                16
#define BUFFER_LEN                                      50

#define TEMP_THRESHOLD 30.0 // 温度触发阈值（℃）
#define MOTOR_PWM_PIN 5 // 假设电机通过PWM引脚控制（需根据硬件修改）
static unsigned int m_msg_queue;
unsigned int m_su03_msg_queue;
bool alarm_light_state = false;
bool beep_state = false;
bool auto_state = false;
/***************************************************************
 * 函数名称: iot_thread
 * 说    明: iot线程
 * 参    数: 无
 * 返 回 值: 无
 ***************************************************************/
void iot_thread(void *args) {
  uint8_t mac_address[12] = {0x00, 0xdc, 0xb6, 0x90, 0x01, 0x00,0};

  char ssid[32]=ROUTE_SSID;
  char password[32]=ROUTE_PASSWORD;
  char mac_addr[32]={0};

  FlashDeinit();
  FlashInit();

  VendorSet(VENDOR_ID_WIFI_MODE, "STA", 3); // 配置为Wifi STA模式
  VendorSet(VENDOR_ID_MAC, mac_address, 6); // 多人同时做该实验，请修改各自不同的WiFi MAC地址
  VendorSet(VENDOR_ID_WIFI_ROUTE_SSID, ssid, sizeof(ssid));
  VendorSet(VENDOR_ID_WIFI_ROUTE_PASSWD, password,sizeof(password));

reconnect:
  SetWifiModeOff();
  int ret = SetWifiModeOn();
  if(ret != 0){
    printf("wifi connect failed,please check wifi config and the AP!\n");
    return;
  }
  mqtt_init();

  while (1) {
    if (!wait_message()) {
      goto reconnect;
    }
    LOS_Msleep(1);
  }
}


/***************************************************************
 * 函数名称: smart_home_thread
 * 说    明: 智慧家居主线程
 * 参    数: 无
 * 返 回 值: 无
 ***************************************************************/
void smart_home_thread(void *arg)
{
    double *data_ptr = NULL;

    double mq2_ppm = 0.0;
  
    

    double illumination_range = 50.0;
    double temperature_range = 35.0;
    double humidity_range = 80.0;
    double ppm_range = 800.0;

    e_iot_data iot_data = {0};

    i2c_dev_init();
    lcd_dev_init();
    motor_dev_init();
    light_dev_init();
    su03t_init();
    beep_dev_init();
    body_induction_dev_init();
    alarm_light_init();
      mq2_init(); 

    // lcd_load_ui();
    lcd_show_ui();

    while(1)
    {
        event_info_t event_info = {0};
        //等待事件触发,如有触发,则立即处理对应事件,如未等到,则执行默认的代码逻辑,更新屏幕
        int ret = smart_home_event_wait(&event_info,3000);
        if(ret == LOS_OK){
            //收到指令
            printf("event recv %d ,%d\n",event_info.event,event_info.data.iot_data);
            switch (event_info.event)
            {
                case event_key_press:
                    smart_home_key_process(event_info.data.key_no);
                    
                    break;
                case event_iot_cmd:
                    smart_home_iot_cmd_process(event_info.data.iot_data);
                    break;
                case event_su03t:
                    smart_home_su03t_cmd_process(event_info.data.su03t_data);
                    break;
               default:break;
            }

        }

        double temp,humi,lum;

        bool light_last_state = false;
        bool body_state = false;
        sht30_read_data(&temp,&humi);
        bh1750_read_data(&lum);
        body_induction_get_state(&body_state);
        mq2_read_data(&mq2_ppm);
        if(body_state){
            lcd_set_body_induction(true);
            beep_set_state(true);
            LOS_Msleep(2000);
            beep_set_state(false);
        } else {
             lcd_set_body_induction(false);
        }

       if (temp > 35.0) {
         lcd_set_motor_state(true);
            motor_set_state(true); // 开启风扇
           // lcd_set_motor_state(true);
      } else { 
         lcd_set_motor_state(false);
            motor_set_state(false); // 关闭风扇
            //lcd_set_motor_state(false);
      }


    
    mq2_read_data(&mq2_ppm);  // 读取MQ2传感器数据（可燃气体浓度）
    if (mq2_ppm > ppm_range) {  // 如果浓度超过阈值（800ppm）
        alarm_light_state = true;  // 设置报警灯状态为开启
        alarm_light_set_gpio(true); // 打开报警灯
        beep_set_state(true);
    } else {
        alarm_light_state = false; // 设置报警灯状态为关闭
        alarm_light_set_gpio(false); // 关闭报警灯
         beep_set_state(false);
    }


        lcd_set_ppm(mq2_ppm);
        lcd_set_illumination(lum);
        lcd_set_temperature(temp);
        lcd_set_humidity(humi);
        if (mqtt_is_connected()) 
        {
            // 发送iot数据
            iot_data.illumination = lum;
            iot_data.temperature = temp;
            iot_data.humidity = humi;
              iot_data.gas = mq2_ppm;
            iot_data.light_state = get_light_state();
            iot_data.motor_state = get_motor_state();
            // iot_data.auto_state = auto_state;
            send_msg_to_mqtt(&iot_data);

            lcd_set_network_state(true);
        }else{  
            lcd_set_network_state(false);
        }

        lcd_show_ui();
        
    }
}

/***************************************************************
 * 函数名称: device_read_thraed
 * 说    明: 设备读取线程
 * 参    数: 无
 * 返 回 值: 无
 ***************************************************************/
// void device_read_thraed(void *arg)
// {
//     double read_data[3] = {0};

//     i2c_dev_init();

//     while(1)
//     {
//         bh1750_read_data(&read_data[0]);
//         sht30_read_data(&read_data[1]);
//         LOS_QueueWrite(m_msg_queue, (void *)&read_data, sizeof(read_data), LOS_WAIT_FOREVER);
//         LOS_QueueWrite(m_su03_msg_queue, (void *)&read_data, sizeof(read_data), LOS_WAIT_FOREVER);
//         LOS_Msleep(500);
//     }
// }

/***************************************************************
 * 函数名称: iot_smart_hone_example
 * 说    明: 开机自启动调用函数
 * 参    数: 无
 * 返 回 值: 无
 ***************************************************************/
void iot_smart_home_example()
{
    unsigned int thread_id_1;
    unsigned int thread_id_2;
    unsigned int thread_id_3;
    TSK_INIT_PARAM_S task_1 = {0};
    TSK_INIT_PARAM_S task_2 = {0};
    TSK_INIT_PARAM_S task_3 = {0};
    unsigned int ret = LOS_OK;
    
    smart_home_event_init();
    
    // ret = LOS_QueueCreate("su03_queue", MSG_QUEUE_LENGTH, &m_su03_msg_queue, 0, BUFFER_LEN);
    // if (ret != LOS_OK)
    // {
    //     printf("Falied to create Message Queue ret:0x%x\n", ret);
    //     return;
    // }

    task_1.pfnTaskEntry = (TSK_ENTRY_FUNC)smart_home_thread;
    task_1.uwStackSize = 2048;
    task_1.pcName = "smart hone thread";
    task_1.usTaskPrio = 24;
    
    ret = LOS_TaskCreate(&thread_id_1, &task_1);
    if (ret != LOS_OK)
    {
        printf("Falied to create task ret:0x%x\n", ret);
        return;
    }

    task_2.pfnTaskEntry = (TSK_ENTRY_FUNC)adc_key_thread;
    task_2.uwStackSize = 2048;
    task_2.pcName = "key thread";
    task_2.usTaskPrio = 24;
    ret = LOS_TaskCreate(&thread_id_2, &task_2);
    if (ret != LOS_OK)
    {
        printf("Falied to create task ret:0x%x\n", ret);
        return;
    }

    task_3.pfnTaskEntry = (TSK_ENTRY_FUNC)iot_thread;
    task_3.uwStackSize = 20480*5;
    task_3.pcName = "iot thread";
    task_3.usTaskPrio = 24;
    ret = LOS_TaskCreate(&thread_id_3, &task_3);
    if (ret != LOS_OK)
    {
        printf("Falied to create task ret:0x%x\n", ret);
        return;
    }

}
APP_FEATURE_INIT(iot_smart_home_example);
