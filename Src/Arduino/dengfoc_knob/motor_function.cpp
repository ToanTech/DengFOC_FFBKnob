#include "DengFOC.h"
#include "motor_function.h"
#include "OneButton.h"
#include "BleKeyboard.h"
#include "DFOC_RGB.h"

bool click_flag = false;
bool clickChangeMode = false;
bool doubleClickChangeMode = false;
bool bleKeyboardClick = false;

float zero_angle = 0;
int time_flag = 0;

float pre_angle_RGB = 0, pre_speed_RGB = 0;
uint8_t Switch_Mode_RGB = 0;

BleKeyboard bleKeyboard;
OneButton button(BUTTON1, true, true); // 低电平触发
DFOC_RGB RGB = DFOC_RGB();

/************************RGB************************/
void RGB_init()
{
    RGB.begin(); // 程序初始化
}

/************************按键状态切换逻辑************************/
typedef enum
{
    LOCK_STATE = 0,

    FINE_RATCHET,
    MID_RATCHET,
    ROUGH_RATCHET,
    Weak_Detents_RATCHET,
    Strong_Detents_RATCHET,

    SWITCH_RATCHET,
    SCOPED_RATCHET,

    FINE_DAMP,
    ROUGH_DAMP,
    Smooth_Boundary,

} KEY_STATE;

int last_state, state = LOCK_STATE;
int last_state_RGB;

static int angle_rest = 0, sta = 0;

