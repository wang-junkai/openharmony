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

#include "smart_home.h"

#include <stdio.h>
#include <stdbool.h>

#include "iot_errno.h"

#include "iot_pwm.h"
#include "iot_gpio.h"
#include "su_03t.h"
#include "iot.h"
#include "lcd.h"
#include "picture.h"
#include "adc_key.h"
#include "components.h"
#include "lcd.h"
#include "string.h"

#include "mq2.h"



static bool auto_state = false;
static bool network_state = false;

#define BEEP_PORT EPWMDEV_PWM5_M0
#define GPIO_BODY_INDUCTION GPIO0_PA3
 #define ppm_range 40.0
#define GPIO_ALARM_LIGHT 12  
 //#define SMOKE_THRESHOLD 40.0  // 当浓度超过40PPM时触发报警

void light_menu_entry(lcd_menu_t *menu);
void fan_menu_entry(lcd_menu_t *menu);

/* 风扇菜单的数据初始化*/
lcd_menu_t fan_menu={
    .img={
        .img=img_fan_off,
        .height=64,
        .width=64,
    },
    .is_selected=false,
    .text={
        .fc=LCD_MAGENTA,
        .bc=LCD_WHITE,
        .font_size=24,
        .name="风扇关",
    },
    .enterFunc=fan_menu_entry,
    .exitFunc=NULL,
    .base_x=100,
    .base_y=64,
 
};

/* 照明灯的菜单初始化数据*/
lcd_menu_t light_menu={
    .img={
        .img= img_light_off,
        .height=64,
        .width=64,
    },
    .is_selected=false,
    .text={
        .fc=LCD_MAGENTA,
        .bc=LCD_WHITE,
        .font_size=24,
        .name="灯光关",
    },
    .base_x=20,
    .base_y=64,
    .enterFunc=light_menu_entry,
    .exitFunc=NULL,
};

/* 温度面板的初始化数据*/
lcd_display_board_t temp_db={
    .img={
        .img= img_temp_normal,
        .height=48,
        .width=48,
    },
    .text={
        .fc=LCD_MAGENTA,
        .bc=LCD_WHITE,
        .font_size=24,
        .name="25.5°C",
    },
    .base_x=180,
    .base_y=64,
};

/* 湿度面板的初始化数据*/
lcd_display_board_t humi_db={
    .img={
        .img= img_humi,
        .height=48,
        .width=48,
    },
    .text={
        .fc=LCD_MAGENTA,
        .bc=LCD_WHITE,
        .font_size=24,
        .name="25.5°C",
    },
    .base_x=180,
    .base_y=120,
};

/* 亮度面板的初始化数据*/
lcd_display_board_t lum_db={
    .img={
        .img= img_lum,
        .height=48,
        .width=48,
    },
    .text={
        .fc=LCD_MAGENTA,
        .bc=LCD_WHITE,
        .font_size=24,
        .name="1234Lx",
    },
    .base_x=176,
    .base_y=174,
};
lcd_display_board_t gas_db={
    .img={
        .img= gImage_blue,
        .height=32,
        .width=32,
    },
    .text={
        .fc=LCD_MAGENTA,
        .bc=LCD_WHITE,
        .font_size=24,
        .name="20.00ppm",
    },
    .base_x=175,
    .base_y=30,
};


/* 所有的面板集合数组,方便遍历查询*/
lcd_display_board_t *lcd_dbs[] ={&temp_db,&humi_db,&lum_db,&gas_db};
/* 所有的菜单集合数组,方便遍历查询*/
lcd_menu_t *lcd_menus[] = {&light_menu,&fan_menu};
/* 菜单的个数*/
static int lcd_menu_number =  sizeof(lcd_menus)/sizeof(lcd_menu_t *);
/* 菜单的当前选中索引,在数组中的位置*/
static int menu_select_index = 0;

//菜单左和右的处理,需要考虑菜单个数的边界
void lcd_menu_selected_move_left()
{
    if(menu_select_index > 0){
        menu_select_index--;
    }
}

void lcd_menu_selected_move_right()
{
    if(menu_select_index <lcd_menu_number-1){
        menu_select_index++;
    }
}

