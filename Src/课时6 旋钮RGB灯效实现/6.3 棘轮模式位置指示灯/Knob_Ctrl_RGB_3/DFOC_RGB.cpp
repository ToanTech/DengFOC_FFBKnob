#include "DFOC_RGB.h"
// 自定义颜色
// 可以参考以下网址 https://www.rapidtables.org/zh-CN/web/color/RGB_Color.html
// RGB.setColor(0, 188, 40, 40);  //红色 [索引号0，R,G,B] ,具体颜色可以参考网址里的颜色表

// 初始化设置
DFOC_RGB::DFOC_RGB() {
  ledCounts = 10;    // 灯珠的数量
  pin = 15;          // RMT输出的引脚，即驱动RGB的IO口
  brightness = 120;  // 初始亮度

  rmt_mem = RMT_MEM_64;  // RMT内存设置

  r_pos = 1;
  g_pos = 0;
  b_pos = 2;
}

// RMT通道设置
bool DFOC_RGB::begin() {
  if ((rmt_send = rmtInit(pin, true, rmt_mem)) == NULL) {
    return false;
  }
  for (int i = 0; i < ledCounts; i++) {
    for (int bit = 0; bit < 24; bit++) {
      led_data[i * 24 + bit].level0 = 1;
      led_data[i * 24 + bit].duration0 = 4;
      led_data[i * 24 + bit].level1 = 0;
      led_data[i * 24 + bit].duration1 = 8;
    }
  }
  realTick = rmtSetTick(rmt_send, 100);
  return true;
}

void DFOC_RGB::setColor(int index, u8 brightness, u32 rgb) {
  setColor(index, brightness, rgb >> 16, rgb >> 8, rgb);
}

// 遍历设置像素颜色
void DFOC_RGB::set_pixel(int index, u8 r, u8 g, u8 b) {
  u32 color = r << 16 | g << 8 | b;
  // delayMicroseconds(300);
  for (int bit = 0; bit < 24; bit++) {
    if (color & (1 << (23 - bit))) {
      led_data[index * 24 + bit].level0 = 1;
      led_data[index * 24 + bit].duration0 = 8;
      led_data[index * 24 + bit].level1 = 0;
      led_data[index * 24 + bit].duration1 = 4;
    } else {
      led_data[index * 24 + bit].level0 = 1;
      led_data[index * 24 + bit].duration0 = 4;
      led_data[index * 24 + bit].level1 = 0;
      led_data[index * 24 + bit].duration1 = 8;
    }
  }
}

// 设置好的led数据写入
void DFOC_RGB::show() {
  rmtWrite(rmt_send, led_data, ledCounts * 24);
}

/************************************************灯效************************************************/
// index: 每盏RGB索引号
// brightness：亮度
// R、G、B: RGB三色值
// 设定红绿蓝三通道的颜色值
void DFOC_RGB::setColor(int index, uint8_t brightness, uint8_t R, uint8_t G, uint8_t B) {
  u8 p[3];
  p[r_pos] = R * brightness / 255;
  p[g_pos] = G * brightness / 255;
  p[b_pos] = B * brightness / 255;
  set_pixel(index, p[0], p[1], p[2]);
}

// 灯环所有灯的统一控制
void DFOC_RGB::setAllcolor(int brightness, uint8_t R, uint8_t G, uint8_t B) {
  for (int i = 0; i < ledCounts; i++) {
    setColor(i, brightness, R, G, B);
  }
  show();  // 显示更新后的颜色
}

// 灯环闪烁
static unsigned long blink_counter = 0;
static bool blink_done = false;
bool DFOC_RGB::blink(u8 times, int brightness, uint8_t R, uint8_t G, uint8_t B) {
  unsigned long current_time = millis();
  static unsigned long prev_time = 0;

  // 定时器
  if (current_time - prev_time > 300) {
    prev_time = current_time;
    if (blink_counter >= times * 2)
      blink_done = true;
    else {
      if ((blink_counter % 2) == 0)
        setAllcolor(brightness, R, G, B);
      else
        setAllcolor(0, 0, 0, 0);
      blink_counter++;
    }
  }
  if (blink_done)
    return true;
  return false;
}

