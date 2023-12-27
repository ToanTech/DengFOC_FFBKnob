#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "BLE2902.h"
#include "BLEHIDDevice.h"
#include "HIDTypes.h"
#include <driver/adc.h>
#include "sdkconfig.h"

#include <arduino.h>

#include "BleConnectionStatus.h"
#include "KeyboardOutputCallbacks.h"
#include "BleKeyboard.h"

#if defined(CONFIG_ARDUHAL_ESP_LOG)
  #include "esp32-hal-log.h"
  #define LOG_TAG ""
#else
  #include "esp_log.h"
  static const char* LOG_TAG = "BLEDevice";
#endif


// Report IDs:
#define KEYBOARD_ID 0x01
#define MEDIA_KEYS_ID 0x02
#define RADIAL_ID 0x03
#define RADIAL_HAPTIC_ID 0x04

static const uint8_t _hidReportDescriptor[] = {
  USAGE_PAGE(1),      0x01,          // USAGE_PAGE (Generic Desktop Ctrls)
  USAGE(1),           0x06,          // USAGE (Keyboard)
  COLLECTION(1),      0x01,          // COLLECTION (Application)
  // ------------------------------------------------- Keyboard
  REPORT_ID(1),       KEYBOARD_ID,   //   REPORT_ID (1)
  USAGE_PAGE(1),      0x07,          //   USAGE_PAGE (Kbrd/Keypad)
  USAGE_MINIMUM(1),   0xE0,          //   USAGE_MINIMUM (0xE0)
  USAGE_MAXIMUM(1),   0xE7,          //   USAGE_MAXIMUM (0xE7)
  LOGICAL_MINIMUM(1), 0x00,          //   LOGICAL_MINIMUM (0)
  LOGICAL_MAXIMUM(1), 0x01,          //   Logical Maximum (1)
  REPORT_SIZE(1),     0x01,          //   REPORT_SIZE (1)
  REPORT_COUNT(1),    0x08,          //   REPORT_COUNT (8)
  HIDINPUT(1),        0x02,          //   INPUT (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  REPORT_COUNT(1),    0x01,          //   REPORT_COUNT (1) ; 1 byte (Reserved)
  REPORT_SIZE(1),     0x08,          //   REPORT_SIZE (8)
  HIDINPUT(1),        0x01,          //   INPUT (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
  REPORT_COUNT(1),    0x05,          //   REPORT_COUNT (5) ; 5 bits (Num lock, Caps lock, Scroll lock, Compose, Kana)
  REPORT_SIZE(1),     0x01,          //   REPORT_SIZE (1)
  USAGE_PAGE(1),      0x08,          //   USAGE_PAGE (LEDs)
  USAGE_MINIMUM(1),   0x01,          //   USAGE_MINIMUM (0x01) ; Num Lock
  USAGE_MAXIMUM(1),   0x05,          //   USAGE_MAXIMUM (0x05) ; Kana
  HIDOUTPUT(1),       0x02,          //   OUTPUT (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
  REPORT_COUNT(1),    0x01,          //   REPORT_COUNT (1) ; 3 bits (Padding)
  REPORT_SIZE(1),     0x03,          //   REPORT_SIZE (3)
  HIDOUTPUT(1),       0x01,          //   OUTPUT (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
  REPORT_COUNT(1),    0x06,          //   REPORT_COUNT (6) ; 6 bytes (Keys)
  REPORT_SIZE(1),     0x08,          //   REPORT_SIZE(8)
  LOGICAL_MINIMUM(1), 0x00,          //   LOGICAL_MINIMUM(0)
  LOGICAL_MAXIMUM(1), 0x65,          //   LOGICAL_MAXIMUM(0x65) ; 101 keys
  USAGE_PAGE(1),      0x07,          //   USAGE_PAGE (Kbrd/Keypad)
  USAGE_MINIMUM(1),   0x00,          //   USAGE_MINIMUM (0)
  USAGE_MAXIMUM(1),   0x65,          //   USAGE_MAXIMUM (0x65)
  HIDINPUT(1),        0x00,          //   INPUT (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
  END_COLLECTION(0),                 // END_COLLECTION
  // ------------------------------------------------- Media Keys
  USAGE_PAGE(1),      0x0C,          // USAGE_PAGE (Consumer)
  USAGE(1),           0x01,          // USAGE (Consumer Control)
  COLLECTION(1),      0x01,          // COLLECTION (Application)
  REPORT_ID(1),       MEDIA_KEYS_ID, //   REPORT_ID (3)
  USAGE_PAGE(1),      0x0C,          //   USAGE_PAGE (Consumer)
  LOGICAL_MINIMUM(1), 0x00,          //   LOGICAL_MINIMUM (0)
  LOGICAL_MAXIMUM(1), 0x01,          //   LOGICAL_MAXIMUM (1)
  REPORT_SIZE(1),     0x01,          //   REPORT_SIZE (1)
  REPORT_COUNT(1),    0x10,          //   REPORT_COUNT (16)
  USAGE(1),           0xB5,          //   USAGE (Scan Next Track)     ; bit 0: 1
  USAGE(1),           0xB6,          //   USAGE (Scan Previous Track) ; bit 1: 2
  USAGE(1),           0xB7,          //   USAGE (Stop)                ; bit 2: 4
  USAGE(1),           0xCD,          //   USAGE (Play/Pause)          ; bit 3: 8
  USAGE(1),           0xE2,          //   USAGE (Mute)                ; bit 4: 16
  USAGE(1),           0xE9,          //   USAGE (Volume Increment)    ; bit 5: 32
  USAGE(1),           0xEA,          //   USAGE (Volume Decrement)    ; bit 6: 64
  USAGE(2),           0x23, 0x02,    //   Usage (WWW Home)            ; bit 7: 128
  USAGE(2),           0x94, 0x01,    //   Usage (My Computer) ; bit 0: 1
  USAGE(2),           0x92, 0x01,    //   Usage (Calculator)  ; bit 1: 2
  USAGE(2),           0x2A, 0x02,    //   Usage (WWW fav)     ; bit 2: 4
  USAGE(2),           0x21, 0x02,    //   Usage (WWW search)  ; bit 3: 8
  USAGE(2),           0x26, 0x02,    //   Usage (WWW stop)    ; bit 4: 16
  USAGE(2),           0x24, 0x02,    //   Usage (WWW back)    ; bit 5: 32
  USAGE(2),           0x83, 0x01,    //   Usage (Media sel)   ; bit 6: 64
  USAGE(2),           0x8A, 0x01,    //   Usage (Mail)        ; bit 7: 128
  HIDINPUT(1),        0x02,          //   INPUT (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  END_COLLECTION(0),                  // END_COLLECTION
   
 0x05, 0x01,                    // Usage Page (Generic Desktop)        0
 0x09, 0x0e,                    // Usage (System Multi-Axis Controller) 2
 0xa1, 0x01,                    // Collection (Application)            4
 0x85, RADIAL_ID,                    //  Report ID (1)                      6
 0x05, 0x0d,                    //  Usage Page (Digitizers)            8
 0x09, 0x21,                    //  Usage (Puck)                       10

 0xa1, 0x02,                    //  Collection (Logical)               12
 0x15, 0x00,                    //   Logical Minimum (0)               14
 0x25, 0x01,                    //   Logical Maximum (1)               16
 0x75, 0x01,                    //   Report Size (1)                   18
 0x95, 0x01,                    //   Report Count (1)                  20
 0xa1, 0x00,                    //   Collection (Physical)             22
 0x05, 0x09,                    //    Usage Page (Button)              24
 0x09, 0x01,                    //    Usage (Vendor Usage 0x01)        26
 0x81, 0x02,                    //    Input (Data,Var,Abs)             28
 0x05, 0x0d,                    //    Usage Page (Digitizers)          30
 0x09, 0x33,                    //    Usage (Touch)                    32
 0x81, 0x02,                    //    Input (Data,Var,Abs)             34
 0x95, 0x06,                    //    Report Count (6)                 36
 0x81, 0x03,                    //    Input (Cnst,Var,Abs)             38
 0xa1, 0x02,                    //    Collection (Logical)             40
 0x05, 0x01,                    //     Usage Page (Generic Desktop)    42
 0x09, 0x37,                    //     Usage (Dial)                    44
 0x16, 0x01, 0x80,              //     Logical Minimum (-32767)        46
 0x26, 0xff, 0x7f,              //     Logical Maximum (32767)         49
 0x75, 0x10,                    //     Report Size (16)                52
 0x95, 0x01,                    //     Report Count (1)                54
 0x81, 0x06,                    //     Input (Data,Var,Rel)            56
 0x35, 0x00,                    //     Physical Minimum (0)            58
 0x46, 0x10, 0x0e,              //     Physical Maximum (3600)         60
 0x15, 0x00,                    //     Logical Minimum (0)             63
 0x26, 0x10, 0x0e,              //     Logical Maximum (3600)          65
 0x09, 0x48,                    //     Usage (Resolution Multiplier)   68
 0xb1, 0x02,                    //     Feature (Data,Var,Abs)          70
 0x45, 0x00,                    //     Physical Maximum (0)            72
 0xc0,                          //    End Collection                   74
 
 0x55, 0x0e,                    //    Unit Exponent (-2)               75
 0x65, 0x11,                    //    Unit (Centimeter,SILinear)       77
 0x46, 0x00, 0x00,              //    Physical Maximum (0)             79
 0x26, 0x00, 0x00,              //    Logical Maximum (0)              82
 0x09, 0x30,                    //    Usage (X)                        85
 0x81, 0x42,                    //    Input (Data,Var,Abs,Null)        87
 0x09, 0x31,                    //    Usage (Y)                        89
 0x46, 0x00, 0x00,              //    Physical Maximum (0)             91
 0x26, 0x00, 0x00,              //    Logical Maximum (0)              94
 0x81, 0x42,                    //    Input (Data,Var,Abs,Null)        97
 0x05, 0x0d,                    //    Usage Page (Digitizers)          99
 0x09, 0x48,                    //    Usage (Width)                    101
 0x15, 0x3a,                    //    Logical Minimum (58)             103
 0x25, 0x3a,                    //    Logical Maximum (58)             105
 0x75, 0x08,                    //    Report Size (8)                  107
 0x55, 0x0f,                    //    Unit Exponent (-1)               109
 0x35, 0x3a,                    //    Physical Minimum (58)            111
 0x45, 0x3a,                    //    Physical Maximum (58)            113
 0x81, 0x03,                    //    Input (Cnst,Var,Abs)             115
 0x55, 0x00,                    //    Unit Exponent (0)                117
 0x65, 0x00,                    //    Unit (None)                      119
 0x35, 0x00,                    //    Physical Minimum (0)             121
 0x45, 0x00,                    //    Physical Maximum (0)             123

 0x05, 0x0e,                    //    Usage Page (Haptic)              125
 0x09, 0x01,                    //    Usage (Simple Haptic Controller) 127
 0xa1, 0x02,                    //    Collection (Logical)             129
 0x15, 0x00,                    //     Logical Minimum (0)             131
 0x26, 0xff, 0x00,              //     Logical Maximum (255)           133
 0x09, 0x24,                    //     Usage (Repeat Count)            136
 0xb1, 0x42,                    //     Feature (Data,Var,Abs,Null)     138
 0x09, 0x24,                    //     Usage (Repeat Count)            140
 0x91, 0x42,                    //     Output (Data,Var,Abs,Null)      142
 0x15, 0x01,                    //     Logical Minimum (1)             144
 0x25, 0x07,                    //     Logical Maximum (7)             146
 0x09, 0x20,                    //     Usage (Auto Trigger)            148
 0xb1, 0x42,                    //     Feature (Data,Var,Abs,Null)     150
 0x09, 0x21,                    //     Usage (Manual Trigger)          152
 0x91, 0x42,                    //     Output (Data,Var,Abs,Null)      154
 0x25, 0x0a,                    //     Logical Maximum (10)            156
 0x09, 0x28,                    //     Usage (Waveform Cutoff Time)    158
 0xb1, 0x42,                    //     Feature (Data,Var,Abs,Null)     160
 0x75, 0x10,                    //     Report Size (16)                162
 0x26, 0xd0, 0x07,              //     Logical Maximum (2000)          164
 0x09, 0x25,                    //     Usage (Retrigger Period)        167
 0xb1, 0x42,                    //     Feature (Data,Var,Abs,Null)     169
 0x09, 0x25,                    //     Usage (Retrigger Period)        171
 0x91, 0x42,                    //     Output (Data,Var,Abs,Null)      173
 0x85, RADIAL_HAPTIC_ID,                    //     Report ID (2)                   175
 0x75, 0x20,                    //     Report Size (32)                177
 0x17, 0x37, 0x00, 0x01, 0x00,  //     Logical Minimum (65591)         179
 0x27, 0x37, 0x00, 0x01, 0x00,  //     Logical Maximum (65591)         184
 0x09, 0x22,                    //     Usage (Auto Trigger Associated Control) 189
 0xb1, 0x02,                    //     Feature (Data,Var,Abs)          191
 0x09, 0x11,                    //     Usage (Duration)                193
 0xa1, 0x02,                    //     Collection (Logical)            195
 0x05, 0x0a,                    //      Usage Page (Ordinals)          197
 0x95, 0x03,                    //      Report Count (3)               199
 0x09, 0x03,                    //      Usage (Vendor Usage 0x03)      201
 0x09, 0x04,                    //      Usage (Vendor Usage 0x04)      203
 0x09, 0x05,                    //      Usage (Vendor Usage 0x05)      205
 0x75, 0x08,                    //      Report Size (8)                207
 0x15, 0x00,                    //      Logical Minimum (0)            209
 0x25, 0xff,                    //      Logical Maximum (255)          211
 0xb1, 0x02,                    //      Feature (Data,Var,Abs)         213
 0xc0,                          //     End Collection                  215
 0x05, 0x0e,                    //     Usage Page (Haptic)             216
 0x09, 0x10,                    //     Usage (Waveform)                218
 0xa1, 0x02,                    //     Collection (Logical)            220
 0x05, 0x0a,                    //      Usage Page (Ordinals)          222
 0x95, 0x01,                    //      Report Count (1)               224
 0x15, 0x03,                    //      Logical Minimum (3)            226
 0x25, 0x03,                    //      Logical Maximum (3)            228
 0x36, 0x03, 0x10,              //      Physical Minimum (4099)        230
 0x46, 0x03, 0x10,              //      Physical Maximum (4099)        233
 0x09, 0x03,                    //      Usage (Vendor Usage 0x03)      236
 0xb1, 0x02,                    //      Feature (Data,Var,Abs)         238
 0x15, 0x04,                    //      Logical Minimum (4)            240
 0x25, 0x04,                    //      Logical Maximum (4)            242
 0x36, 0x04, 0x10,              //      Physical Minimum (4100)        244
 0x46, 0x04, 0x10,              //      Physical Maximum (4100)        247
 0x09, 0x04,                    //      Usage (Vendor Usage 0x04)      250
 0xb1, 0x02,                    //      Feature (Data,Var,Abs)         252
 0x15, 0x05,                    //      Logical Minimum (5)            254
 0x25, 0x05,                    //      Logical Maximum (5)            256
 0x36, 0x04, 0x10,              //      Physical Minimum (4100)        258
 0x46, 0x04, 0x10,              //      Physical Maximum (4100)        261
 0x09, 0x05,                    //      Usage (Vendor Usage 0x05)      264
 0xb1, 0x02,                    //      Feature (Data,Var,Abs)         266
 0x35, 0x00,                    //      Physical Minimum (0)           268
 0x45, 0x00,                    //      Physical Maximum (0)           270
 0xc0,                          //     End Collection                  272
 0xc0,                          //    End Collection                   273
 0xc0,                          //   End Collection                    274
 0xc0,                          //  End Collection                     275
 0xc0,                          // End Collection                      276
  
};

