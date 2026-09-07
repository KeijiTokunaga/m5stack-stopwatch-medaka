#include "../src/Energy.h"
#include <cassert>
#include <cstdio>
using namespace aquarium;
int main() {
  Energy e;e.begin(100);
  e.update(60099,false);assert(!e.eco);
  e.update(60100,false);assert(e.eco && e.brightness(false)==45 && e.frameInterval()==200);
  e.update(60200,true);assert(!e.eco && e.brightness(false)==150 && e.brightness(true)==70);
  e.update(120200,false);assert(e.eco && e.brightness(true)==22);
  e.begin(UINT32_MAX-30000);e.update(29998,false);assert(!e.eco);
  e.update(29999,false);assert(e.eco);e.update(30000,true);assert(!e.eco);
  assert(!e.motion(0,1,0,0,0,0));assert(!e.motion(.001f,1,0,.1f,.1f,.1f));
  assert(e.motion(.2f,1,0,0,0,0));assert(e.motion(.2f,1,0,0,0,20));
  assert(!e.motion(NAN,1,0,0,0,0));
  puts("PASS: idle boundary, wake, dim-mode restore, clock rollover, motion and noise rejection");
}