/**
 * @brief 照明灯菜单按下确认按键
 * 
 * @param menu 
 */
void light_menu_entry(lcd_menu_t *menu)
{
    int light_state = get_light_state();
    if(light_state)
    {
        light_set_state(false);
        lcd_set_light_state(false);
    }else{
        light_set_state(true);
        lcd_set_light_state(true);
    }
}
/**
 * @brief  风扇菜单按下确认按键
 * 
 * @param menu 
 */
void fan_menu_entry(lcd_menu_t *menu)
{
   int motor_state = get_motor_state();
   if(motor_state)
   {
       motor_set_state(false);
       lcd_set_motor_state(false);
   }else{
       motor_set_state(true);
       lcd_set_motor_state(true);
   }
}


/***************************************************************
* 函数名称: lcd_dev_init
* 说    明: lcd初始化
* 参    数: 无
* 返 回 值: 无
***************************************************************/
void lcd_dev_init(void)
{
    lcd_init();
    lcd_fill(0, 0, LCD_W, LCD_H, LCD_WHITE);
}

/**
 * @brief 按键处理函数
 * 
 * @param key_no 按键号
 */
void smart_home_key_process(int key_no)
{
    printf("smart_home_key_process:%d\n",key_no);
    if(key_no == KEY_UP){

    }else if(key_no == KEY_DOWN){
        lcd_menu_entry(lcd_menus[menu_select_index]);

    }else if(key_no == KEY_LEFT){
        
        lcd_menu_selected_move_left();
    }else if(key_no == KEY_RIGHT){
        lcd_menu_selected_move_right();
    }
}
/**
 * @brief 物联网的指令处理函数
 * 
 * @param iot_cmd iot的指令
 */
void smart_home_iot_cmd_process(int iot_cmd)
{
    switch (iot_cmd)
    {
        case IOT_CMD_LIGHT_ON:
            light_set_state(true);
            lcd_set_light_state(true);
            break;
        case IOT_CMD_LIGHT_OFF:
            light_set_state(false);
            lcd_set_light_state(false);
            break;
        case IOT_CMD_MOTOR_ON:
            motor_set_state(true);
            lcd_set_motor_state(true);
            break;
        case IOT_CMD_MOTOR_OFF:
            motor_set_state(false);
            lcd_set_motor_state(false);
            break;
    }
}

/**
 * @brief 语音管家发出的指令
 * 
 * @param su03t_cmd 语音管家的指令
 */
void smart_home_su03t_cmd_process(int su03t_cmd)
{
    switch (su03t_cmd)
    {
        case light_state_on:
            light_set_state(true);
            lcd_set_light_state(true);
            break;
        case light_state_off:
            light_set_state(false);
            lcd_set_light_state(false);
            break;
        case motor_state_on:
            motor_set_state(true);
            lcd_set_motor_state(true);
            break;
        case motor_state_off:
            motor_set_state(false);
            lcd_set_motor_state(false);
            break;
        case temperature_get:
        {
            double temp,humi;

            sht30_read_data(&temp, &humi);
            su03t_send_double_msg(1, temp);
        }
            
            break;
        case humidity_get:
        {
            double temp,humi;
            sht30_read_data(&temp, &humi);

            su03t_send_double_msg(2, humi);
        }
            break;
        case illumination_get:
        {
            double lum;
            bh1750_read_data(&lum);
            su03t_send_double_msg(3, lum);
        }
            
            break;
       
        default:
            break;
    }
}


/***************************************************************
* 函数名称: lcd_load_ui
* 说    明: 加载lcd ui
* 参    数: 无
* 返 回 值: 无
***************************************************************/
void lcd_show_ui(void)
{

    lcd_show_chinese(74, 0, "智能宠物管家", LCD_RED, LCD_WHITE, 32, 0);
    lcd_show_picture(280,0, 32,32, network_state? img_wifi_on : img_wifi_off);

    lcd_menu_update(lcd_menus ,lcd_menu_number,menu_select_index);

    lcd_menu_show(lcd_menus ,  lcd_menu_number);
   lcd_db_show(lcd_dbs, sizeof(lcd_dbs)/sizeof(lcd_display_board_t *)); 

    lcd_show_picture(175,30,32,32, gImage_blue);
    lcd_show_picture(10,170,100,60, gImage_have1);
    lcd_show_picture(1,1,70,70, gImage_home);

    
}

