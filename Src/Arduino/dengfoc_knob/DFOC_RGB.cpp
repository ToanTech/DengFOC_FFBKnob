#include "DFOC_RGB.h"
#include "motor_function.h"

// 自定义颜色
// 可以参考以下网址 https://www.rapidtables.org/zh-CN/web/color/RGB_Color.html
// RGB.setColor(0, 188, 40, 40);  //红色 [索引号0，R,G,B] ,具体颜色可以参考网址里的颜色表

// 初始化设置
DFOC_RGB::DFOC_RGB()
{
	ledCounts = 10;
	pin = 15; // 4
	rmt_chn = 0;
	rmt_mem = RMT_MEM_64;
	br = 255;
	LED_TYPE t = TYPE_GRB;
	setLedType(t);
}
// RMT通道设置
bool DFOC_RGB::begin()
{
	switch (rmt_chn)
	{
	case 0:
		rmt_mem = RMT_MEM_64;
		break;
	case 1:
		rmt_mem = RMT_MEM_128;
		break;
	case 2:
		rmt_mem = RMT_MEM_192;
		break;
	case 3:
		rmt_mem = RMT_MEM_256;
		break;
	case 4:
		rmt_mem = RMT_MEM_320;
		break;
	case 5:
		rmt_mem = RMT_MEM_384;
		break;
	case 6:
		rmt_mem = RMT_MEM_448;
		break;
	case 7:
		rmt_mem = RMT_MEM_512;
		break;
	default:
		rmt_mem = RMT_MEM_64;
		break;
	}
	if ((rmt_send = rmtInit(pin, true, rmt_mem)) == NULL)
	{
		return false;
	}
	for (int i = 0; i < ledCounts; i++)
	{
		for (int bit = 0; bit < 24; bit++)
		{
			led_data[i * 24 + bit].level0 = 1;
			led_data[i * 24 + bit].duration0 = 4;
			led_data[i * 24 + bit].level1 = 0;
			led_data[i * 24 + bit].duration1 = 8;
		}
	}
	realTick = rmtSetTick(rmt_send, 100);
	return true;
}

// 设置RGB类型
void DFOC_RGB::setLedType(LED_TYPE t)
{
	rOffset = (t >> 4) & 0x03;
	gOffset = (t >> 2) & 0x03;
	bOffset = t & 0x03;
}

// 设置RGB亮度
void DFOC_RGB::setBrightness(u8 brightness)
{
	br = constrain(brightness, 0, 255);
}

void DFOC_RGB::setColor(int index, u32 rgb)
{
	setColor(index, rgb >> 16, rgb >> 8, rgb);
}

// 设定红绿蓝三通道的颜色值
void DFOC_RGB::setColor(int index, u8 r, u8 g, u8 b)
{
	u8 p[3];
	p[rOffset] = r * br / 255;
	p[gOffset] = g * br / 255;
	p[bOffset] = b * br / 255;
	set_pixel(index, p[0], p[1], p[2]);
}

// 遍历设置像素颜色
void DFOC_RGB::set_pixel(int index, u8 r, u8 g, u8 b)
{
	u32 color = r << 16 | g << 8 | b;
	for (int bit = 0; bit < 24; bit++)
	{
		if (color & (1 << (23 - bit)))
		{
			led_data[index * 24 + bit].level0 = 1;
			led_data[index * 24 + bit].duration0 = 8;
			led_data[index * 24 + bit].level1 = 0;
			led_data[index * 24 + bit].duration1 = 4;
		}
		else
		{
			led_data[index * 24 + bit].level0 = 1;
			led_data[index * 24 + bit].duration0 = 4;
			led_data[index * 24 + bit].level1 = 0;
			led_data[index * 24 + bit].duration1 = 8;
		}
	}
}

// 设置好的led数据写入
void DFOC_RGB::show()
{
	rmtWrite(rmt_send, led_data, ledCounts * 24);
}

