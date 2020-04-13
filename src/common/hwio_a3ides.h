// hwio_a3ides.h

#ifndef _HWIO_A3IDES_H
#define _HWIO_A3IDES_H

//a3ides digital inputs
#define _DI_Z_MIN   0 // PA8
#define _DI_E_DIAG  1 // PA15
#define _DI_Y_DIAG  2 // PE1
#define _DI_X_DIAG  3 // PE2
#define _DI_Z_DIAG  4 // PE3
#define _DI_BTN_ENC 5 // PE12
#define _DI_BTN_EN1 6 // PE13
#define _DI_BTN_EN2 7 // PE15

//a3ides digital outputs
#define _DO_X_DIR    0  // PD0
#define _DO_X_STEP   1  // PD1
#define _DO_Z_ENABLE 2  // PD2
#define _DO_X_ENABLE 3  // PD3
#define _DO_Z_STEP   4  // PD4
#define _DO_E_DIR    5  // PD8
#define _DO_E_STEP   6  // PD9
#define _DO_E_ENABLE 7  // PD10
#define _DO_Y_DIR    8  // PD12
#define _DO_Y_STEP   9  // PD13
#define _DO_Y_ENABLE 10 // PD14
#define _DO_Z_DIR    11 // PD15

//a3ides analog inputs
#define _ADC_HW_IDENTIFY    0 // PA3 - chan 3
#define _ADC_TEMP_BED       1 // PA4 - chan 4
#define _ADC_TEMP_2         2 // PA5 - chan 5
#define _ADC_TEMP_HEATBREAK 3 // PA6 - chan 6
#define _ADC_TEMP_0         4 // PC0 - chan 10

//a3ides pwm outputs
#define _PWM_HEATER_BED 0 //
#define _PWM_HEATER_0   1 //
#define _PWM_FAN1       2 //
#define _PWM_FAN        3 //

//a3ides fan control
#define _FAN  0 //
#define _FAN1 1 //

//a3ides heater control
#define _HEATER_0   0 //
#define _HEATER_BED 1 //

#endif // _HWIO_A3IDES_H
