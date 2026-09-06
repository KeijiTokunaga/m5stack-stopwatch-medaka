#include <M5Unified.h>
#include "World.h"
using namespace aquarium;
M5Canvas frame(&M5.Display);
World world;
bool ready=false, imu=false, night=false;
uint32_t previous=0, lastFrame=0, frames=0;
float accumulator=0;
uint16_t rgb(int r,int g,int b) { return M5.Display.color565(r,g,b); }
uint16_t color(int r,int g,int b) { float k=night?.40f:1.f; return rgb(clamp(r*k*world.tint[0],0,255),clamp(g*k*world.tint[1],0,255),clamp(b*k*world.tint[2],0,255)); }

#include "Scene.h"

void setup() {
  auto cfg=M5.config(); cfg.internal_imu=true; cfg.internal_spk=false; cfg.internal_mic=false;
  M5.begin(cfg); Serial.begin(115200);
  M5.Display.setRotation(0); M5.Display.setBrightness(150);
  frame.setColorDepth(16); frame.setPsram(true);
  ready=frame.createSprite(466,466)!=nullptr;
  if(!ready) { M5.Display.fillScreen(TFT_BLACK); M5.Display.setTextDatum(middle_center); M5.Display.drawString("PSRAM allocation failed",233,233,2); return; }
  imu=M5.Imu.isEnabled(); previous=millis();
  Serial.printf("MEDAKA ready imu=%d psram=%u\n",imu,ESP.getPsramSize());
}
void loop() {
  M5.update();
  if(!ready) { delay(50); return; }
  uint32_t now=millis(); float elapsed=std::min((now-previous)*.001f,.1f); previous=now;
  if(imu) {
    if(M5.Imu.update()) {
      auto data=M5.Imu.getImuData();
      world.sense(data.accel.x,data.accel.y,data.accel.z,elapsed,data.gyro.x,data.gyro.y,data.gyro.z);
    }
  }
  if(M5.BtnA.wasClicked()) world.feed(155);
  if(M5.BtnA.wasHold()) { night=!night; M5.Display.setBrightness(night?70:150); }
  if(M5.BtnB.wasClicked()) world.feed(311);
  auto touch=M5.Touch.getDetail();
  if(touch.wasHold()) world.toggleLight();
  if(touch.wasClicked()) world.ripple(touch.x,125);
  accumulator+=elapsed;
  while(accumulator>=1.f/120) { world.step(1.f/120); accumulator-=1.f/120; }
  if(now-lastFrame>=33) { lastFrame=now; render(); }
  if(Serial.available()) {
    char c=Serial.read();
    if(c=='?') Serial.printf("MEDAKA imu=%d frames=%lu tilt=%.3f activity=%.3f gyroZ=%.1f slosh=%.1f food=%d eaten=%u light=%.1f fixed=%d heap=%u\n",imu,(unsigned long)frames,world.tilt,world.activity,world.rollRate,world.slosh,world.foodCount(),world.eaten,world.lightClock,world.lightFrozen,ESP.getFreeHeap());
  }
  delay(1);
}
