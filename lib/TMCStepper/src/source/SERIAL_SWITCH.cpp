#include "SERIAL_SWITCH.h"

#ifdef TMC_SERIAL_SWITCH
SSwitch::SSwitch(uint16_t pin1,uint16_t pin2,uint8_t address) :
  p1(pin1),
  p2(pin2),
  addr(address)
	{
		pinMode(pin1, OUTPUT);		
    pinMode(pin2, OUTPUT);
	}

void SSwitch::active() {
  switch(addr) {
    case 0:
      writePIN_L(p1);
      writePIN_L(p2);
      break;

    case 1:
      writePIN_H(p1);
      writePIN_L(p2);
      break;

    case 2:
      writePIN_L(p1);
      writePIN_H(p2);
      break;

    case 3:
      writePIN_H(p1);
      writePIN_H(p2);
      break;

    default:
      writePIN_L(p1);
      writePIN_L(p2);
      break;
  }
}

#endif