uint32_t DFOC_RGB::Wheel(byte pos)
{
	u32 WheelPos = pos % 0xff;
	if (WheelPos < 85)
	{
		return ((255 - WheelPos * 3) << 16) | ((WheelPos * 3) << 8);
	}
	if (WheelPos < 170)
	{
		WheelPos -= 85;
		return (((255 - WheelPos * 3) << 8) | (WheelPos * 3));
	}
	WheelPos -= 170;
	return ((WheelPos * 3) << 16 | (255 - WheelPos * 3));
}

/************************************************灯效************************************************/
uint8_t blink_end = 0;
void DFOC_RGB::blink(u8 times, int brightness, u8 R, u8 G, u8 B)
{
	unsigned long startTime = millis();
	static unsigned long prevTime = 0;
	static int rgbChangeFlag = 0;

	// 更新电机状态
	if (state != last_state_RGB)
		rgbChangeFlag = 0;
	last_state_RGB = state;

	// 定时器
	if (startTime - prevTime > 300)
	{
		prevTime = startTime;
		rgbChangeFlag++;
	}
	// 实现
	if (((rgbChangeFlag % 2) == 0) && rgbChangeFlag <= 4) // 偶数亮，奇数灭，flag判断5次
		setAllcolor(brightness, R, G, B);

	// 奇数时灭
	else if (rgbChangeFlag <= times)
		setAllClose(10);

	// 跳出闪烁
	else if (rgbChangeFlag > times + 1)
		blink_end = 1;
}

// 角度变化调节亮度
void DFOC_RGB::brighness_Adjustable(float _subdivision, float pre_angle)
{
	static int i = -1;
	static int stutes_A = 0;
	static int stutes_B = 1;

	// RGB
	float subdivision_angle = (360.0 / _subdivision) - 2.0; // -2为灵敏度
	static float last_angle_RGB = 0;
	static uint8_t angle_turn = 0;

	if (pre_angle - last_angle_RGB >= subdivision_angle)
	{
		angle_turn = 0;
		last_angle_RGB = pre_angle;
	}
	else if (pre_angle - last_angle_RGB <= -subdivision_angle)
	{
		angle_turn = 1;
		last_angle_RGB = pre_angle;
	}

	// 更新i
	if (angle_turn == 0)
	{
		stutes_B = 0;
		angle_turn = 2;		   // 不为0/1
		i = (i - 1 + 60) % 60; // 确保在 0 到 60 之间循环
		if (i == 0)
			stutes_A = 1;
	}
	else if (angle_turn == 1)
	{
		stutes_A = 0;
		angle_turn = 2; // 不为0/1
		i = (i + 1) % 60;
		if (i == 59)
			stutes_B = 1;
	}
	if (stutes_B == 1)
	{
		setAllcolor(255, 255, 255, 0);
		i = 59;
	}
	else if (stutes_A == 1)
	{
		setAllcolor(4.25, 255, 255, 0);
		i = 0;
	}
	else
		setAllcolor(4.25 * i, 255, 255, 0);
}

// 速度越快，跑马灯越亮
void DFOC_RGB::bri_Adjustable_horse_race_lamp(float pre_speed)
{
	int ledSpeed = int(map(pre_speed, 0, 120, 0, 250));
	int speed_brightly = abs(ledSpeed);
	horse_race_lamp(1, speed_brightly + 5, 1); // speed：为最快，5为初始亮度，1为cw，0为ccw
}