BleKeyboard::BleKeyboard(std::string deviceName, std::string deviceManufacturer, uint8_t batteryLevel) : hid(0)
{
  this->deviceName = deviceName;
  this->deviceManufacturer = deviceManufacturer;
  this->batteryLevel = batteryLevel;
  this->connectionStatus = new BleConnectionStatus();
}

void BleKeyboard::begin(void)
{
  BLEDevice::init(this->deviceName);
  this->pServer = BLEDevice::createServer();
  this->pServer->setCallbacks(this->connectionStatus);

  this->hid = new BLEHIDDevice(this->pServer);
  this->inputKeyboard = this->hid->inputReport(KEYBOARD_ID); // <-- input REPORTID from report map
  this->outputKeyboard = this->hid->outputReport(KEYBOARD_ID);
  this->inputMediaKeys = this->hid->inputReport(MEDIA_KEYS_ID);
  this->inputRadial = this->hid->inputReport(RADIAL_ID);
  this->outputRadial = this->hid->outputReport(RADIAL_ID);
  this->featureRadial = this->hid->featureReport(RADIAL_ID);
  
  
  
  
  this->connectionStatus->inputKeyboard = this->inputKeyboard;
  this->connectionStatus->outputKeyboard = this->outputKeyboard;
  this->connectionStatus->inputMediaKeys = this->inputMediaKeys;
  this->connectionStatus->inputRadial = this->inputRadial;
  
  this->featureRadial->setCallbacks(new radialFeatureHapticCallback(this));

  this->outputKeyboard->setCallbacks(new KeyboardOutputCallbacks());

  this->hid->manufacturer()->setValue(this->deviceManufacturer);

  this->hid->pnp(0x02, 0xe502, 0xa111, 0x0210);
  this->hid->hidInfo(0x00,0x01);

  BLESecurity *pSecurity = new BLESecurity();

  pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND);

  this->hid->reportMap((uint8_t*)_hidReportDescriptor, sizeof(_hidReportDescriptor));
  this->hid->startServices();


  this->onStarted(this->pServer);
  
  BLEAdvertising *pAdvertising = this->pServer->getAdvertising();
  pAdvertising->setAppearance(HID_KEYBOARD);
  pAdvertising->addServiceUUID(this->hid->hidService()->getUUID());
  pAdvertising->start();
  //this->hid->setBatteryLevel(this->batteryLevel);
}