void Mode_Change()
{
    update_angle_speed();

    switch (state)
    {
    case LOCK_STATE:
        // print
        if (time_flag)
            Serial.println("LOCK_STATE");
        // RGB
        RGB.horse_race_lamp(10, 120, 1);
        // motor
        DFOC_M0_set_Force_Angle(zero_angle);

        if (clickChangeMode)
        {
            state = LOCK_STATE;

            clickChangeMode = false;
        }
        else if (doubleClickChangeMode)
        {
            state = FINE_RATCHET;

            doubleClickChangeMode = false;
        }
        break;

    case FINE_RATCHET:
        // print
        if (time_flag)
            Serial.println("细棘轮模式");

        // RGB
        RGB.blink(3, 120, 0, 255, 0); // 双闪
        if (blink_end == 1)
            RGB.brighness_Adjustable(60, pre_angle_RGB);

        // motor
        Fine_Detents_Mode();

        if (clickChangeMode)
        {
            state = MID_RATCHET;

            clickChangeMode = false;
        }
        else if (doubleClickChangeMode)
        {
            state = SWITCH_RATCHET;

            doubleClickChangeMode = false;
        }
        break;

    case MID_RATCHET:
        // print
        if (time_flag)
            Serial.println("中棘轮模式");

        // RGB
        RGB.blink(2, 120, 0, 255, 0); // 单闪
        if (blink_end == 1)
            RGB.Alternate_light_up(20, pre_angle_RGB, 120);

        // motor
        Detents_Mode();

        if (clickChangeMode)
        {
            state = ROUGH_RATCHET;

            clickChangeMode = false;
        }
        else if (doubleClickChangeMode)
        {
            state = SWITCH_RATCHET;

            doubleClickChangeMode = false;
        }
        break;

    case ROUGH_RATCHET:
        // print
        if (time_flag)
            Serial.println("粗棘轮模式");

        // RGB
        RGB.blink(2, 120, 0, 255, 0); // 单闪
        if (blink_end == 1)
        {
            // 粗棘轮模式下，转动一格，高亮一个灯，10档
            // sta为更新电机状态，进入此效果时，清除上个状态下灯的灯效，让所有灯灭掉
            if (sta == 0)
            {
                RGB.setAllcolor(0, 0, 0, 0);
                sta = 1;
            }
            RGB.Tail_lamp(RGB.update_i(10, pre_angle_RGB, 10), 120);
        }

        // motor
        Coarse_Detents_Mode();

        if (clickChangeMode)
        {
            state = Weak_Detents_RATCHET;

            clickChangeMode = false;
        }
        else if (doubleClickChangeMode)
        {
            state = SWITCH_RATCHET;

            doubleClickChangeMode = false;
        }
        break;

    case Weak_Detents_RATCHET:
        // print
        if (time_flag)
            Serial.println("弱力中棘轮模式");

        // RGB
        RGB.blink(2, 120, 0, 255, 0); // 单闪
        if (blink_end == 1)
            RGB.breath(5, 10, 0, 255, 0);

        // motor
        Weak_Detents_Mode();

        if (clickChangeMode)
        {
            state = Strong_Detents_RATCHET;

            clickChangeMode = false;
        }
        else if (doubleClickChangeMode)
        {
            state = SWITCH_RATCHET;

            doubleClickChangeMode = false;
        }
        break;

    case Strong_Detents_RATCHET:
        // print
        if (time_flag)
            Serial.println("强力中棘轮模式");

        // RGB
        RGB.blink(2, 120, 0, 255, 0); // 单闪
        if (blink_end == 1)
            RGB.breath(5, 10, 0, 255, 0);

        // motor
        Strong_Detents_Mode();

        if (clickChangeMode)
        {
            state = FINE_RATCHET;

            clickChangeMode = false;
        }
        else if (doubleClickChangeMode)
        {
            state = SWITCH_RATCHET;

            doubleClickChangeMode = false;
        }
        break;

    case SWITCH_RATCHET:
        // print
        if (time_flag)
            Serial.println("有界——棘轮模式");

        // RGB
        RGB.blink(3, 120, 0, 0, 255); // 双闪
        if (blink_end == 1)
            RGB.breath(5, 10, 0, 0, 255);

        // motor
        Detents_Boundary_Mode(120);

        if (clickChangeMode)
        {
            state = SCOPED_RATCHET;

            clickChangeMode = false;
        }
        else if (doubleClickChangeMode)
        {
            state = FINE_DAMP;

            doubleClickChangeMode = false;
        }
        break;

    case SCOPED_RATCHET:
        // print
        if (time_flag)
            Serial.println("棘轮开关");

        // RGB
        RGB.blink(2, 120, 0, 0, 255); // 单闪
        if (blink_end == 1)
        {
            if (Switch_Mode_RGB == 1 || Switch_Mode_RGB == 3)
                RGB.setAllcolor(150, 204, 0, 204);
            else if (Switch_Mode_RGB == 2 || Switch_Mode_RGB == 4)
                RGB.setAllcolor(150, 102, 102, 0);
        }

        // motor
        Switch_Mode(100);

        if (clickChangeMode)
        {
            state = SWITCH_RATCHET;

            clickChangeMode = false;
        }
        else if (doubleClickChangeMode)
        {
            state = FINE_DAMP;

            doubleClickChangeMode = false;
        }
        break;

    case FINE_DAMP:
        // print
        if (time_flag)
            Serial.println("无阻尼——顺滑模式");

        // RGB
        RGB.blink(3, 120, 255, 0, 0); // 双闪
        if (blink_end == 1)
            RGB.bri_Adjustable_horse_race_lamp(pre_speed_RGB);

        // motor
        Current_NO_Detents_Mode();

        if (clickChangeMode)
        {
            state = ROUGH_DAMP;

            clickChangeMode = false;
        }
        else if (doubleClickChangeMode)
        {
            state = FINE_RATCHET;

            doubleClickChangeMode = false;
        }
        break;

    case ROUGH_DAMP:
        // print
        if (time_flag)
            Serial.println("微阻尼模式");

        // RGB
        RGB.blink(2, 120, 255, 0, 0); // 单闪
        if (blink_end == 1)
            RGB.change_dir_horse_race_lamp(pre_speed_RGB, 120);

        // motor
        Current_Detents_Mode();

        if (clickChangeMode)
        {
            state = Smooth_Boundary;

            clickChangeMode = false;
        }
        else if (doubleClickChangeMode)
        {
            state = FINE_RATCHET;

            doubleClickChangeMode = false;
        }
        break;

    case Smooth_Boundary:
        // print
        if (time_flag)
            Serial.println("有界-平滑模式");

        // RGB
        RGB.blink(2, 120, 255, 0, 0); // 单闪
        if (blink_end == 1)
            RGB.breath(5, 10, 255, 0, 0);

        // motor
        Smooth_Boundary_Mode(120);

        if (clickChangeMode)
        {
            state = FINE_DAMP;

            clickChangeMode = false;
        }
        else if (doubleClickChangeMode)
        {
            state = FINE_RATCHET;

            doubleClickChangeMode = false;
        }
        break;

    default:
        break;
    }
}

/************************按键控制************************/
void my_button_tick()
{
    button.tick();
}

