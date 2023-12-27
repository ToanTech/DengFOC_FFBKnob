#include <arduino.h>

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>


#include "BLE2902.h"
#include "BLEHIDDevice.h"
#include "HIDTypes.h"
#include "HIDKeyboardTypes.h"
#include <driver/adc.h>
#include "sdkconfig.h"



#include "driver/pcnt.h"

#include "bleradial.h"

void BleRadialInput::sendValue(int button, int dial){
  RadialHidReport r;
  r.button = button;
  r.rotation = dial;

  
  inputReport->setValue((uint8_t*)&r, 2);
  inputReport->notify();
}

void BleRadialInput::init(){
  BLEDevice::init("Radial Input Device 2");
  BLEServer *pServer = BLEDevice::createServer();
  BLEHIDDevice *hid = new BLEHIDDevice(pServer);
  
  hid->pnp(0x02, 0xe502, 0xa111, 0x0210);
  //hid->pnp(0x01, 0x02E5, 0xabcd, 0x0110);
  hid->hidInfo(0x01,0x0E);
  //hid->hidInfo(0x00,0x01);
  hid->manufacturer()->setValue("Microsoft 2");

  
  
  BLESecurity *pSecurity = new BLESecurity();

  pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND);
  
  
  hid->reportMap((uint8_t*)radial_hidReportDescriptor, sizeof(radial_hidReportDescriptor));
  inputReport = hid->inputReport(0x01);

 
  
  //hid->setBatteryLevel(100);
  hid->startServices();

  sendValue(0, 0);
  
  
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(hid->hidService()->getUUID());
  pAdvertising->setScanResponse(true);
  BLEDevice::startAdvertising();
}