void BleKeyboard::end(void)
{
}

bool BleKeyboard::isConnected(void) {
  return this->connectionStatus->connected;
}

void BleKeyboard::setBatteryLevel(uint8_t level) {
  this->batteryLevel = level;
  if (hid != 0)
    this->hid->setBatteryLevel(this->batteryLevel);
}


void BleKeyboard::sendReport(KeyReport* keys)
{
  if (this->isConnected())
  {
    this->inputKeyboard->setValue((uint8_t*)keys, sizeof(KeyReport));
    this->inputKeyboard->notify();
  }
}

void BleKeyboard::sendReport(MediaKeyReport* keys)
{
  if (this->isConnected())
  {
    this->inputMediaKeys->setValue((uint8_t*)keys, sizeof(MediaKeyReport));
    this->inputMediaKeys->notify();
  }
}
void BleKeyboard::sendReport(RadialReport* keys)
{
  if (this->isConnected())
  {
    this->inputRadial->setValue((uint8_t*)keys, sizeof(RadialReport));
    this->inputRadial->notify();
  }
}
void BleKeyboard::sendReport(RadialOutputReport* keys)
{
  if (this->isConnected())
  {
    this->outputRadial->setValue((uint8_t*)keys, sizeof(RadialOutputReport));
    this->outputRadial->notify();
  }
}
extern
const uint8_t _asciimap[128] PROGMEM;

