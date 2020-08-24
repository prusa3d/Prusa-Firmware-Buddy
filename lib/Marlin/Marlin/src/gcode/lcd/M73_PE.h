#ifndef SRC_GCODE_LCD_M73_PE_H_
#define SRC_GCODE_LCD_M73_PE_H_

#include <stdint.h>
#define PROGRESS_DATA_VALIDITY_PERIOD (60*5)      // [s] ~ 5min
//#define PROGRESS_DATA_VALIDITY_PERIOD 20


class ClValidityValue
{
protected:
uint32_t nValue;
uint32_t nTime=0;                                 // [s]
bool bIsUsed=false;

public:
void mSetValue(uint32_t nN,uint32_t nNow);
uint32_t mGetValue(void);
void mInit(void);
bool mIsActual(uint32_t nNow);
bool mIsActual(uint32_t nNow,uint16_t nPeriod);
bool mIsUsed(void);
//void mFormatSeconds(char *sStr,uint16_t nFeedrate);
};


class ClValidityValueSec : public ClValidityValue
{
public:
void mFormatSeconds(char *sStr,uint16_t nFeedrate);
};


class ClProgressData
{
public:
ClValidityValue oPercentDirectControl;
ClValidityValue oPercentDone;
ClValidityValueSec oTime2End;
ClValidityValueSec oTime2Pause;

public:
void mInit(void);
};


extern ClProgressData oProgressData;


#endif /* SRC_GCODE_LCD_M73_PE_H_ */