/***************************************************************
 * 函数名称: lcd_set_ppm
 * 说    明: lcd设置烟雾浓度显示
 * 参    数: 无
 * 返 回 值: 无
 ***************************************************************/
void lcd_set_ppm(double ppm)
{
   
 sprintf(gas_db.text.name, "%.2fppm",ppm);
    

    //lcd_show_string(195, 40, buf, LCD_MAGENTA, LCD_WHITE, 24, 0);
}

/***************************************************************
* 函数名称: lcd_set_temperature
* 说    明: 设置温度显示
* 参    数: double temperature 温度
* 返 回 值: 无
***************************************************************/
void lcd_set_temperature(double temperature)
{
    sprintf(temp_db.text.name, "%.01f℃ ", temperature);
    /* 对温度做高温和正常的区分*/
    if(temperature > 35)
    {
        temp_db.text.fc = LCD_RED;
        temp_db.img.img = img_temp_high;
    }
    else
    {
       temp_db.text.fc = LCD_MAGENTA;
       temp_db.img.img = img_temp_normal;
    }
}

/***************************************************************
* 函数名称: lcd_set_humidity
* 说    明: 设置湿度显示
* 参    数: double humidity 湿度
* 返 回 值: 无
***************************************************************/
void lcd_set_humidity(double humidity)
{
    sprintf(humi_db.text.name, "%.01f%% ", humidity);

}

/***************************************************************
* 函数名称: lcd_set_illumination
* 说    明: 设置光照强度显示
* 参    数: double illumination 光照强度
* 返 回 值: 无
***************************************************************/
void lcd_set_illumination(double illumination)
{
    sprintf(lum_db.text.name, "%.01fLx ", illumination);

}

void lcd_set_network_state(int state){
    network_state = state;
}

/***************************************************************
* 函数名称: lcd_set_light_state
* 说    明: 设置灯状态显示
* 参    数: bool state true：显示"打开" false：显示"关闭"
* 返 回 值: 无
***************************************************************/
void lcd_set_light_state(bool state)
{
    
    strcpy(light_menu.text.name,state? "灯光开" :"灯光关");
    light_menu.img.img = state? img_light_on : img_light_off;
}

/***************************************************************
* 函数名称: lcd_set_motor_state
* 说    明: 设置电机状态显示
* 参    数: bool state true：显示"打开" false：显示"关闭"
* 返 回 值: 无
***************************************************************/
void lcd_set_motor_state(bool state)
{

    strcpy(fan_menu.text.name,state? "风扇开" :"风扇关");
    fan_menu.img.img = state? img_fan_on : img_fan_off;

}

/***************************************************************
 * 函数名称: body_induction_get_state
 * 说    明: 获取人体感应状态
 * 参    数: 无
 * 返 回 值: 无
 ***************************************************************/
void body_induction_get_state(bool *dat)
{
    IotGpioValue value = IOT_GPIO_VALUE0;

    IoTGpioGetInputVal(GPIO_BODY_INDUCTION, &value);

    if (value) 
    {
        *dat = true;
    }
    else
    {
        *dat = false;
    }
}

/***************************************************************
 * 函数名称: body_induction_dev_init
 * 说    明: 人体感应传感器初始化
 * 参    数: 无
 * 返 回 值: 无
 ***************************************************************/
void body_induction_dev_init(void)
{
    IoTGpioInit(GPIO_BODY_INDUCTION);
    IoTGpioSetDir(GPIO_BODY_INDUCTION, IOT_GPIO_DIR_IN);
}
/***************************************************************
 * 函数名称: beep_dev_init
 * 说    明: 蜂鸣器初始化
 * 参    数: 无
 * 返 回 值: 无
 ***************************************************************/
void beep_dev_init(void)
{
    IoTPwmInit(BEEP_PORT);
}

/***************************************************************
 * 函数名称: beep_set_pwm
 * 说    明: 设置蜂鸣器PWM
 * 参    数: 无
 * 返 回 值: 无
 ***************************************************************/