#define SHIFT 0x80
const uint8_t _asciimap[128] =
{
	0x00,             // NUL
	0x00,             // SOH
	0x00,             // STX
	0x00,             // ETX
	0x00,             // EOT
	0x00,             // ENQ
	0x00,             // ACK
	0x00,             // BEL
	0x2a,			// BS	Backspace
	0x2b,			// TAB	Tab
	0x28,			// LF	Enter
	0x00,             // VT
	0x00,             // FF
	0x00,             // CR
	0x00,             // SO
	0x00,             // SI
	0x00,             // DEL
	0x00,             // DC1
	0x00,             // DC2
	0x00,             // DC3
	0x00,             // DC4
	0x00,             // NAK
	0x00,             // SYN
	0x00,             // ETB
	0x00,             // CAN
	0x00,             // EM
	0x00,             // SUB
	0x00,             // ESC
	0x00,             // FS
	0x00,             // GS
	0x00,             // RS
	0x00,             // US

	0x2c,		   //  ' '
	0x1e|SHIFT,	   // !
	0x34|SHIFT,	   // "
	0x20|SHIFT,    // #
	0x21|SHIFT,    // $
	0x22|SHIFT,    // %
	0x24|SHIFT,    // &
	0x34,          // '
	0x26|SHIFT,    // (
	0x27|SHIFT,    // )
	0x25|SHIFT,    // *
	0x2e|SHIFT,    // +
	0x36,          // ,
	0x2d,          // -
	0x37,          // .
	0x38,          // /
	0x27,          // 0
	0x1e,          // 1
	0x1f,          // 2
	0x20,          // 3
	0x21,          // 4
	0x22,          // 5
	0x23,          // 6
	0x24,          // 7
	0x25,          // 8
	0x26,          // 9
	0x33|SHIFT,      // :
	0x33,          // ;
	0x36|SHIFT,      // <
	0x2e,          // =
	0x37|SHIFT,      // >
	0x38|SHIFT,      // ?
	0x1f|SHIFT,      // @
	0x04|SHIFT,      // A
	0x05|SHIFT,      // B
	0x06|SHIFT,      // C
	0x07|SHIFT,      // D
	0x08|SHIFT,      // E
	0x09|SHIFT,      // F
	0x0a|SHIFT,      // G
	0x0b|SHIFT,      // H
	0x0c|SHIFT,      // I
	0x0d|SHIFT,      // J
	0x0e|SHIFT,      // K
	0x0f|SHIFT,      // L
	0x10|SHIFT,      // M
	0x11|SHIFT,      // N
	0x12|SHIFT,      // O
	0x13|SHIFT,      // P
	0x14|SHIFT,      // Q
	0x15|SHIFT,      // R
	0x16|SHIFT,      // S
	0x17|SHIFT,      // T
	0x18|SHIFT,      // U
	0x19|SHIFT,      // V
	0x1a|SHIFT,      // W
	0x1b|SHIFT,      // X
	0x1c|SHIFT,      // Y
	0x1d|SHIFT,      // Z
	0x2f,          // [
	0x31,          // bslash
	0x30,          // ]
	0x23|SHIFT,    // ^
	0x2d|SHIFT,    // _
	0x35,          // `
	0x04,          // a
	0x05,          // b
	0x06,          // c
	0x07,          // d
	0x08,          // e
	0x09,          // f
	0x0a,          // g
	0x0b,          // h
	0x0c,          // i
	0x0d,          // j
	0x0e,          // k
	0x0f,          // l
	0x10,          // m
	0x11,          // n
	0x12,          // o
	0x13,          // p
	0x14,          // q
	0x15,          // r
	0x16,          // s
	0x17,          // t
	0x18,          // u
	0x19,          // v
	0x1a,          // w
	0x1b,          // x
	0x1c,          // y
	0x1d,          // z
	0x2f|SHIFT,    // {
	0x31|SHIFT,    // |
	0x30|SHIFT,    // }
	0x35|SHIFT,    // ~
	0				// DEL
};


