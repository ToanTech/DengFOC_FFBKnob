//例程：蓝牙控制电脑

// DengFOC V0.2
// 灯哥开源，遵循GNU协议，转载请著名版权！
// GNU开源协议（GNU General Public License, GPL）是一种自由软件许可协议，保障用户能够自由地使用、研究、分享和修改软件。
// 该协议的主要特点是，要求任何修改或衍生的作品必须以相同的方式公开发布，即必须开源。此外，该协议也要求在使用或分发软件时，必须保留版权信息和许可协议。GNU开源协议是自由软件基金会（FSF）制定和维护的一种协议，常用于GNU计划的软件和其他自由软件中。
// 仅在DengFOC官方硬件上测试过，欢迎硬件购买/支持作者，淘宝搜索店铺：灯哥开源
// 你的支持将是接下来做视频和持续开源的经费，灯哥在这里先谢谢大家了

#include "DengFOC.h"
#include "BleConnectionStatus.h"
#include "OneButton.h"
#include "BleKeyboard.h"

int Sensor_DIR = 1;  // 传感器方向，若电机运动不正常，将此值取反
int Motor_PP = 7;    // 电机极对数

/******************************************************/
bool botton_Click = false;
bool Keyboard_Click = false;
/******************************************************/
BleKeyboard bleKeyboard;          //蓝牙键盘初始化
OneButton button(4, true, true);  // 低电平触发
/******************************************************/
void setup() {
  Serial.begin(115200);
  pinMode(12, OUTPUT);
  digitalWrite(12, HIGH);  // 使能，一定要放在校准电机前

  DFOC_Vbus(12.6);  // 设定驱动器供电电压
  DFOC_alignSensor(Motor_PP, Sensor_DIR);

  bleKeyboard.begin();  //蓝牙键盘初始
  button_event_init();  // 按钮事件初始化
}
void loop() {
  runFOC();
  float kp2 = 0.052;  //顺滑模式P值
  DFOC_M0_setTorque(kp2 * DFOC_M0_Velocity());

  blekeyboard();  //转盘转动
  button.tick();  //按下检测
}
/************************按键控制************************/
// 单击
void attachClick() {
  Serial.println("click-单击");
  botton_Click = true;

  if (Keyboard_Click) {
    bleKeyboard.pressDial();
    bleKeyboard.releaseDial();
    Keyboard_Click = false;
  }
}
// 长铵开始
void attachLongPressStart() {
  bleKeyboard.pressDial();
  Serial.println("longPressStart-长按开始");
  Keyboard_Click = true;  // 电脑调出转盘标志位
}
// 长按结束
void attachLongPressStop() {
  bleKeyboard.releaseDial();
  Serial.println("longPressStop-长按结束");
}
// 按键事件初始化
void button_event_init() {
  button.reset();               // 清除一下按钮状态机的状态
  button.setDebounceTicks(80);  // 设置消抖时长为80毫秒,默认值为：50毫秒
  button.setClickTicks(400);    // 设置单击时长为500毫秒,默认值为：400毫秒
  button.setPressTicks(1000);   // 设置长按时长为1000毫秒,默认值为：800毫秒

  button.attachClick(attachClick);                    // 初始化单击回调函数
  button.attachLongPressStart(attachLongPressStart);  // 初始化长按开始回调函数
  button.attachLongPressStop(attachLongPressStop);    // 初始化长按结束回调函数
}
/************************蓝牙键盘************************/
//连上蓝牙时，检测角度并转动转盘
float current_angle, last_angle;
void blekeyboard() {
  current_angle = round(DFOC_M0_Angle());
  if (bleKeyboard.isConnected())  // if BLE Connected
  {
    if (current_angle > last_angle)
      bleKeyboard.rotate(1);
    else if (current_angle < last_angle)
      bleKeyboard.rotate(-1);
  }
  last_angle = current_angle;
}