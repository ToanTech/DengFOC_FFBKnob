//  例程3：棘轮位置的指示灯
//  DengFOC V0.2
//  灯哥开源，遵循GNU协议，转载请著名版权！
//  GNU开源协议（GNU General Public License, GPL）是一种自由软件许可协议，保障用户能够自由地使用、研究、分享和修改软件。
//  该协议的主要特点是，要求任何修改或衍生的作品必须以相同的方式公开发布，即必须开源。此外，该协议也要求在使用或分发软件时，必须保留版权信息和许可协议。GNU开源协议是自由软件基金会（FSF）制定和维护的一种协议，常用于GNU计划的软件和其他自由软件中。
//  仅在DengFOC官方硬件上测试过，欢迎硬件购买/支持作者，淘宝搜索店铺：灯哥开源
//  你的支持将是接下来做视频和持续开源的经费，灯哥在这里先谢谢大家了

#include "DengFOC.h"
#include "DFOC_RGB.h"

int Sensor_DIR = 1;  // 传感器方向，若电机运动不正常，将此值取反
int Motor_PP = 7;    // 电机极对数

DFOC_RGB RGB = DFOC_RGB();

// 十分度粗棘轮
float attractor_distance = 36 * 3.1415926 / 180.0;  // 十分度，一分度36°
void Coarse_Detents_Mode() {
  float Kp = 20;  // 4->20,更大力矩
  float target = round(DFOC_M0_Angle() / attractor_distance) * attractor_distance;
  DFOC_M0_setTorque(Kp * (target - DFOC_M0_Angle()));
}

void setup() {
  Serial.begin(115200);
  pinMode(12, OUTPUT);
  digitalWrite(12, HIGH);

  DFOC_Vbus(12.6);  // 设定驱动器供电电压
  DFOC_alignSensor(Motor_PP, Sensor_DIR);

  RGB.begin();  // RGB程序初始化
}


void loop() {
  runFOC();
  Coarse_Detents_Mode();

  float angle = DFOC_M0_Angle();
  // RGB.tenTeeth_Ratchet(angle, 100, 255, 0, 0);//无拖尾
  RGB.tenTeeth_Ratchet_Tail(angle, 100, 255, 0, 0);//拖尾
}