uint8_t USBPutChar(uint8_t c);

// press() adds the specified key (printing, non-printing, or modifier)
// to the persistent key report and sends the report.  Because of the way
// USB HID works, the host acts like the key remains pressed until we
// call release(), releaseAll(), or otherwise clear the report and resend.
size_t BleKeyboard::press(uint8_t k)
{
	uint8_t i;
	if (k >= 136) {			// it's a non-printing key (not a modifier)
		k = k - 136;
	} else if (k >= 128) {	// it's a modifier key
		_keyReport.modifiers |= (1<<(k-128));
		k = 0;
	} else {				// it's a printing key
		k = pgm_read_byte(_asciimap + k);
		if (!k) {
			setWriteError();
			return 0;
		}
		if (k & 0x80) {						// it's a capital letter or other character reached with shift
			_keyReport.modifiers |= 0x02;	// the left shift modifier
			k &= 0x7F;
		}
	}

	// Add k to the key report only if it's not already present
	// and if there is an empty slot.
	if (_keyReport.keys[0] != k && _keyReport.keys[1] != k &&
		_keyReport.keys[2] != k && _keyReport.keys[3] != k &&
		_keyReport.keys[4] != k && _keyReport.keys[5] != k) {

		for (i=0; i<6; i++) {
			if (_keyReport.keys[i] == 0x00) {
				_keyReport.keys[i] = k;
				break;
			}
		}
		if (i == 6) {
			setWriteError();
			return 0;
		}
	}
	sendReport(&_keyReport);
	return 1;
}

