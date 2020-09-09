#pragma once

#include <Arduino.h>
#include "TMC_platforms.h"

class SSwitch
{
  public:
    SSwitch(uint16_t pin1,uint16_t pin2,uint8_t address);
    void active();
  private:
    uint16_t p1;
    uint16_t p2;
    uint8_t addr;
};

