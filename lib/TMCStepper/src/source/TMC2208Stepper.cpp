#include "TMCStepper.h"
#include "TMC_MACROS.h"

#ifdef TMC_SERIAL_SWITCH
  #include "SERIAL_SWITCH.h"
#endif

// Protected
// addr needed for TMC2209
#ifdef TMC_SERIAL_SWITCH
  TMC2208Stepper::TMC2208Stepper(Stream * SerialPort, float RS, uint8_t addr, uint16_t mul_pin1, uint16_t mul_pin2) :
  	TMCStepper(RS),
  	slave_address(TMC2208_SLAVE_ADDR),
  	write_only(false)
  	{
  	  SSwitch *SMulObj = new SSwitch(mul_pin1,mul_pin2,addr);
  	  sswitch = SMulObj;
  		HWSerial = SerialPort;
  		defaults();
  	}
#endif

TMC2208Stepper::TMC2208Stepper(Stream * SerialPort, float RS, uint8_t addr) :
	TMCStepper(RS),
	slave_address(addr),
	write_only(false)
	{
		HWSerial = SerialPort;
		defaults();
	}

#if SW_CAPABLE_PLATFORM
	// Protected
	// addr needed for TMC2209
	TMC2208Stepper::TMC2208Stepper(uint16_t SW_RX_pin, uint16_t SW_TX_pin, float RS, uint8_t addr, bool has_rx) :
		TMCStepper(RS),
		slave_address(addr),
		write_only(!has_rx)
		{
			SoftwareSerial *SWSerialObj = new SoftwareSerial(SW_RX_pin, SW_TX_pin);
			SWSerial = SWSerialObj;
			defaults();
		}

	void TMC2208Stepper::beginSerial(uint32_t baudrate) {
		if (SWSerial != NULL) SWSerial->begin(baudrate);
	}
#endif

void TMC2208Stepper::begin() {
	#if SW_CAPABLE_PLATFORM
		beginSerial(115200);
	#endif
	pdn_disable(true);
	mstep_reg_select(true);

}

void TMC2208Stepper::defaults() {
	GCONF_register.i_scale_analog = 1;
	GCONF_register.internal_rsense = 0; // OTP
	GCONF_register.en_spreadcycle = 0; // OTP
	GCONF_register.multistep_filt = 1; // OTP
	IHOLD_IRUN_register.iholddelay = 1; // OTP
	TPOWERDOWN_register.sr = 20;
	CHOPCONF_register.sr = 0x10000053;
	PWMCONF_register.sr = 0xC10D0024;
  //MSLUT0_register.sr = ???;
  //MSLUT1_register.sr = ???;
  //MSLUT2_register.sr = ???;
  //MSLUT3_register.sr = ???;
  //MSLUT4_register.sr = ???;
  //MSLUT5_register.sr = ???;
  //MSLUT6_register.sr = ???;
  //MSLUT7_register.sr = ???;
  //MSLUTSTART_register.start_sin90 = 247;
}

void TMC2208Stepper::push() {
	GCONF(GCONF_register.sr);
	IHOLD_IRUN(IHOLD_IRUN_register.sr);
	SLAVECONF(SLAVECONF_register.sr);
	TPOWERDOWN(TPOWERDOWN_register.sr);
	TPWMTHRS(TPWMTHRS_register.sr);
	VACTUAL(VACTUAL_register.sr);
	CHOPCONF(CHOPCONF_register.sr);
	PWMCONF(PWMCONF_register.sr);
}

bool TMC2208Stepper::isEnabled() { return !enn() && toff(); }

uint8_t TMC2208Stepper::calcCRC(uint8_t datagram[], uint8_t len) {
	uint8_t crc = 0;
	for (uint8_t i = 0; i < len; i++) {
		uint8_t currentByte = datagram[i];
		for (uint8_t j = 0; j < 8; j++) {
			if ((crc >> 7) ^ (currentByte & 0x01)) {
				crc = (crc << 1) ^ 0x07;
			} else {
				crc = (crc << 1);
			}
			crc &= 0xff;
			currentByte = currentByte >> 1;
		}
	}
	return crc;
}

void TMC2208Stepper::write(uint8_t addr, uint32_t regVal) {
	uint8_t len = 7;
	addr |= TMC_WRITE;
	uint8_t datagram[] = {TMC2208_SYNC, slave_address, addr, (uint8_t)(regVal>>24), (uint8_t)(regVal>>16), (uint8_t)(regVal>>8), (uint8_t)(regVal>>0), 0x00};

	datagram[len] = calcCRC(datagram, len);

	#if SW_CAPABLE_PLATFORM
		if (SWSerial != NULL) {
				for(int i=0; i<=len; i++){
					bytesWritten += SWSerial->write(datagram[i]);
				}
		} else
	#endif
		{
		  #ifdef TMC_SERIAL_SWITCH
		    sswitch->active();
		  #endif
			for(int i=0; i<=len; i++){			
				bytesWritten += HWSerial->write(datagram[i]);
		}
	}
	delay(replyDelay);
}