size_t BleKeyboard::press(const MediaKeyReport k)
{
    uint16_t k_16 = k[1] | (k[0] << 8);
    uint16_t mediaKeyReport_16 = _mediaKeyReport[1] | (_mediaKeyReport[0] << 8);

    mediaKeyReport_16 |= k_16;
    _mediaKeyReport[0] = (uint8_t)((mediaKeyReport_16 & 0xFF00) >> 8);
    _mediaKeyReport[1] = (uint8_t)(mediaKeyReport_16 & 0x00FF);

	sendReport(&_mediaKeyReport);
	return 1;
}

// release() takes the specified key out of the persistent key report and
// sends the report.  This tells the OS the key is no longer pressed and that
// it shouldn't be repeated any more.
size_t BleKeyboard::release(uint8_t k)
{
	uint8_t i;
	if (k >= 136) {			// it's a non-printing key (not a modifier)
		k = k - 136;
	} else if (k >= 128) {	// it's a modifier key
		_keyReport.modifiers &= ~(1<<(k-128));
		k = 0;
	} else {				// it's a printing key
		k = pgm_read_byte(_asciimap + k);
		if (!k) {
			return 0;
		}
		if (k & 0x80) {							// it's a capital letter or other character reached with shift
			_keyReport.modifiers &= ~(0x02);	// the left shift modifier
			k &= 0x7F;
		}
	}

	// Test the key report to see if k is present.  Clear it if it exists.
	// Check all positions in case the key is present more than once (which it shouldn't be)
	for (i=0; i<6; i++) {
		if (0 != k && _keyReport.keys[i] == k) {
			_keyReport.keys[i] = 0x00;
		}
	}

	sendReport(&_keyReport);
	return 1;
}