// 转20格为一圈，对应亮10个灯。效果：交替亮起
void DFOC_RGB::Alternate_light_up(float _subdivision, float pre_angle, u8 brightness)
{
	// RGB
	float subdivision_angle = (360.0 / _subdivision) - 2.0; // -2为灵敏度
	static float last_angle_RGB = 0;
	static uint8_t angle_turn = 0;

	if (pre_angle - last_angle_RGB >= subdivision_angle)
	{
		angle_turn = 0;
		last_angle_RGB = pre_angle;
	}
	else if (pre_angle - last_angle_RGB <= -subdivision_angle)
	{
		angle_turn = 1;
		last_angle_RGB = pre_angle;
	}

	static int i = -1;
	// 更新i
	int statues = 0;
	// 判断颜色是否变化
	static int state = 0;
	if (angle_turn == 0)
	{
		// 圈数减少
		angle_turn = 2; // 不为0/1
		i = (i - 1 + 20) % 20;
		statues = 1;
	}
	else if (angle_turn == 1)
	{
		// 圈数增加
		angle_turn = 2; // 不为0/1
		i = (i + 1) % 20;
		statues = 1;
	}

	if (state == 0 && statues != 0)
	{
		// 所有为洋红色
		for (int j = 0; j < ledCounts; j++)
		{
			setColor(j, 255, 0, 255);
		}
		// 为红色
		setColor(i / 2, 0, 204, 0);
		state = 1;
	}
	else if (state == 1 && statues != 0)
	{
		for (int j = 0; j < ledCounts; j++)
		{
			setColor(j, 255, 0, 255);
		}
		setColor(i / 2, 255, 255, 51);
		state = 0;
	}
	setBrightness(brightness);
	show();
}

// 通过传入角度，判断转动多少角度来更新i
// Rotate and light up one grid
// 细分度要跟灯的数量对上才能转动一格亮一格
int DFOC_RGB::update_i(float _subdivision, float pre_angle, int numLEDs)
{
	float subdivision_angle = (360.0 / _subdivision) - 2.0; // 36°,  -2为灵敏度
	static float last_angle_RGB = 0;
	static uint8_t angle_turn = 0;

	if (pre_angle - last_angle_RGB >= subdivision_angle)
	{
		angle_turn = 0;
		last_angle_RGB = pre_angle;
	}
	else if (pre_angle - last_angle_RGB <= -subdivision_angle)
	{
		angle_turn = 1;
		last_angle_RGB = pre_angle;
	}

	static int i = -1;
	// 更新i
	if (angle_turn == 0)
	{
		angle_turn = 2;					 // 不为0/1
		i = (i - 1 + numLEDs) % numLEDs; // 确保在 0 到 numLEDs 之间循环
	}
	else if (angle_turn == 1)
	{
		angle_turn = 2;		   // 不为0/1
		i = (i + 1) % numLEDs; // 确保在 0 到 numLEDs 之间循环
	}
	return i;
}

// cw跑马灯正传，ccw跑马灯反转
void DFOC_RGB::change_dir_horse_race_lamp(float pre_speed, u8 brightness) // 传入当前速度
{
	// 根据速度正负判断方向
	static int dir_color = 0;
	// 触发阈值，绝对速度大于2
	if (pre_speed > 2)
		dir_color = 1;
	else if (pre_speed < -2)
		dir_color = 0;

	// setAllrandom(400, 10);//随机闪烁
	horse_race_lamp(1, brightness, dir_color); // 1为速度最快,120为初始亮度,1为cw，0为ccw
}

