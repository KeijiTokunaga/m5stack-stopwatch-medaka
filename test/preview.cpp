// Desktop reference rasterizer: runs the same World and Scene as the firmware.
#include "../src/World.h"
#include <cstdio>
#include <utility>
#include <cstdlib>
#include <string>
using namespace aquarium;
struct Raster {
  uint16_t pixels[466*466]{};
  void drawPixel(int x,int y,uint16_t c) { if(x>=0&&x<466&&y>=0&&y<466) pixels[y*466+x]=c; }
  void fillSprite(uint16_t c) { for(auto &p:pixels)p=c; }
  void drawFastHLine(int x,int y,int w,uint16_t c) { for(int i=0;i<w;++i)drawPixel(x+i,y,c); }
  void drawFastVLine(int x,int y,int h,uint16_t c) { for(int i=0;i<h;++i)drawPixel(x,y+i,c); }
  void drawLine(int x,int y,int bx,int by,uint16_t c) {
    int dx=std::abs(bx-x),sx=x<bx?1:-1,dy=-std::abs(by-y),sy=y<by?1:-1,e=dx+dy;
    for(;;) { drawPixel(x,y,c); if(x==bx&&y==by)break; int ee=e*2;if(ee>=dy){e+=dy;x+=sx;}if(ee<=dx){e+=dx;y+=sy;} }
  }
  void fillEllipse(int x,int y,int rx,int ry,uint16_t c) {
    for(int j=-ry;j<=ry;++j)for(int i=-rx;i<=rx;++i)if(float(i*i)/(rx*rx)+float(j*j)/(ry*ry)<=1)drawPixel(x+i,y+j,c);
  }
  void fillCircle(int x,int y,int r,uint16_t c) { fillEllipse(x,y,r,r,c); }
  void drawEllipse(int x,int y,int rx,int ry,uint16_t c) { for(int i=0;i<180;++i)drawPixel(x+std::cos(i*kPi/90)*rx,y+std::sin(i*kPi/90)*ry,c); }
  void fillTriangle(int ax,int ay,int bx,int by,int cx,int cy,uint16_t c) {
    auto cross=[](int x,int y,int u,int v,int p,int q){return (p-x)*(v-y)-(q-y)*(u-x);};
    int area=cross(ax,ay,bx,by,cx,cy); if(!area)return;
    for(int y=std::min(ay,std::min(by,cy));y<=std::max(ay,std::max(by,cy));++y)
      for(int x=std::min(ax,std::min(bx,cx));x<=std::max(ax,std::max(bx,cx));++x) {
        int a=cross(ax,ay,bx,by,x,y),b=cross(bx,by,cx,cy,x,y),d=cross(cx,cy,ax,ay,x,y);
        if((a>=0&&b>=0&&d>=0)||(a<=0&&b<=0&&d<=0))drawPixel(x,y,c);
      }
  }
  void setTextColor(uint16_t){} void setTextDatum(int){} void drawString(const char*,int,int,int){} void pushSprite(int,int){}
  void save(const char* path) {
    FILE *f=std::fopen(path,"wb"); if(!f)std::abort(); std::fprintf(f,"P6\n466 466\n255\n");
    for(int y=0;y<466;++y)for(int x=0;x<466;++x) {
      uint16_t p=pixels[y*466+x]; bool outside=(x-233)*(x-233)+(y-233)*(y-233)>232*232;
      unsigned char rgb[3]={uint8_t((p>>11)*255/31),uint8_t(((p>>5)&63)*255/63),uint8_t((p&31)*255/31)};
      if(outside)rgb[0]=rgb[1]=rgb[2]=8; std::fwrite(rgb,1,3,f);
    }
    std::fclose(f);
  }
} frame;
World world;
bool imu=true,night=false;
unsigned frames=0;
constexpr int middle_center=0;
uint16_t color(int r,int g,int b) { float k=night?.4f:1;return (int(clamp(r*k*world.tint[0],0,255))>>3)<<11|(int(clamp(g*k*world.tint[1],0,255))>>2)<<5|(int(clamp(b*k*world.tint[2],0,255))>>3); }
#include "../src/Scene.h"
int main(int argc,char**argv) {
  int count=argc>1?std::atoi(argv[1]):1;
  for(int i=0;i<480;++i) {world.sense(0,1,0,1.f/120);world.step(1.f/120);}
  bool ambientDemo=argc>2 && std::string(argv[2])=="ambient";
  bool feedingDemo=argc>2 && !ambientDemo;
  for(int n=0;n<count;++n) {
    if(feedingDemo && n==20) world.feed(155);
    if(feedingDemo && n==150) world.feed(311);
    for(int j=0;j<6;++j) {float t=n*.05f+j/120.f; bool moving=!ambientDemo && !feedingDemo && n>25&&n<80;
      world.sense(moving?.65f*std::sin(t*8):0,1,0,1.f/120,moving?100*std::sin(t*6):0,0,moving?210*std::cos(t*8):0);world.step(1.f/120);}
    if(ambientDemo) { world.lightClock=n*240.f/count;world.updateLight(0); }
    render();char path[128];std::snprintf(path,sizeof(path),"/tmp/medaka-frame-%03d.ppm",n);frame.save(path);
  }
}