size_t BleKeyboard::release(const MediaKeyReport k)
{
    uint16_t k_16 = k[1] | (k[0] << 8);
    uint16_t mediaKeyReport_16 = _mediaKeyReport[1] | (_mediaKeyReport[0] << 8);
    mediaKeyReport_16 &= ~k_16;
    _mediaKeyReport[0] = (uint8_t)((mediaKeyReport_16 & 0xFF00) >> 8);
    _mediaKeyReport[1] = (uint8_t)(mediaKeyReport_16 & 0x00FF);

	sendReport(&_mediaKeyReport);
	return 1;
}

void BleKeyboard::releaseAll(void)
{
	_keyReport.keys[0] = 0;
	_keyReport.keys[1] = 0;
	_keyReport.keys[2] = 0;
	_keyReport.keys[3] = 0;
	_keyReport.keys[4] = 0;
	_keyReport.keys[5] = 0;
	_keyReport.modifiers = 0;
    _mediaKeyReport[0] = 0;
    _mediaKeyReport[1] = 0;
	sendReport(&_keyReport);
}

size_t BleKeyboard::write(uint8_t c)
{
	uint8_t p = press(c);  // Keydown
	release(c);            // Keyup
	return p;              // just return the result of press() since release() almost always returns 1
}

size_t BleKeyboard::write(const MediaKeyReport c)
{
	uint16_t p = press(c);  // Keydown
	release(c);            // Keyup
	return p;              // just return the result of press() since release() almost always returns 1
}