template<typename SERIAL_TYPE>
uint64_t _sendDatagram(SERIAL_TYPE &serPtr, uint8_t datagram[], const uint8_t len, uint16_t timeout) {
	while (serPtr.available() > 0) serPtr.read(); // Flush
	for(int i=0; i<=len; i++) serPtr.write(datagram[i]);
	delay(TMC2208Stepper::replyDelay);

	// scan for the rx frame and read it
	uint32_t ms = millis();
	uint32_t sync_target = ((uint32_t)datagram[0]<<16) | 0xFF00 | datagram[2];
	uint32_t sync = 0;

	do {
		uint32_t ms2 = millis();
		if (ms2 != ms) {
			// 1ms tick
			ms = ms2;
			timeout--;
		}
		if (!timeout) return 0;

		int16_t res = serPtr.read();
		if (res < 0) continue;

		sync <<= 8;
		sync |= res & 0xFF;
		sync &= 0xFFFFFF;

	} while (sync != sync_target);

	uint64_t out = sync;
	ms = millis();
	timeout = TMC2208Stepper::abort_window;

	for(uint8_t i=0; i<5;) {
		uint32_t ms2 = millis();
		if (ms2 != ms) {
			// 1ms tick
			ms = ms2;
			timeout--;
		}
		if (!timeout) return 0;

		int16_t res = serPtr.read();
		if (res < 0) continue;

		out <<= 8;
		out |= res & 0xFF;

		i++;
	}

	while (serPtr.available() > 0) serPtr.read(); // Flush
	return out;
}

uint32_t TMC2208Stepper::read(uint8_t addr) {
	constexpr uint8_t len = 3;
	addr |= TMC_READ;
	uint8_t datagram[] = {TMC2208_SYNC, slave_address, addr, 0x00};
	datagram[len] = calcCRC(datagram, len);
	uint64_t out = 0x00000000UL;

	for (uint8_t i = 0; i < max_retries; i++) {
		#if SW_CAPABLE_PLATFORM
			if (SWSerial != NULL) {
					SWSerial->listen();
					out = _sendDatagram(*SWSerial, datagram, len, abort_window);
					SWSerial->stopListening();
			} else
		#endif
			{
			  #ifdef TMC_SERIAL_SWITCH
		      sswitch->active();
		    #endif
				out = _sendDatagram(*HWSerial, datagram, len, abort_window);
			}

		delay(replyDelay);

		CRCerror = false;
		uint8_t out_datagram[] = {(uint8_t)(out>>56), (uint8_t)(out>>48), (uint8_t)(out>>40), (uint8_t)(out>>32), (uint8_t)(out>>24), (uint8_t)(out>>16), (uint8_t)(out>>8), (uint8_t)(out>>0)};
		uint8_t crc = calcCRC(out_datagram, 7);
		if ((crc != (uint8_t)out) || crc == 0 ) {
			CRCerror = true;
			out = 0;
		} else {
			break;
		}
	}

	return out>>8;
}

uint8_t TMC2208Stepper::IFCNT() {
	return read(IFCNT_t::address);
}

void TMC2208Stepper::SLAVECONF(uint16_t input) {
	SLAVECONF_register.sr = input&0xF00;
	write(SLAVECONF_register.address, SLAVECONF_register.sr);
}
uint16_t TMC2208Stepper::SLAVECONF() {
	return SLAVECONF_register.sr;
}
void TMC2208Stepper::senddelay(uint8_t B) 	{ SLAVECONF_register.senddelay = B; write(SLAVECONF_register.address, SLAVECONF_register.sr); }
uint8_t TMC2208Stepper::senddelay() 		{ return SLAVECONF_register.senddelay; }

void TMC2208Stepper::OTP_PROG(uint16_t input) {
	write(OTP_PROG_t::address, input);
}

uint32_t TMC2208Stepper::OTP_READ() {
	return read(OTP_READ_t::address);
}

uint32_t TMC2208Stepper::IOIN() {
	return read(TMC2208_n::IOIN_t::address);
}
bool TMC2208Stepper::enn()			{ TMC2208_n::IOIN_t r{0}; r.sr = IOIN(); return r.enn;		}
bool TMC2208Stepper::ms1()			{ TMC2208_n::IOIN_t r{0}; r.sr = IOIN(); return r.ms1;		}
bool TMC2208Stepper::ms2()			{ TMC2208_n::IOIN_t r{0}; r.sr = IOIN(); return r.ms2;		}
bool TMC2208Stepper::diag()			{ TMC2208_n::IOIN_t r{0}; r.sr = IOIN(); return r.diag;		}
bool TMC2208Stepper::pdn_uart()		{ TMC2208_n::IOIN_t r{0}; r.sr = IOIN(); return r.pdn_uart;	}
bool TMC2208Stepper::step()			{ TMC2208_n::IOIN_t r{0}; r.sr = IOIN(); return r.step;		}
bool TMC2208Stepper::sel_a()		{ TMC2208_n::IOIN_t r{0}; r.sr = IOIN(); return r.sel_a;	}
bool TMC2208Stepper::dir()			{ TMC2208_n::IOIN_t r{0}; r.sr = IOIN(); return r.dir;		}
uint8_t TMC2208Stepper::version() 	{ TMC2208_n::IOIN_t r{0}; r.sr = IOIN(); return r.version;	}