// 下次blink之前，切换状态后要reset blink的参数
void DFOC_RGB::blink_reset() {
  blink_done = false;
  blink_counter = 0;
}

// 灯环呼吸灯
void DFOC_RGB::breath(int interval, uint8_t R, uint8_t G, uint8_t B) {
  unsigned long current_time = millis();
  static unsigned long prev_time = 0;
  static unsigned int brightness = 0;
  static bool turn_flag = true;

  // 定时器
  if (current_time - prev_time > interval) {
    prev_time = current_time;
    setAllcolor(brightness, R, G, B);
    if (turn_flag) {
      brightness++;
      if (brightness >= 255)
        turn_flag = false;
    } else {
      brightness--;
      if (brightness <= 0)
        turn_flag = true;
    }
  }
}

// 旋钮调节亮度
void DFOC_RGB::bri_Adj(float angle, uint8_t R, uint8_t G, uint8_t B) {
  uint8_t brightness;
  float bri_angle = (angle >= 0 && angle <= 6.28) ? angle : (angle < 0 ? 0 : 6.28);
  brightness = int((bri_angle / TWO_PI) * 255);
  setAllcolor(brightness, R, G, B);
}

// 棘轮位置的指示灯,有拖尾
void DFOC_RGB::tenTeeth_Ratchet(float angle, uint8_t brightness, uint8_t R, uint8_t G, uint8_t B) {
  float attractor_distance = 36 * 3.1415926 / 180.0;  // 十分度，一分度36°
  float tar = round(angle / attractor_distance);
  int a = int(tar) % 10;                       //限制在-9~9之间
  int lightsup_index = a >= 0 ? a : (a + 10);  //限制在0~9之间

  for (int i = 0; i < 10; i++) {
    if (i == lightsup_index)
      setColor(i, brightness, R, G, B);
    else
      setColor(i, 0, 0, 0, 0);
  }
  show();
}

void DFOC_RGB::tenTeeth_Ratchet_Tail(float angle, uint8_t brightness, uint8_t R, uint8_t G, uint8_t B) {
  float attractor_distance = 36 * 3.1415926 / 180.0;  // 十分度，一分度36°
  float tar = round(angle / attractor_distance);
  int a = int(tar) % 10;                       //限制在-9~9之间
  int lightsup_index = a >= 0 ? a : (a + 10);  //限制在0~9之间

  // for tail
  static int last_lightsup_index = 0;
  static int tail_timer_count = 0;
  unsigned long current_time = millis();
  static unsigned long prev_time = 0;
  static uint8_t tail_brightness = 0;
  static bool tail_dir = true;

  if (current_time - prev_time > 100)  // 加入定时函数，用于tail_timer_count的自加
  {
    prev_time = current_time;
    tail_timer_count++;
  }
  if (lightsup_index != last_lightsup_index) {
    int index_error = lightsup_index - last_lightsup_index;                 //用于判断方向
    tail_dir = (index_error == 1 || index_error == -9) ? (true) : (false);  //一般为8-7=1  9-8=1，下一个为0-9=-9为特殊情况。-1和9都是false，1和-9为true

    last_lightsup_index = lightsup_index;
    tail_timer_count = 0;
  }

  setColor(lightsup_index, brightness, R, G, B);
  tail_brightness = brightness;
  if (tail_dir) {
    for (int i = 9; i > 0; i--) {
      tail_brightness = int(tail_brightness * 0.5);
      setColor(((lightsup_index + i) % 10), int(tail_brightness * pow(0.6, tail_timer_count)), R, G, B);
    }
  } else {
    for (int i = 1; i < 10; i++) {
      tail_brightness = int(tail_brightness * 0.5);
      setColor(((lightsup_index + i) % 10), int(tail_brightness * pow(0.6, tail_timer_count)), R, G, B);
    }
  }
  show();
}
