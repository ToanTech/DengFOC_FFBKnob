#ifndef _DFOC_RGB_h
#define _DFOC_RGB_h

#include <Arduino.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "Arduino.h"

#include "esp32-hal.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;

#define LEDNum 12
#define TotalBits 24 * LEDNum

class DFOC_RGB {
protected:
  u8 pin;

  u8 r_pos;
  u8 g_pos;
  u8 b_pos;

  float realTick;
  rmt_reserve_memsize_t rmt_mem;
  rmt_data_t led_data[TotalBits];
  rmt_obj_t *rmt_send = NULL;

  // 新增私有成员变量用于保存当前颜色和亮度
  int brightness;

public:
  u16 ledCounts;
  DFOC_RGB();
  bool begin();

  void setColor(int index, u8 brightness, u32 rgb);
  void set_pixel(int index, u8 r, u8 g, u8 b);
  void show();

  /************************************************灯效************************************************/

  void setColor(int index, uint8_t brightness, uint8_t R, uint8_t G, uint8_t B);
  void setAllcolor(int brightness, uint8_t R, uint8_t G, uint8_t B);  // 灯环所有灯的统一控制
  bool blink(u8 times, int brightness, uint8_t R, uint8_t G, uint8_t B);
  void blink_reset();
  void breath(int interval, uint8_t R, uint8_t G, uint8_t B);
  void bri_Adj(float angle, uint8_t R, uint8_t G, uint8_t B);
  void tenTeeth_Ratchet(float angle, uint8_t brightness, uint8_t R, uint8_t G, uint8_t B);
  void tenTeeth_Ratchet_Tail(float angle, uint8_t brightness, uint8_t R, uint8_t G, uint8_t B);
};

#endif
