#include <M5Unified.h>
#include "World.h"
#include "Energy.h"
#include "Screens.h"
#include "Battery.h"
using namespace aquarium;
M5Canvas frame(&M5.Display);
World world;
Energy energy;
Screens screens;
Battery battery;
bool batteryCharging=false;
int batteryLevel=-1;
uint32_t lastBatteryRead=0;
int appliedBrightness=150;
bool ready=false, imu=false, night=false;
uint32_t previous=0, lastFrame=0, frames=0;
float accumulator=0;
uint16_t rgb(int r,int g,int b) { return M5.Display.color565(r,g,b); }
uint16_t color(int r,int g,int b) { float k=night?.40f:1.f; return rgb(clamp(r*k*world.tint[0],0,255),clamp(g*k*world.tint[1],0,255),clamp(b*k*world.tint[2],0,255)); }

#include "Scene.h"

void readBattery(uint32_t now) {
  batteryCharging=M5.Power.isCharging()==m5::Power_Class::is_charging_t::is_charging;
  batteryLevel=battery.update(M5.Power.getBatteryLevel(),batteryCharging);
  lastBatteryRead=now;
}

void renderBattery() {
  frame.fillSprite(rgb(8,19,25));
  frame.setTextDatum(middle_center);
  frame.setTextColor(rgb(159,194,183));
  frame.drawString("BATTERY",233,103,4);
  uint16_t accent=batteryLevel>=0 && batteryLevel<=20?rgb(235,158,85):rgb(113,207,176);
  frame.drawRoundRect(133,146,190,78,12,accent);
  frame.fillRoundRect(325,169,9,31,3,accent);
  if(batteryLevel>0)frame.fillRoundRect(140,153,176*batteryLevel/100,64,6,accent);
  frame.setTextColor(rgb(228,239,223));
  String value=batteryLevel<0?String("--"):String(batteryLevel)+"%";
  frame.setTextSize(3);frame.drawString(value,233,275,4);frame.setTextSize(1);
  frame.setTextColor(rgb(133,161,156));
  frame.drawString(batteryLevel<0?"UNAVAILABLE":batteryCharging?"CHARGING":"ESTIMATED",233,331,2);
  frame.setTextColor(rgb(231,203,100));
  frame.drawString("YELLOW: BACK",233,374,2);
  frame.pushSprite(0,0);++frames;
}

void setup() {
  auto cfg=M5.config(); cfg.internal_imu=true; cfg.internal_spk=false; cfg.internal_mic=false;
  M5.begin(cfg); Serial.begin(115200);
  M5.Display.setRotation(0); M5.Display.setBrightness(150);
  frame.setColorDepth(16); frame.setPsram(true);
  ready=frame.createSprite(466,466)!=nullptr;
  if(!ready) { M5.Display.fillScreen(TFT_BLACK); M5.Display.setTextDatum(middle_center); M5.Display.drawString("PSRAM allocation failed",233,233,2); return; }
  imu=M5.Imu.isEnabled(); previous=millis(); energy.begin(previous);
  readBattery(previous);
  Serial.printf("MEDAKA ready imu=%d psram=%u\n",imu,ESP.getPsramSize());
}
void loop() {
  M5.update();
  if(!ready) { delay(50); return; }
  uint32_t now=millis(); float elapsed=std::min((now-previous)*.001f,.25f); previous=now;
  bool moved=false;
  if(imu) {
    if(M5.Imu.update()) {
      auto data=M5.Imu.getImuData();
      moved=energy.motion(data.accel.x,data.accel.y,data.accel.z,data.gyro.x,data.gyro.y,data.gyro.z);
      world.sense(data.accel.x,data.accel.y,data.accel.z,elapsed,data.gyro.x,data.gyro.y,data.gyro.z);
    }
  }
  bool wasBattery=screens.battery;
  if(screens.yellow(M5.BtnA.wasHold(),M5.BtnA.wasClicked())) world.feed(155);
  if(!screens.battery && M5.BtnB.wasClicked()) world.feed(311);
  auto touch=M5.Touch.getDetail();
  if(!screens.battery) {
    if(touch.wasHold()) world.toggleLight();
    if(touch.wasClicked()) world.ripple(touch.x,125);
  }
  bool screenChanged=wasBattery!=screens.battery;
  if((screens.battery && screenChanged) || now-lastBatteryRead>=1000) readBattery(now);
  energy.update(now,moved || M5.BtnA.isPressed() || M5.BtnB.isPressed()
    || screenChanged || touch.isPressed() || touch.wasClicked() || world.foodCount()>0);
  int brightness=energy.brightness(night);
  if(brightness!=appliedBrightness) {M5.Display.setBrightness(brightness);appliedBrightness=brightness;}
  accumulator+=elapsed;
  while(accumulator>=1.f/120) { world.step(1.f/120); accumulator-=1.f/120; }
  if(screenChanged || now-lastFrame>=(screens.battery?1000:energy.frameInterval())) {
    lastFrame=now;if(screens.battery)renderBattery();else render();
  }
  if(Serial.available()) {
    char c=Serial.read();
    if(c=='?') Serial.printf("MEDAKA imu=%d frames=%lu tilt=%.3f activity=%.3f gyroZ=%.1f slosh=%.1f food=%d eaten=%u light=%.1f fixed=%d eco=%d brightness=%d screen=%s battery=%d charging=%d heap=%u\n",imu,(unsigned long)frames,world.tilt,world.activity,world.rollRate,world.slosh,world.foodCount(),world.eaten,world.lightClock,world.lightFrozen,energy.eco,appliedBrightness,screens.battery?"battery":"aquarium",batteryLevel,batteryCharging,ESP.getFreeHeap());
  }
  delay(energy.loopDelay());
}
