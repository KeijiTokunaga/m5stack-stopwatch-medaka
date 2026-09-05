#pragma once
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace aquarium {
constexpr float kPi = 3.14159265359f;
inline float clamp(float v, float lo, float hi) { return std::max(lo, std::min(v, hi)); }
enum class Species : uint8_t { Medaka, NeonTetra, Guppy };
struct SwimStyle { float cruise, turn, cadence, schooling, restDepth; };
inline SwimStyle swimStyle(Species s) {
  switch(s) {
    case Species::NeonTetra: return {49,3.2f,1.25f,1.8f,245};
    case Species::Guppy: return {34,1.9f,.72f,.65f,185};
    default: return {42,2.5f,1,1,215};
  }
}
struct Fish { float x, y, vx, vy, heading, phase, depth, urge; float bite=0; Species species=Species::Medaka; };
struct Food { float x=0, y=0, life=0; bool wet=false; };
class World {
public:
  static constexpr int N = 65, COUNT = 9, FOOD_MAX = 24;
  float wave[N]{}, velocity[N]{}, time = 0, tilt = 0, current = 0, alarm = 0;
  float gx = 0, gy = 1, gz = 0, activity = 0;
  float rollRate=0, pitchRate=0, yawRate=0, rotationalKick=0;
  float slosh=0, sloshSpeed=0, forcing=0, crossForcing=0;
  Fish fish[COUNT];
  Food food[FOOD_MAX];
  unsigned eaten=0;
  unsigned eatenBySpecies[3]{};
  float feedCooldown=0;
  int foodCount() const { int n=0; for(const auto &p:food) if(p.life>0) ++n; return n; }
  void feed(float x) {
    if(!std::isfinite(x) || feedCooldown>0) return;
    feedCooldown=.35f; int added=0;
    for(auto &p:food) if(p.life<=0 && added<6) {
      p.x=clamp(x+(random()-.5f)*44,110,356);
      p.y=std::max(18.f,surface(p.x)-26-random()*16);
      p.life=24; p.wet=false; ++added;
    }
  }
  uint32_t seed = 0x4d454441;
  float random() { seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5; return (seed & 0xffffff) / 16777216.f; }
  World() {
    int index=0;
    for (auto &f : fish) {
      f.species=static_cast<Species>(index++%3);
      f.x = 90 + random()*280; f.y = 155 + random()*210;
      f.heading = random()*2*kPi; f.vx = std::cos(f.heading)*20; f.vy = std::sin(f.heading)*8;
      f.phase = random()*2*kPi; f.depth = .6f + random()*.4f; f.urge = random();
    }
  }
  float surface(float x) const {
    float p = clamp(x / 465.f * (N-1), 0, N-1.001f);
    int i = int(p);
    return clamp(104 + tilt*(x-233) + slosh*std::cos(p*kPi/(N-1))
      + wave[i]*(1-(p-i)) + wave[i+1]*(p-i), 12, 240);
  }
  void ripple(float x, float power) {
    float p = clamp(x/465.f*(N-1), 0, N-1);
    for (int i=0;i<N;++i) velocity[i] += power*std::exp(-(i-p)*(i-p)/12.f);
  }
  void sense(float ax, float ay, float az, float dt, float rx=0, float ry=0, float rz=0) {
    if (!std::isfinite(ax+ay+az+rx+ry+rz) || dt<=0 || dt>.2f) return;
    // M5Unified gyro values are degrees/second. Z is rotation in the screen plane.
    float blend=1-std::exp(-dt*12), oldRoll=rollRate;
    rollRate+=(clamp(rz,-720,720)-rollRate)*blend;
    pitchRate+=(clamp(rx,-720,720)-pitchRate)*blend;
    yawRate+=(clamp(ry,-720,720)-yawRate)*blend;
    rotationalKick=clamp((rollRate-oldRoll)/dt,-2400,2400);
    float k = 1-std::exp(-dt*3);
    gx += (ax-gx)*k; gy += (ay-gy)*k; gz += (az-gz)*k;
    float dx = ax-gx, dy = ay-gy, dz = az-gz;
    float impulse = clamp(std::sqrt(dx*dx+dy*dy+dz*dz)-.07f, 0, 2);
    activity += (impulse-activity)*(1-std::exp(-dt*8));
    // Display rotation 0: acceleration +X raises the right-hand surface.
    float target = clamp(-gx / std::max(.35f, std::fabs(gy)), -.43f, .43f);
    tilt += (target-tilt)*(1-std::exp(-dt*2.2f));
    current += (clamp(-dx*160-rollRate*.25f,-180,180)-current)*(1-std::exp(-dt*4));
    // Held forces are integrated at the fixed physics rate, independent of rendering.
    forcing=clamp(dx*700-rollRate*4.5f-rotationalKick*.7f,-1600,1600);
    crossForcing=clamp(dy*280+dz*220+pitchRate*1.7f+yawRate*1.2f,-750,750);
    alarm=std::max(alarm,clamp((std::fabs(rollRate)+std::fabs(pitchRate)+std::fabs(yawRate))/500,0,1));
    alarm = std::max(alarm, clamp(impulse*.7f,0,1));
  }
  void step(float dt) {
    time += dt; alarm *= std::exp(-dt*1.4f);
    sloshSpeed+=(forcing-slosh*22-sloshSpeed*1.5f)*dt;
    slosh+=sloshSpeed*dt;
    if(std::fabs(slosh)>66) { slosh=clamp(slosh,-66,66); if(slosh*sloshSpeed>0)sloshSpeed*=-.25f; }
    // Sensor loss must not leave a permanent force acting on the water.
    forcing*=std::exp(-dt*3); crossForcing*=std::exp(-dt*3);
    float next[N];
    for (int i=0;i<N;++i) {
      float l = wave[i ? i-1 : 1], r = wave[i<N-1 ? i+1 : N-2];
      float p=i/(N-1.f);
      float drive=forcing*.35f*std::cos(3*kPi*p)+crossForcing*std::cos(2*kPi*p);
      next[i] = (velocity[i]+((l+r-2*wave[i])*250 - wave[i]*4.f+drive)*dt)*std::exp(-dt*.72f);
    }
    float mean = 0;
    for(int i=0;i<N;++i) { velocity[i]=next[i]; wave[i]=clamp(wave[i]+velocity[i]*dt,-48,48); mean+=wave[i]; }
    for(auto &h:wave) h -= mean/N; // Preserve water volume after impulses.
    feedCooldown=std::max(0.f,feedCooldown-dt);
    for(auto &p:food) if(p.life>0) {
      p.life-=dt;
      if(!p.wet) {
        p.y+=90*dt;
        if(p.y>=surface(p.x)+26) { p.wet=true; ripple(p.x,22); }
      } else {
        p.x=clamp(p.x+(current*.05f+std::sin(time+p.x*.03f)*1.5f)*dt,105,361);
        p.y=std::max(surface(p.x)+26,p.y+2.8f*dt);
      }
      if(p.y>395) p.life=0;
    }
    Fish old[COUNT]; std::copy(fish,fish+COUNT,old);
    for(int i=0;i<COUNT;++i) {
      auto &f=fish[i]; const auto style=swimStyle(f.species); float fx=0,fy=0, mx=0,my=0,avx=0,avy=0; int neighbors=0;
      for(int j=0;j<COUNT;++j) if(i!=j) {
        float dx=old[j].x-f.x,dy=old[j].y-f.y,d2=dx*dx+dy*dy;
        if(d2<150*150 && old[j].species==f.species) { mx+=old[j].x; my+=old[j].y; avx+=old[j].vx; avy+=old[j].vy; ++neighbors; }
        if(d2<42*42) { fx-=dx*150/(d2+16); fy-=dy*150/(d2+16); }
      }
      if(neighbors) { fx+=((mx/neighbors-f.x)*.08f+(avx/neighbors-f.vx)*.16f)*style.schooling; fy+=((my/neighbors-f.y)*.07f+(avy/neighbors-f.vy)*.12f)*style.schooling; }
      f.bite=std::max(0.f,f.bite-dt);
      int targetFood=-1; float nearest=1e9f;
      for(int j=0;j<FOOD_MAX;++j) if(food[j].life>0 && food[j].wet) {
        float dx=food[j].x-f.x,dy=food[j].y-f.y,d2=dx*dx+dy*dy;
        if(d2<nearest) { nearest=d2; targetFood=j; }
      }
      bool feeding=targetFood>=0;
      if(feeding) {
        auto &p=food[targetFood]; float d=std::sqrt(nearest);
        if(d<17 && f.bite<=0) { p.life=0; ++eaten; ++eatenBySpecies[static_cast<int>(f.species)]; f.bite=.65f; }
        else {
          float desired=std::min(68.f,d*2.4f);
          fx+=((p.x-f.x)/std::max(1.f,d)*desired-f.vx)*2.8f;
          fy+=((p.y-f.y)/std::max(1.f,d)*desired-f.vy)*2.8f;
        }
      }
      if(!feeding) fy+=(style.restDepth-f.y)*.035f;
      float radial=std::sqrt((f.x-233)*(f.x-233)+(f.y-233)*(f.y-233));
      if(radial>166) { fx-=(f.x-233)*(radial-166)*.025f; fy-=(f.y-233)*(radial-166)*.025f; }
      float top=surface(f.x)+(feeding?0:32);
      if(f.y<top+25) fy+=(top+25-f.y)*1.8f;
      if(f.y>382) fy-=(f.y-382)*1.8f;
      f.urge-=dt;
      if(f.urge<0) f.urge=(.7f+random()*1.9f)/style.cadence;
      float burst = f.urge<.27f ? 1.f : .12f;
      fx+=std::cos(f.heading)*(12*style.cadence+alarm*60)*burst+std::sin(time*.63f+i*2)*8;
      fy+=std::sin(time*.81f+i*3)*5+alarm*14;
      f.vx+=(fx-f.vx*.20f+current*.28f)*dt; f.vy+=(fy-f.vy*.38f)*dt;
      float speed=std::sqrt(f.vx*f.vx+f.vy*f.vy), limit=(feeding?72:style.cruise)+alarm*65;
      if(speed>limit) { f.vx*=limit/speed; f.vy*=limit/speed; }
      if(speed>2) { float delta=std::atan2(f.vy,f.vx)-f.heading; delta=std::atan2(std::sin(delta),std::cos(delta)); f.heading+=clamp(delta,-dt*style.turn,dt*style.turn); }
      f.x+=f.vx*dt; f.y+=f.vy*dt;
      float dx=f.x-233,dy=f.y-233,d=std::sqrt(dx*dx+dy*dy);
      if(d>199) { f.x=233+dx*199/d; f.y=233+dy*199/d; }
      f.x=clamp(f.x,45,421);
      float edge=std::sqrt(199*199-(f.x-233)*(f.x-233));
      f.y=clamp(f.y,std::max(surface(f.x)+20,233-edge),std::min(413.f,233+edge));
      f.phase+=dt*(6+speed*.18f+burst*5)*style.cadence;
    }
  }
};
}
