//----------------------------------------------------------------------------//
// hwio.h - hardware input output abstraction
#ifndef _HWIO_H
#define _HWIO_H

#include <inttypes.h>

//low level I/O classes
#define HWIO_CLS_DI  0x01 //class digital input
#define HWIO_CLS_DO  0x02 //class digital output
#define HWIO_CLS_ADC 0x03 //class analog input
#define HWIO_CLS_DAC 0x04 //class analog output
#define HWIO_CLS_PWM 0x05 //class pwm output

//high level I/O classes
#define HWIO_CLS_FAN    0x10 //class fan controller
#define HWIO_CLS_HEATER 0x11 //class heater controller

//pwm outputs
#define HWIO_PWM_HEATER_BED 0 //BED PWM
#define HWIO_PWM_HEATER_0   1 //NOZZLE PWM
#define HWIO_PWM_FAN1       2 //PRINT FAN?
#define HWIO_PWM_FAN        3 //NOZZLE FAN?

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

//a3ides analog inputs
enum {
    ADC_HW_IDENTIFY,    //!< PA3 - chan 3
    ADC_TEMP_BED,       //!< PA4 - chan 4
    ADC_TEMP_2,         //!< PA5 - chan 5
    ADC_TEMP_HEATBREAK, //!< PA6 - chan 6
    ADC_TEMP_0,         //!< PC0 - chan 10
    ONE_BEHIND_LAST_ADC,
} typedef ADC_t;

//--------------------------------------
// low level I/O functions

//analog inputs
extern int hwio_adc_get_cnt(void);        //number of analog inputs
extern int hwio_adc_get_max(int i_adc);   //analog input maximum value
extern int hwio_adc_get_val(ADC_t i_adc); //read analog input
extern void adc_ready(uint8_t index);
extern uint8_t adc_seq2idx(uint8_t seq);

//analog outputs
extern int hwio_dac_get_cnt(void);                //number of analog outputs
extern int hwio_dac_get_max(int i_dac);           //analog output maximum value
extern void hwio_dac_set_val(int i_dac, int val); //write analog output

//pwm outputs
extern int hwio_pwm_get_cnt(void);                     //number of pwm outputs
extern int hwio_pwm_get_max(int i_pwm);                //pwm output maximum value
extern void hwio_pwm_set_val(int i_pwm, uint32_t val); //write pwm output

extern int hwio_pwm_get_val(int i_pwm);                       //get pwm value, returns 0 if stopped
extern void hwio_pwm_set_period_us(int i_pwm, int T_us);      //changes period of timer, can affect multiple channels
extern int hwio_pwm_get_period_us(int i_pwm);                 //gets period of timer in micro seconds
extern void hwio_pwm_set_prescaler(int i_pwm, int prescaler); //directly sets prescaler
extern int hwio_pwm_get_prescaler(int i_pwm);                 //gets value of prescaler
extern void hwio_pwm_set_prescaler_exp2(int i_pwm, int exp);  //changes prescaler (2^n)-1 ... 1.5us, 3us, 6us, 12us ...
extern int hwio_pwm_get_prescaler_log2(int i_pwm);            //gets exponent of prescaler - reversion of hwio_pwm_set_prescaler_exp2

// arduino compatibility functions
//--------------------------------------
// high level I/O functions

//fans
extern int hwio_fan_get_cnt(void); //number of fans
extern void hwio_fan_set_pwm(int i_fan, int val);

//heaters
extern int hwio_heater_get_cnt(void);                   //number of heaters
extern void hwio_heater_set_pwm(int i_heater, int val); //pwm output maximum value

//--------------------------------------
// misc I/O functions

//jogwheel
extern void hwio_jogwheel_enable(void);
extern void hwio_jogwheel_disable(void);

//tone
extern float hwio_beeper_get_vol(void);
extern void hwio_beeper_set_vol(float vol);
extern void hwio_beeper_set_pwm(uint32_t per, uint32_t pul);
extern void hwio_beeper_tone(float frq, uint32_t del);
extern void hwio_beeper_tone2(float frq, uint32_t del, float vol);
extern void hwio_beeper_notone(void);

//cycle 1ms
extern void hwio_update_1ms(void);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_HWIO_H
