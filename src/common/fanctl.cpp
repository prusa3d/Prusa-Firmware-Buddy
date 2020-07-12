// fanctl.cpp

#include "fanctl.h"
#include "stm32f4xx_hal.h"
#include "gpio.h"

// global variables for C wrapper
static uint8_t CFanCtl_count = 0;                  // number of instances
static CFanCtl* CFanCtl_instance[FANCTL_MAX_FANS]; // array of pointers to instances

// constructor
CFanCtl::CFanCtl(uint8_t pinOut, uint8_t pinTach, uint8_t minPWM, uint8_t maxPWM, uint16_t minRPM, uint16_t maxRPM) {
	m_pinOut = pinOut;
	m_pinTach = pinTach;
	m_MinPWM = minPWM;
	m_MaxPWM = maxPWM;
	m_MinRPM = minRPM;
	m_MaxRPM = maxRPM;
	m_State = idle;
	m_PWM = 0;
	m_ActualRPM = 0;
	m_TargetRPM = 0;
	m_pwm_out = false;
	m_pwm_val = 0;
	m_pwm_pha = 0;
	m_pwm_pha_max = 0;
	m_pwm_pha_stp = 0;
	m_pwm_cnt = 0;
	m_tach = false;
	m_tach_edge = 0;
	m_fan_rpm100 = 0;
	m_tach_time = 0;
	m_tach_max = 600;
	// this is not thread-safe for first look, but CFanCtl instances are global variables, so it is safe
	if (CFanCtl_count < FANCTL_MAX_FANS)
		CFanCtl_instance[CFanCtl_count++] = this;
}

void CFanCtl::init() {
    gpio_init(m_pinOut, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_FREQ_HIGH);
    gpio_init(m_pinTach, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_HIGH);
    gpio_set(m_pinOut, 0);
}

CFanCtl::FanState CFanCtl::getState() {
	return m_State;
}

uint8_t CFanCtl::getMinPWM() {
	return m_MinPWM;
}

uint8_t CFanCtl::getMaxPWM() {
	return m_MaxPWM;
}

uint16_t CFanCtl::getMinRPM() {
	return m_MinRPM;
}

uint16_t CFanCtl::getMaxRPM() {
	return m_MaxRPM;
}

uint8_t CFanCtl::getPWM() {
	return m_PWM;
}

void CFanCtl::setPWM(uint8_t pwm) {
	if (pwm > m_MaxPWM) pwm = m_MaxPWM;
	if (pwm && (pwm < m_MinPWM)) pwm = m_MinPWM;
	m_PWM = pwm;
	m_TargetRPM = 0;
}

uint16_t CFanCtl::getActualRPM() {
	return m_ActualRPM;
}

uint16_t CFanCtl::getTargetRPM() {
	return m_TargetRPM;
}

void CFanCtl::setTargetRPM(uint16_t targetRPM) {
	if (targetRPM > m_MaxPWM) targetRPM = m_MaxPWM;
	if (targetRPM && (targetRPM < m_MinPWM)) targetRPM = m_MinPWM;
	m_TargetRPM = targetRPM;
	m_PWM = 0;
}

void CFanCtl::tick1ms() {
	// PWM Control
	bool pwmOut = (m_pwm_cnt >= m_pwm_pha) && (m_pwm_cnt < (m_pwm_pha + m_pwm_val));
	if (++m_pwm_cnt >= m_MaxPWM)
	{
		m_pwm_cnt = 0;
		if (m_pwm_val != m_PWM)
		{
			m_pwm_val = m_PWM;
			m_pwm_pha = 0;
			m_pwm_pha_max = m_MaxPWM - m_pwm_val;
			m_pwm_pha_stp = 1;
		}
		else
		{
			if (m_pwm_pha_stp > 0)
			{
				if (++m_pwm_pha >= m_pwm_pha_max)
				{
					m_pwm_pha_stp = -1;
					m_pwm_pha--;
				}
			}
			if (m_pwm_pha_stp < 0)
			{
				if (m_pwm_pha-- == 0)
				{
					m_pwm_pha_stp = 1;
					m_pwm_pha = 0;
				}
			}
		}
	}
	if (pwmOut != m_pwm_out) // pwm output changed?
		gpio_set(m_pinOut, pwmOut?1:0); // set output pin
	m_pwm_out = pwmOut; // store pwm output state
	//RPM Measure
	bool tach = gpio_get(m_pinTach)?true:false; // sample tach input pin
	bool edge = (m_tach ^ tach) & m_pwm_out; // detect edge - ignore edges for pwmOut = 0
	m_tach = tach; // store current tach input state
	if (m_pwm_cnt > (2 + m_pwm_pha)) // ignore first two sub-periods after 0-1 pwm transition
		if ((m_tach_edge < 255) && edge) m_tach_edge++;
	if (++m_tach_time >= m_tach_max)
	{
		m_fan_rpm100 = ((uint16_t)m_tach_edge + m_fan_rpm100) >> 1;
		m_tach_edge = 0; // reset edge counter
		m_tach_time = 0; // reset tach cycle counter
		m_ActualRPM = 60 * (uint16_t)m_fan_rpm100 / 2;
	}
}


extern "C" {

void fanctl_init(void) {
	for (uint8_t fan = 0; fan < CFanCtl_count; fan++)
		CFanCtl_instance[fan]->init();
}

void fanctl_tick1ms(void) {
	for (uint8_t fan = 0; fan < CFanCtl_count; fan++)
		CFanCtl_instance[fan]->tick1ms();
}

void fanctl_set_pwm(uint8_t fan, uint8_t pwm) {
	if (fan < CFanCtl_count)
		CFanCtl_instance[fan]->setPWM(pwm);
}

void fanctl_set_rpm(uint8_t fan, uint16_t rpm) {
	if (fan < CFanCtl_count)
		CFanCtl_instance[fan]->setTargetRPM(rpm);
}

}
