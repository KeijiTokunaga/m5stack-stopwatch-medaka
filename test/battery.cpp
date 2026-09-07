#include "../src/Battery.h"
#include <cassert>
#include <cstdio>
int main() {
 aquarium::Battery b;
 assert(b.update(-1,false)==-1);
 assert(b.update(70,false)==70);
 assert(b.update(80,false)==70);
 assert(b.update(68,false)==68);
 assert(b.update(75,false)==68);
 assert(b.update(-2,false)==68);
 assert(b.update(101,true)==68);
 assert(b.update(85,true)==85);
 assert(b.update(83,true)==83);
 assert(b.update(90,false)==83);
 assert(b.update(0,false)==0);
 assert(b.update(4,false)==0);
 assert(b.update(5,true)==5);
 puts("PASS: discharge increases ignored, decreases accepted, charge increases accepted, invalid readings retained");
}