uint32_t TMC2224Stepper::IOIN() {
	return read(TMC2224_n::IOIN_t::address);
}
bool TMC2224Stepper::enn()			{ TMC2224_n::IOIN_t r{0}; r.sr = IOIN(); return r.enn;		}
bool TMC2224Stepper::ms1()			{ TMC2224_n::IOIN_t r{0}; r.sr = IOIN(); return r.ms1;		}
bool TMC2224Stepper::ms2()			{ TMC2224_n::IOIN_t r{0}; r.sr = IOIN(); return r.ms2;		}
bool TMC2224Stepper::pdn_uart()		{ TMC2224_n::IOIN_t r{0}; r.sr = IOIN(); return r.pdn_uart;	}
bool TMC2224Stepper::spread()		{ TMC2224_n::IOIN_t r{0}; r.sr = IOIN(); return r.spread;	}
bool TMC2224Stepper::step()			{ TMC2224_n::IOIN_t r{0}; r.sr = IOIN(); return r.step;		}
bool TMC2224Stepper::sel_a()		{ TMC2224_n::IOIN_t r{0}; r.sr = IOIN(); return r.sel_a;	}
bool TMC2224Stepper::dir()			{ TMC2224_n::IOIN_t r{0}; r.sr = IOIN(); return r.dir;		}
uint8_t TMC2224Stepper::version() 	{ TMC2224_n::IOIN_t r{0}; r.sr = IOIN(); return r.version;	}

uint16_t TMC2208Stepper::FACTORY_CONF() {
	if (write_only) return FACTORY_CONF_register.sr;
	return read(FACTORY_CONF_register.address);
}
void TMC2208Stepper::FACTORY_CONF(uint16_t input) {
	FACTORY_CONF_register.sr = input;
	write(FACTORY_CONF_register.address, FACTORY_CONF_register.sr);
}
void TMC2208Stepper::fclktrim(uint8_t B){ FACTORY_CONF_register.fclktrim = B; write(FACTORY_CONF_register.address, FACTORY_CONF_register.sr); }
void TMC2208Stepper::ottrim(uint8_t B)	{ FACTORY_CONF_register.ottrim = B; write(FACTORY_CONF_register.address, FACTORY_CONF_register.sr); }
uint8_t TMC2208Stepper::fclktrim()		{ FACTORY_CONF_t r{0}; r.sr = FACTORY_CONF(); return r.fclktrim; }
uint8_t TMC2208Stepper::ottrim()		{ FACTORY_CONF_t r{0}; r.sr = FACTORY_CONF(); return r.ottrim; }

void TMC2208Stepper::VACTUAL(uint32_t input) {
	VACTUAL_register.sr = input;
	write(VACTUAL_register.address, VACTUAL_register.sr);
}
uint32_t TMC2208Stepper::VACTUAL() {
	return VACTUAL_register.sr;
}

uint32_t TMC2208Stepper::PWM_SCALE() {
	return read(TMC2208_n::PWM_SCALE_t::address);
}
uint8_t TMC2208Stepper::pwm_scale_sum() {
	TMC2208_n::PWM_SCALE_t r{0};
	r.sr = PWM_SCALE();
	return r.pwm_scale_sum;
}

int16_t TMC2208Stepper::pwm_scale_auto() {
	TMC2208_n::PWM_SCALE_t r{0};
	r.sr = PWM_SCALE();
	return r.pwm_scale_auto;
	// Not two's complement? 9nth bit determines sign
	/*
	uint32_t d = PWM_SCALE();
	int16_t response = (d>>PWM_SCALE_AUTO_bp)&0xFF;
	if (((d&PWM_SCALE_AUTO_bm) >> 24) & 0x1) return -response;
	else return response;
	*/
}

// R: PWM_AUTO
uint32_t TMC2208Stepper::PWM_AUTO() {
	return read(PWM_AUTO_t::address);
}
uint8_t TMC2208Stepper::pwm_ofs_auto()  { PWM_AUTO_t r{0}; r.sr = PWM_AUTO(); return r.pwm_ofs_auto; }
uint8_t TMC2208Stepper::pwm_grad_auto() { PWM_AUTO_t r{0}; r.sr = PWM_AUTO(); return r.pwm_grad_auto; }