void beep_set_pwm(unsigned int duty)
{
    IoTPwmStart(BEEP_PORT, duty, 1000);
}

/***************************************************************
 * 函数名称: beep_set_state
 * 说    明: 设置蜂鸣器状态
 * 参    数: 无
 * 返 回 值: 无
 ***************************************************************/
void beep_set_state(bool state)
{
    static bool last_state = false;

    if (state == last_state)
    {
        return;
    }

    if (state)
    {
        beep_set_pwm(20);
    }
    else
    {
        beep_set_pwm(1);
        IoTPwmStop(BEEP_PORT);
    }

    last_state = state;
}



void lcd_set_auto_state(bool state)
{



    // if (state)
    // {
    //     lcd_show_chinese(77, 204, "开启", LCD_RED, LCD_WHITE, 24, 0);
    // }
    // else
    // {
    //     lcd_show_chinese(77, 204, "关闭", LCD_RED, LCD_WHITE, 24, 0);
    // 
}
/***************************************************************
 * 函数名称: lcd_set_body_induction
 * 说    明: lcd设置人体感应显示
 * 参    数: 无
 * 返 回 值: 无
 ***************************************************************/
void lcd_set_body_induction(bool body_induction)
{
    if (body_induction)
    {
        lcd_show_picture(10,170,100,60, gImage_Yhave);
    }
    else
    {
        lcd_show_picture(10,170,100,60, gImage_have1);
    }
}
/***************************************************************
 * 函数名称: alarm_light_init
 * 说    明: 报警灯初始化
 * 参    数: 无
 * 返 回 值: 无
 ***************************************************************/
void alarm_light_init(void)
{
    IoTGpioInit(GPIO_ALARM_LIGHT);
    IoTGpioSetDir(GPIO_ALARM_LIGHT, IOT_GPIO_DIR_OUT);

    IoTGpioSetOutputVal(GPIO_ALARM_LIGHT, IOT_GPIO_VALUE0); 
}

/***************************************************************
 * 函数名称: alarm_light_set_gpio
 * 说    明: 设置报警灯
 * 参    数: 无
 * 返 回 值: 无
 ***************************************************************/
void alarm_light_set_gpio(bool state)
{
    static bool last_state = false;

    if (state == last_state)
    {
        return;
    }

    if (state)
    {
        IoTGpioSetOutputVal(GPIO_ALARM_LIGHT, IOT_GPIO_VALUE1);
    }
    else
    {
        IoTGpioSetOutputVal(GPIO_ALARM_LIGHT, IOT_GPIO_VALUE0);
    }

    last_state = state;
}
/***************************************************************
 * 函数名称: mq2_init
 * 说    明: mq2初始化
 * 参    数: 无
 * 返 回 值: 无
 ***************************************************************/
void mq2_init(void)
{
    mq2_dev_init();
    mq2_ppm_calibration();
}

/***************************************************************
 * 函数名称: mq2_read_data
 * 说    明: 读取mq2传感器数据
 * 参    数: 无
 * 返 回 值: 无
 ***************************************************************/
void mq2_read_data(double *dat)
{
    *dat = get_mq2_ppm();

}
/***************************************************************
 * 函数名称: lcd_set_gas_alarm
 * 说    明: 根据可燃气体浓度设置LCD显示图片
 * 参    数: 无(直接读取全局变量或传感器数据)
 * 返 回 值: 无
 ***************************************************************/
void lcd_set_mq2_alarm(void)
{
    // 定义可燃气体浓度阈值(单位:PPM)
    #define ppm_range 800.0  // 当浓度超过40PPM时显示报警图片
    
    // 获取当前可燃气体浓度
    static double mq2_ppm = 0.0;
    mq2_read_data(&mq2_ppm);  // 读取最新浓度值
    
    // 判断浓度是否超过阈值
    if (mq2_ppm > ppm_range)
    {
        // 浓度超标，显示报警图片
        lcd_show_picture(180, 30, 36, 36, gImage_red);
    }
    else
    {
        // 浓度正常，显示正常图片
        lcd_show_picture(180, 30, 36, 36, gImage_blue);
    }
}