// 拖尾+闪烁/不闪烁  注释中的代码为闪烁代码
void DFOC_RGB::Tail_lamp(u8 i, u8 brightness)
{
	static uint8_t cnt[10]{255, 255, 255, 255, 255, 255, 255, 255, 255, 255};
	static int sta[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	setColor(i, 0, 100, 255);
	sta[i] = 1;

	for (int j = 0; j < 10; j++)
	{
		if (sta[j] == 1 && i != j)
		{
			setColor(j, 0, cnt[j] = cnt[j] - 0.5, cnt[j] = cnt[j] - 0.4);
			if (cnt[j] == 0)
			{
				cnt[j] = 255;
				setColor(0, 0, 0, 0);
				sta[j] = 0;
			}
		}
	}
	// static uint16_t cnt[10] = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255};
	// static int j[10] = {0};

	// // 只有 i 在有效范围内才执行
	// if (i >= 0 && i < 10)
	// {
	// 	j[i] = 1;
	// 	setColor(i, 0, 0, 255);

	// 	if (j[i] == 1)
	// 	{
	// 		setColor(i, 0, 0, cnt[i]--);
	// 		if (cnt[i] == 0)
	// 		{
	// 			cnt[i] = 255;
	// 			setColor(i, 0, 0, 0);
	// 			j[i] = 0;
	// 		}
	// 	}
	// }

	// // 循环处理所有灯
	// for (int k = 0; k < 10; ++k)
	// {
	// 	if (j[k] == 1 && i != k)
	// 	{
	// 		setColor(k, 0, 0, cnt[k]--);
	// 		if (cnt[k] == 0)
	// 		{
	// 			cnt[k] = 255;
	// 			setColor(k, 0, 0, 0);
	// 			j[k] = 0;
	// 		}
	// 	}
	// }

	setBrightness(brightness);
	show();
}

// 跑马灯
// LOCK_STATE
void DFOC_RGB::horse_race_lamp(u8 speed, u8 brightness, u8 _dir_flag)
{
	unsigned long startTime = millis();
	static unsigned long prevTime = 0;
	static int color_count = 0, led_count = -1, cnt_flag = 0;
	if (startTime - prevTime > speed)
	{

		if (color_count++ < 255)
			cnt_flag = 1;
		else
			color_count = 0;
		prevTime = startTime;
	}
	if (cnt_flag == 1)
	{
		if (_dir_flag)
		{
			for (int i = 0; i < ledCounts; i++)
				setColor(i, Wheel((i * 256 / ledCounts + color_count) & 255)); // 设置灯带中每个LED的颜色值
		}
		else if (_dir_flag == 0)
		{
			for (int i = 0; i < ledCounts; i++)
				setColor(9 - i, Wheel((i * 256 / ledCounts + color_count) & 255)); // 设置灯带中每个LED的颜色值
		}

		cnt_flag = 0;
		setBrightness(brightness);
		show();
	}
}

// 呼吸灯
void DFOC_RGB::breath(int _speed, int numLEDs, int R, int G, int B)
{
	unsigned long startTime = millis();
	static unsigned long prevTime = 0;
	static int j = -1, i = -1, count = 0, k = 255;

	if (startTime - prevTime > _speed)
	{
		if (j++ < 255)
		{
			count = 1;
		}
		else if (j > 255 && k > 0)
		{
			count = 2;
			k--;
		}
		else if (k <= 0 && j > 256)
		{
			j = -1;
			k = 255;
		}
		prevTime = startTime;
	}

	if (count == 1)
	{
		if (i++ < ledCounts)
		{
			setBrightness(j);
			setColor(i, R, G, B);
		}
		else
			i = -1;
		show();
		count = 0;
	}
	else if (count == 2)
	{
		if (i++ < ledCounts)
		{
			setBrightness(k);
			setColor(i, R, G, B);
		}
		else
			i = -1;
		show();
		count = 0;
	}
}

// 设置所有灯颜色一样
void DFOC_RGB::setAllcolor(int brightness, u8 R, u8 G, u8 B)
{
	for (int i = 0; i < ledCounts; i++)
	{
		setColor(i, R, G, B);
	}
	setBrightness(brightness);
	show(); // 显示更新后的颜色
}

// 随机闪烁
void DFOC_RGB::setAllrandom(int _speed, int numLEDs)
{
	unsigned long startTime = millis();
	static unsigned long prevTime = 0;
	if (startTime - prevTime > _speed)
	{
		for (int i = 0; i < numLEDs; i++)
		{
			int r = random(256);
			int g = random(256);
			int b = random(256);
			setColor(i, r, g, b);
		}
		// int brightness = random(256);
		setBrightness(100);
		show(); // 显示更新后的颜色
		prevTime = startTime;
	}
}

// 熄灭所有灯
void DFOC_RGB::setAllClose(int numLEDs)
{
	for (int i = 0; i < numLEDs; i++)
	{
		setColor(i, 0, 0, 0);
	}
	setBrightness(0);
	show(); // 显示更新后的颜色
}