size_t BleKeyboard::write(const uint8_t *buffer, size_t size) {
	size_t n = 0;
	while (size--) {
		if (*buffer != '\r') {
			if (write(*buffer)) {
			  n++;
			} else {
			  break;
			}
		}
		buffer++;
	}
	return n;
}

bool BleKeyboard::dialPressed(){
  return _radialReport.button == 1;
}
void BleKeyboard::pressDial(){
  _radialReport.button = 3;
  rotate(0);
}
void BleKeyboard::releaseDial(){
  _radialReport.button = 2;
  rotate(0);
}
void BleKeyboard::rotate(int angle){
  
  _radialReport.vala = 0x0A;
  _radialReport.valb = 0x0B;
  _radialReport.valc = 0x0C;
  _radialReport.vald = 0x0D;
  _radialReport.vale = 0x3A;
  _radialReport.rotation = angle;
  if (dial_mult > 1){
    _radialReport.rotation = angle * dial_mult;
  }
  sendReport(&_radialReport);
  
}

void radialHapticCallback::onRead(BLECharacteristic* pCharacteristic){
  ESP_LOGI(LOG_TAG, "I've been read!");
}
void radialHapticCallback::onWrite(BLECharacteristic* pCharacteristic){  
  std::string buff = pCharacteristic->getValue();
  Serial.print("Written data:");
  for (int i = 0; i < buff.length(); i++){
    Serial.print(buff[i], HEX);
  }
  Serial.println();
}

radialFeatureHapticCallback::radialFeatureHapticCallback(BleKeyboard *kbd){
  pKeyboardReference = kbd;
}

void radialFeatureHapticCallback::onWrite(BLECharacteristic* pCharacteristic){  
  std::string buff = pCharacteristic->getValue();
  RadialFeatureReport *r = (RadialFeatureReport*) buff.c_str();
  
  Serial.print("Feature report data:");
  for (int i = buff.length() -1; i >= 0; i--){
    Serial.print(buff[i], HEX);
  }
  Serial.println();
  
  
  pKeyboardReference->dial_vibrate = r->enabled;
  if (r->vibration_amount == 0){
    pKeyboardReference->dial_interval = 1;
    pKeyboardReference->dial_mult = 1;
  }else if (r->vibration_amount < 100){
    pKeyboardReference->dial_interval = round(100.0 / (float)r->vibration_amount);
    pKeyboardReference->dial_mult = 1;
  } else {
    pKeyboardReference->dial_interval = 1;
    pKeyboardReference->dial_mult = (float)r->vibration_amount / 100.0;
  }
  if (pKeyboardReference->dial_interval < 2){
    pKeyboardReference->dial_vibrate = 0;
  }
  pKeyboardReference->dial_pos = 0;
  Serial.println(r->vibration_amount);
  Serial.println(pKeyboardReference->dial_interval);
  Serial.println(pKeyboardReference->dial_mult);
}
