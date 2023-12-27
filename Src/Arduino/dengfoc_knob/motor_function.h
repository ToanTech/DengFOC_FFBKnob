#include <Arduino.h>

#ifndef MOTOR_FUNCTION_H
#define MOTOR_FUNCTION_H

#define BUTTON1 4 // 16
#define BUTTON2 -1
#define BUTTON3 -1

#define _2PI 6.28318530718f

/************按键函数声明*****************/
void Mode_Change();           // 旋钮功能模式切换
void attachClick();           // 单击
void attachDoubleClick();     // 双击
void attachLongPressStart();  // 长按开始
void attachDuringLongPress(); // 长按过程
void attachLongPressStop();   // 长按结束
void attachMultiClick();      // 打印按下多次次数
void button_event_init();     // 按键事件初始化
void button_attach_loop();    // 按钮检测状态子程序
void button_vibrate();        // 单击震动
void knob_init();             // 按键重置零角度

/**************旋钮功能函数声明***************/
void Smooth_Boundary_Mode(float angle_rang);  // 有界——平滑模式
void Detents_Mode();                          // 棘轮模式
void Fine_Detents_Mode();                     // 细棘轮模式
void Coarse_Detents_Mode();                   // 粗棘轮模式
void Weak_Detents_Mode();                     // 弱力棘轮模式
void Strong_Detents_Mode();                   // 强力棘轮模式
void Detents_Boundary_Mode(float angle_rang); // 有界——棘轮模式
void Switch_Mode(float angle_rang);           // 棘轮开关
void Current_Detents_Mode();                  // 阻尼模式
void Current_NO_Detents_Mode();               // 无阻尼-顺滑模式

void my_button_tick();
void RGB_init();
void blekeyboard();
void BleKeyboard_begin();
void update_angle_speed();

extern int last_state_RGB;
extern int state;

#endif
