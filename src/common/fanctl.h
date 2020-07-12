// fanctl.h
#ifndef _FANCTL_H
#define _FANCTL_H

#include <inttypes.h>

#define FANCTL_MAX_FANS 2 // maximum number of fans for C wrapper functions


#ifdef __cplusplus

class CFanCtl {
public:
	enum FanState{
		idle,              // idle - no rotation, PWM = 0%
		starting,          // starting - PWM = 100%, waiting for tacho pulse
		running,           // running - PWM set by setPWM(), no regulation
		closedloop,        // closed-loop - PWM set by internal regulator
		error_starting,    // starting error - means no feedback after timeout expired
		error_running,     // running error - means zero RPM measured (no feedback)
		error_closedloop,  // closed-loop error - means unable to reach target rpm with desired tolerance limit
	};
public:
	// constructor
	CFanCtl(uint8_t pinOut, uint8_t pinTach, uint8_t minPWM, uint8_t maxPWM, uint16_t minRPM, uint16_t maxRPM);
	void init();                           // init function - initialize hw
	uint8_t getMinPWM();                   // get minimum PWM, this should be safe value for self starting
	uint8_t getMaxPWM();                   // get maximum PWM, this is value representing 100% power
	uint16_t getMinRPM();                  // get minimum RPM [n/min], this is lowest RPM that can be reached with reliable response
	uint16_t getMaxRPM();                  // get maximup RPM [n/min], this is highest RPM at 100% power
	FanState getState();                   // get fan control state
	uint8_t getPWM();                      // get PWM value
	void setPWM(uint8_t pwm);              // set PWM value - switch to non closed-loop mode
	uint16_t getActualRPM();               // get actual (measured) RPM
	uint16_t getTargetRPM();               // get target RPM
	void setTargetRPM(uint16_t targetRPM); // set target RPM - switch to closed-loop mode
	void tick1ms();                        // 1ms callback from timer interrupt
private:
	uint8_t m_pinOut;      // output control pin, pwm output (set in constructor)
	uint8_t m_pinTach;     // input tacho pin (set in constructor)
	uint8_t m_MinPWM;      // minimum pwm value (set in constructor)
	uint8_t m_MaxPWM;      // maximum pwm value (set in constructor)
	uint16_t m_MinRPM;     // minimum rpm value (set in constructor)
	uint16_t m_MaxRPM;     // maximum rpm value (set in constructor)
	FanState m_State;      // fan control state
	uint8_t m_PWM;         // pwm value
	uint16_t m_ActualRPM;  // actual rpm value (measured)
	uint16_t m_TargetRPM;  // target rpm value for closed-loop mode
	bool m_pwm_out;        // cached pwm output state (0/1)
	uint8_t m_pwm_val;     // pwm value (cached during pwm cycle)
	uint8_t m_pwm_pha;     // pwm phase shift
	uint8_t m_pwm_pha_max; // pwm phase shift maximum
	int8_t m_pwm_pha_stp;  // pwm phase shift step
	uint8_t m_pwm_cnt;     // pwm counter (value 0..m_MaxPWM-1)
	bool m_tach;           // cached tach input state (0/1)
	uint8_t m_tach_edge;   // edge counter
	uint8_t m_fan_rpm100;  // rpm value (filtered)
	uint16_t m_tach_time;  // tach cycle counter
	uint16_t m_tach_max;   // tach cycle length
};


extern "C" {
#endif //__cplusplus

// C wrapper functions
extern void fanctl_init(void);                         // init for all fanctl instances is done using this function in appmain.cpp
extern void fanctl_tick1ms(void);                      // tick for all fanctl instances is done using this function in appmain.cpp
extern void fanctl_set_pwm(uint8_t fan, uint8_t pwm);  // set PWM
extern void fanctl_set_rpm(uint8_t fan, uint16_t rpm); // set RPM

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // _FANCTL_H
