#include "../src/World.h"
#include <cassert>
#include <cstdio>
using namespace aquarium;
int main() {
  World w;
  constexpr float dt=1.f/120;
  // Ten minutes: alternating violent shaking and rest, plus repeated taps.
  for(int n=0;n<72000;++n) {
    float t=n*dt, shake=(n%3600<1200)?1.7f:0.f;
    w.sense(shake*std::sin(t*19),1+shake*std::sin(t*13),shake*std::cos(t*17),dt,shake*180*std::sin(t*11),shake*90*std::cos(t*7),shake*250*std::cos(t*19));
    if(n%719==0) w.ripple((n*37)%466,100);
    w.step(dt);
    float mean=0;
    for(float h:w.wave) { assert(std::isfinite(h)); assert(std::fabs(h)<97); mean+=h; }
    assert(std::fabs(mean)<.01f);
    for(auto f:w.fish) {
      assert(std::isfinite(f.x+f.y+f.vx+f.vy));
      assert(f.y>=w.surface(f.x)+19.9f);
      assert(std::hypot(f.x-233,f.y-233)<199.1f);
    }
  }
  World calm; calm.ripple(230,100);
  for(int n=0;n<4800;++n) calm.step(dt);
  for(float h:calm.wave) assert(std::fabs(h)<.02f);
  calm.sense(NAN,1,0,dt); assert(std::isfinite(calm.tilt));
  for(int n=0;n<1200;++n) calm.sense(.5f,1,0,dt);
  assert(calm.tilt<-.4f && calm.tilt>=-.43f);
  World rotation; float peak=0,after=0;
  for(int n=0;n<1200;++n) {
    float t=n*dt;
    rotation.sense(0,1,0,dt,0,0,n<240?200*std::cos(t*8):0);
    rotation.step(dt); peak=std::max(peak,std::fabs(rotation.slosh));
    if(n>240&&n<480)after=std::max(after,std::fabs(rotation.slosh));
  }
  assert(peak>25); assert(after>5); assert(std::fabs(rotation.slosh)<1);
  std::printf("Gyro-only: peak %.1f px, after-motion %.1f px\n",peak,after);
  puts("PASS: ten-minute shake/tap stress, fish containment, wave decay, volume conservation, invalid IMU input, tilt response");
}
