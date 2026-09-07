#pragma once
namespace aquarium {
struct Screens {
  bool battery=false;
  bool yellow(bool held,bool clicked) {
    if(held) {battery=true;return false;}
    if(!clicked)return false;
    if(battery) {battery=false;return false;}
    return true;
  }
};
}