// 单击
void attachClick()
{
    Serial.println("click-单击");
    click_flag = true;

    if (bleKeyboardClick)
    {
        bleKeyboard.pressDial();
        bleKeyboard.releaseDial();
        bleKeyboardClick = false;
    }
    else
        clickChangeMode = true;
}

// 双击
void attachDoubleClick()
{
    Serial.println("doubleclick-双击切换");
    doubleClickChangeMode = true;
}

// 长铵开始
void attachLongPressStart()
{
    bleKeyboard.pressDial();
    Serial.println("longPressStart-长按开始");
    bleKeyboardClick = true;
}

// 长按结束
void attachLongPressStop()
{
    bleKeyboard.releaseDial();
    Serial.println("longPressStop-长按结束");
}

// 长按过程
void attachDuringLongPress()
{
    if (button.isLongPressed())
    {
        Serial.println("duringLongPress-长按期间");
    }
}

// 连击
void attachMultiClick()
{
    Serial.printf("您已成功连击：%d次!!!!!!!!\r\n", button.getNumberClicks());
}

// 单击震动
void button_vibrate()
{
    if (click_flag)
    {
        DFOC_M0_setTorque(0.3);
        for (uint8_t i = 0; i < 3; i++)
        {
            runFOC();
            delay(1);
        }
        DFOC_M0_setTorque(-0.3);
        for (uint8_t i = 0; i < 3; i++)
        {
            runFOC();
            delay(1);
        }
        DFOC_M0_setTorque(0);
        click_flag = false;
    }
}

// 按下检测
void button_attach_loop()
{
    button.tick();
}

// 按键事件初始化
void button_event_init()
{
    button.reset();              // 清除一下按钮状态机的状态
    button.setDebounceTicks(80); // 设置消抖时长为80毫秒,默认值为：50毫秒
    button.setClickTicks(400);   // 设置单击时长为500毫秒,默认值为：400毫秒
    button.setPressTicks(1000);  // 设置长按时长为1000毫秒,默认值为：800毫秒

    button.attachClick(attachClick);                     // 初始化单击回调函数
    button.attachDoubleClick(attachDoubleClick);         // 初始化双击回调函数
    button.attachLongPressStart(attachLongPressStart);   // 初始化长按开始回调函数
    button.attachDuringLongPress(attachDuringLongPress); // 初始化长按期间回调函数
    button.attachLongPressStop(attachLongPressStop);     // 初始化长按结束回调函数
    button.attachMultiClick(attachMultiClick);           // 初始化按了多次(3次或以上)回调函数
}

/************************KeyBorad************************/
//  固定时间打印间隔
unsigned long prev_time = 0, now_time = 0;
float angle_last, angle_last2;

void BleKeyboard_begin()
{
    bleKeyboard.begin();
}

void blekeyboard()
{
    // 不断检测按钮按下状态
    now_time = millis();

    /*按键*/
    button_attach_loop();
    button_vibrate();

    /*蓝牙*/
    angle_last = round(DFOC_M0_Angle());
    if (bleKeyboard.isConnected()) // if BLE Connected
    {
        if (time_flag)
            Serial.println("连接成功！！！");
        // button.tick();
        my_button_tick();
        if (angle_last > angle_last2)
            bleKeyboard.rotate(1);
        else if (angle_last < angle_last2)
            bleKeyboard.rotate(-1);
    }
    angle_last2 = angle_last;

    if (now_time - prev_time > 1000)
    {
        prev_time = now_time;
        time_flag = 1;
    }
    else
        time_flag = 0;
}

void knob_init()
{
    zero_angle = DFOC_M0_Angle();
}

void update_angle_speed()
{

    if (state != last_state)
    {
        angle_rest = 0;
        blink_end = 0; // 闪烁标志位清零
        sta = 0;       // 粗棘轮清除上次模式下灯的状态
    }
    last_state = state;
    pre_speed_RGB = DFOC_M0_Velocity();                // 更新速度
    pre_angle_RGB = DFOC_M0_Angle() * 180 / 3.1415926; // 更新角度，角度制
}

/************************电机模式************************/
// 除Current_NO_Detents_Mode外，都是基于电压环
// 已在DengFOC.h中修改为电压模式，电流力矩模式加多了条函数接口

// 细棘轮模式
void Fine_Detents_Mode()
{
    static bool isFirstTime = true;
    float attractor_distance = 6 * 3.1415926 / 180.0;
    float target = round(DFOC_M0_Angle() / attractor_distance) * attractor_distance;
    DFOC_M0_setTorque(20 * (target - DFOC_M0_Angle()));
}

