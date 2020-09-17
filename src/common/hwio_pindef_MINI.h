//----------------------------------------------------------------------------//
// pin names
#pragma once

#include "Pin.hpp"

#define MARLIN_PORT_Z_MIN   MARLIN_PORT_A
#define MARLIN_PIN_NR_Z_MIN MARLIN_PIN_NR_8
extern InputPin zMin;

#define MARLIN_PORT_X_DIAG   MARLIN_PORT_E
#define MARLIN_PIN_NR_X_DIAG MARLIN_PIN_NR_2
extern InputPin xDiag;

#define MARLIN_PORT_Y_DIAG   MARLIN_PORT_E
#define MARLIN_PIN_NR_Y_DIAG MARLIN_PIN_NR_1
extern InputPin yDiag;

#define MARLIN_PORT_Z_DIAG   MARLIN_PORT_E
#define MARLIN_PIN_NR_Z_DIAG MARLIN_PIN_NR_3
extern InputPin zDiag;

#define MARLIN_PORT_E0_DIAG   MARLIN_PORT_A
#define MARLIN_PIN_NR_E0_DIAG MARLIN_PIN_NR_15
extern InputPin e0Diag;

#define MARLIN_PORT_X_STEP   MARLIN_PORT_D
#define MARLIN_PIN_NR_X_STEP MARLIN_PIN_NR_1
extern OutputPin xStep;

#define MARLIN_PORT_X_DIR   MARLIN_PORT_D
#define MARLIN_PIN_NR_X_DIR MARLIN_PIN_NR_0
extern OutputPin xDir;

#define MARLIN_PORT_X_ENA   MARLIN_PORT_D
#define MARLIN_PIN_NR_X_ENA MARLIN_PIN_NR_3
extern OutputPin xEnable;

#define MARLIN_PORT_Y_STEP   MARLIN_PORT_D
#define MARLIN_PIN_NR_Y_STEP MARLIN_PIN_NR_13
extern OutputPin yStep;

#define MARLIN_PORT_Y_DIR   MARLIN_PORT_D
#define MARLIN_PIN_NR_Y_DIR MARLIN_PIN_NR_12
extern OutputPin yDir;

#define MARLIN_PORT_Y_ENA   MARLIN_PORT_D
#define MARLIN_PIN_NR_Y_ENA MARLIN_PIN_NR_14
extern OutputPin yEnable;

#define MARLIN_PORT_Z_STEP   MARLIN_PORT_D
#define MARLIN_PIN_NR_Z_STEP MARLIN_PIN_NR_4
extern OutputPin zStep;

#define MARLIN_PORT_Z_DIR   MARLIN_PORT_D
#define MARLIN_PIN_NR_Z_DIR MARLIN_PIN_NR_15
extern OutputPin zDir;

#define MARLIN_PORT_Z_ENA   MARLIN_PORT_D
#define MARLIN_PIN_NR_Z_ENA MARLIN_PIN_NR_2
extern OutputPin zEnable;

#define MARLIN_PORT_E0_STEP   MARLIN_PORT_D
#define MARLIN_PIN_NR_E0_STEP MARLIN_PIN_NR_9
extern OutputPin e0Step;

#define MARLIN_PORT_E0_DIR   MARLIN_PORT_B
#define MARLIN_PIN_NR_E0_DIR MARLIN_PIN_NR_8
extern OutputPin e0Dir;

#define MARLIN_PORT_E0_ENA   MARLIN_PORT_D
#define MARLIN_PIN_NR_E0_ENA MARLIN_PIN_NR_10
extern OutputPin e0Enable;

#define MARLIN_PORT_BED_HEAT   MARLIN_PORT_B
#define MARLIN_PIN_NR_BED_HEAT MARLIN_PIN_NR_0
extern OutputPin bedHeat;

#define MARLIN_PORT_HEAT0   MARLIN_PORT_B
#define MARLIN_PIN_NR_HEAT0 MARLIN_PIN_NR_1
extern OutputPin heat0;

#if 0 //not used outside of its definition scope
extern InputPin fastBoot;
extern InputPin jogWheelEN1;
extern InputPin jogWheelEN2;
extern InputPin jogWheelENC;
extern OutputPin cs;
extern OutputPin rs;
extern OutputInputPin rst;
extern InputPin fSensor;
#endif

#define MARLIN_PORT_FAN   MARLIN_PORT_E
#define MARLIN_PIN_NR_FAN MARLIN_PIN_NR_11

#define MARLIN_PORT_FAN1   MARLIN_PORT_E
#define MARLIN_PIN_NR_FAN1 MARLIN_PIN_NR_9

#define MARLIN_PORT_HW_IDENTIFY   MARLIN_PORT_A
#define MARLIN_PIN_NR_HW_IDENTIFY MARLIN_PIN_NR_3 //ADC, unused

#define MARLIN_PORT_TEMP_BED   MARLIN_PORT_A
#define MARLIN_PIN_NR_TEMP_BED MARLIN_PIN_NR_4 //ADC

#define MARLIN_PORT_THERM2   MARLIN_PORT_A
#define MARLIN_PIN_NR_THERM2 MARLIN_PIN_NR_5 //ADC

#define MARLIN_PORT_TEMP_HEATBREAK   MARLIN_PORT_A
#define MARLIN_PIN_NR_TEMP_HEATBREAK MARLIN_PIN_NR_6 //ADC

#define MARLIN_PORT_TEMP_0   MARLIN_PORT_C
#define MARLIN_PIN_NR_TEMP_0 MARLIN_PIN_NR_0 //ADC
