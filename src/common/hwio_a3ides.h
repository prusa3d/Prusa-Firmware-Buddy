// hwio_a3ides.h

#pragma once

//a3ides digital inputs
static const unsigned char _DI_Z_MIN = 0;   // PA8
static const unsigned char _DI_E_DIAG = 1;  // PA15
static const unsigned char _DI_Y_DIAG = 2;  // PE1
static const unsigned char _DI_X_DIAG = 3;  // PE2
static const unsigned char _DI_Z_DIAG = 4;  // PE3
static const unsigned char _DI_BTN_ENC = 5; // PE12
static const unsigned char _DI_BTN_EN1 = 6; // PE13
static const unsigned char _DI_BTN_EN2 = 7; // PE15

//a3ides digital outputs
static const unsigned char _DO_X_DIR = 0;     // PD0
static const unsigned char _DO_X_STEP = 1;    // PD1
static const unsigned char _DO_Z_ENABLE = 2;  // PD2
static const unsigned char _DO_X_ENABLE = 3;  // PD3
static const unsigned char _DO_Z_STEP = 4;    // PD4
static const unsigned char _DO_E_DIR = 5;     // PD8
static const unsigned char _DO_E_STEP = 6;    // PD9
static const unsigned char _DO_E_ENABLE = 7;  // PD10
static const unsigned char _DO_Y_DIR = 8;     // PD12
static const unsigned char _DO_Y_STEP = 9;    // PD13
static const unsigned char _DO_Y_ENABLE = 10; // PD14
static const unsigned char _DO_Z_DIR = 11;    // PD15

//a3ides analog inputs
static const unsigned char _ADC_HW_IDENTIFY = 0;    // PA3 - chan 3
static const unsigned char _ADC_TEMP_BED = 1;       // PA4 - chan 4
static const unsigned char _ADC_TEMP_2 = 2;         // PA5 - chan 5
static const unsigned char _ADC_TEMP_HEATBREAK = 3; // PA6 - chan 6
static const unsigned char _ADC_TEMP_0 = 4;         // PC0 - chan 10

//a3ides pwm outputs
static const unsigned char _PWM_HEATER_BED = 0; //
static const unsigned char _PWM_HEATER_0 = 1;   //
static const unsigned char _PWM_FAN1 = 2;       //
static const unsigned char _PWM_FAN = 3;        //

//a3ides fan control
static const unsigned char _FAN = 0;  //
static const unsigned char _FAN1 = 1; //

//a3ides heater control
static const unsigned char _HEATER_0 = 0;   //
static const unsigned char _HEATER_BED = 1; //