// 中棘轮模式
void Detents_Mode()
{
    float attractor_distance = 20 * 3.1415926 / 180.0; // dimp each 45 degrees调光每45度
    float target = round(DFOC_M0_Angle() / attractor_distance) * attractor_distance;
    DFOC_M0_setTorque(20 * (target - DFOC_M0_Angle()));
}

// 粗棘轮模式
void Coarse_Detents_Mode()
{
    float attractor_distance = 36 * 3.1415926 / 180.0;
    float target = round(DFOC_M0_Angle() / attractor_distance) * attractor_distance;
    DFOC_M0_setTorque(20 * (target - DFOC_M0_Angle()));
}

// 弱力棘轮模式
void Weak_Detents_Mode()
{
    float attractor_distance = 15 * 3.1415926 / 180.0;
    float target = round(DFOC_M0_Angle() / attractor_distance) * attractor_distance;
    DFOC_M0_setTorque(12 * (target - DFOC_M0_Angle()));
}

// 强力棘轮模式
void Strong_Detents_Mode()
{
    float attractor_distance = 15 * 3.1415926 / 180.0;
    float target = round(DFOC_M0_Angle() / attractor_distance) * attractor_distance;
    DFOC_M0_setTorque(25 * (target - DFOC_M0_Angle()));
}

// 有界——棘轮模式
float kp_pos2 = 8; 
void Detents_Boundary_Mode(float angle_rang)
{
    if (angle_rest == 0)
    {
        zero_angle = DFOC_M0_Angle();
        angle_rest = 1;
    }

    float angle = DFOC_M0_Angle();
    float limit_rang = angle_rang / 360.0f / 2 * _2PI;
    if (angle > zero_angle - limit_rang && angle < zero_angle + limit_rang)
        Detents_Mode();
    else
    {
        if (angle < zero_angle - limit_rang)
            DFOC_M0_setTorque(kp_pos2 * (zero_angle - limit_rang - angle));
        if (angle > zero_angle + limit_rang)
            DFOC_M0_setTorque(kp_pos2 * (zero_angle + limit_rang - angle));
    }
}

// 棘轮开关
void Switch_Mode(float angle_rang)
{
    float angle = DFOC_M0_Angle();
    float limit_rang = angle_rang / 360.0f / 2 * _2PI; // 转成弧度制
    if (angle > zero_angle - limit_rang && angle < zero_angle + limit_rang)
    {
        if (angle < zero_angle)
        {
            DFOC_M0_setTorque(10 * (zero_angle - limit_rang - angle));
            Switch_Mode_RGB = 1;
        }
        if (angle > zero_angle)
        {
            DFOC_M0_setTorque(10 * (zero_angle + limit_rang - angle));
            Switch_Mode_RGB = 2;
        }
    }
    else
    {
        if (angle < zero_angle - limit_rang)
        {
            DFOC_M0_setTorque(15 * (zero_angle - limit_rang - angle));
            Switch_Mode_RGB = 3;
        }
        if (angle > zero_angle + limit_rang)
        {
            DFOC_M0_setTorque(15 * (zero_angle + limit_rang - angle));
            Switch_Mode_RGB = 4;
        }
    }
}

// 微阻尼模式
void Current_Detents_Mode()
{
    DFOC_M0_setTorque(0); // 电压力矩
}

// 无阻尼-顺滑模式
void Current_NO_Detents_Mode()
{
    DFOC_M0_SET_CURRENT_PID(5, 200, 0, 100000);
    DFOC_M0_setTorque_current(0); // 电流环
}

// 有界——微阻尼模式
float kp_pos1 = 0.5;
void Smooth_Boundary_Mode(float angle_rang)
{
    if (angle_rest == 0)
    {
        zero_angle = DFOC_M0_Angle();
        angle_rest = 1;
    }
    float angle = DFOC_M0_Angle();
    float limit_rang = angle_rang / 360.0f / 2 * _2PI;
    if (angle > zero_angle - limit_rang && angle < zero_angle + limit_rang)
        DFOC_M0_setTorque(0);
    else
    {
        if (angle < zero_angle - limit_rang)
            DFOC_M0_setTorque(kp_pos1 * (zero_angle - limit_rang - angle));
        if (angle > zero_angle + limit_rang)
            DFOC_M0_setTorque(kp_pos1 * (zero_angle + limit_rang - angle));
    }
}