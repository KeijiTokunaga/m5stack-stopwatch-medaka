#include "../src/Screens.h"
#include <cassert>
#include <cstdio>
int main() {
  aquarium::Screens s;
  assert(s.yellow(false,true) && !s.battery);
  assert(!s.yellow(true,false) && s.battery);
  assert(!s.yellow(false,false) && s.battery);
  assert(!s.yellow(true,false) && s.battery);
  assert(!s.yellow(false,true) && !s.battery);
  assert(s.yellow(false,true));
  assert(!s.yellow(true,true) && s.battery);
  puts("PASS: long press opens; short press returns without feeding; repeat hold stays; aquarium click feeds");
}
