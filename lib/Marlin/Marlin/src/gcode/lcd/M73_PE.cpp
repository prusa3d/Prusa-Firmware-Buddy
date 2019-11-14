#include "../../inc/MarlinConfig.h"

#include "../gcode.h"
#include "../../lcd/ultralcd.h"
#include "../../sd/cardreader.h"

//-//
#include "dbg.h"

#include "M73_PE.h"
#include "../Marlin/src/libs/stopwatch.h"
extern Stopwatch print_job_timer;


ClProgressData oProgressData;


void ClValidityValue::mInit(void)
{
//nTime=0;
bIsUsed=false;
}

void ClValidityValue::mSetValue(uint32_t nN,uint32_t nNow)
{
nValue=nN;
nTime=nNow;
bIsUsed=true;
}

uint32_t ClValidityValue::mGetValue(void)
{
return(nValue);
}

bool ClValidityValue::mIsActual(uint32_t nNow)
{
return(mIsActual(nNow,PROGRESS_DATA_VALIDITY_PERIOD));
}

bool ClValidityValue::mIsActual(uint32_t nNow,uint16_t nPeriod)
{
//return((nTime+nPeriod)>=nNow);
return(mIsUsed()&&((nTime+nPeriod)>=nNow));
}

bool ClValidityValue::mIsUsed(void)
{
//return(nTime>0);
return(bIsUsed);
}


void ClValidityValueSec::mFormatSeconds(char *sStr,uint16_t nFeedrate)
{
uint8_t nDay,nHour,nMin;
uint32_t nRest;

nRest=(nValue*100)/nFeedrate;
nDay=nRest/(60*60*24);
nRest=nRest%(60*60*24);
nHour=nRest/(60*60);
nRest=nRest%(60*60);
nMin=nRest/60;
if(nDay>0)
     sprintf_P(sStr,PSTR("%dd %dh"),nDay,nHour);
else if(nHour>0)
          sprintf_P(sStr,PSTR("%dh %dm"),nHour,nMin);
     else sprintf_P(sStr,PSTR("%dm"),nMin);
}


void ClProgressData::mInit(void)
{
oPercentDirectControl.mInit();
oPercentDone.mInit();
oTime2End.mInit();
oTime2Pause.mInit();
}

#if ENABLED(M73_PRUSA)
void GcodeSuite::M73_PE()
{
uint32_t nTimeNow;
uint8_t nValue;

//ui.set_progress_time(...);
nTimeNow=print_job_timer.duration();              // !!! [s]
if(parser.seen('P'))
     {
     nValue=parser.value_byte();
	 if(parser.seen('R'))
	      {
          oProgressData.oPercentDone.mSetValue((uint32_t)nValue,nTimeNow);
          oProgressData.oTime2End.mSetValue((uint32_t)(parser.value_ulong()*60),nTimeNow); // [min] -> [s]
//_dbg("### M73pe / P :: %d t0: %d\r",oProgressData.oPercentDone.mGetValue(),nTimeNow);
//_dbg("### M73pe / R :: %d t0: %d\r",oProgressData.oTime2End.mGetValue(),nTimeNow);
	      }
	 else {
          oProgressData.oPercentDirectControl.mSetValue((uint32_t)nValue,nTimeNow);
//_dbg("### M73pe / p :: %d t0: %d\r",oProgressData.oPercentDirectControl.mGetValue(),nTimeNow);
          }
     }

if(parser.seen('T'))
     {
     oProgressData.oTime2Pause.mSetValue((uint32_t)(parser.value_ulong()*60),nTimeNow); // [min] -> [s]
//_dbg("### M73pe / T :: %d t0: %d\r",oProgressData.oTime2Pause.mGetValue(),nTimeNow);
     }

// musi byt 'nastaven' "LCD_SET_PROGRESS_MANUALLY"
//    ui.set_progress(parser.value_byte());
//.//_dbg("### M73pe :: P: %d R: %d T: %d",nP,nR,nT);
}
#endif
