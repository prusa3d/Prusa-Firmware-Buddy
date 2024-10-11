#include "../../inc/MarlinConfig.h"

#include "../gcode.h"
#include "../../lcd/ultralcd.h"
#include "../../sd/cardreader.h"

#include "M73_PE.h"
#include "../Marlin/src/libs/stopwatch.h"
#include "marlin_vars.hpp"
extern Stopwatch print_job_timer;

ClProgressData oProgressData;

void ClValidityValue::mInit(void) {
    // nTime=0;
    bIsUsed = false;
}

void ClValidityValue::mSetValue(uint32_t nN, uint32_t nNow) {
    nValue = nN;
    nTime = nNow;
    bIsUsed = true;
}

uint32_t ClValidityValue::mGetValue(void) const {
    return (nValue);
}

bool ClValidityValue::mIsActual(uint32_t nNow) const {
    return (mIsActual(nNow, PROGRESS_DATA_VALIDITY_PERIOD));
}

bool ClValidityValue::mIsActual(uint32_t nNow, uint16_t nPeriod) const {
    // return((nTime+nPeriod)>=nNow);
    return (mIsUsed() && ((nTime + nPeriod) >= nNow));
}

bool ClValidityValue::mIsUsed(void) const {
    // return(nTime>0);
    return (bIsUsed);
}

void ClValidityValueSec::mFormatSeconds(char *sStr, uint16_t nFeedrate) {
    uint8_t nDay, nHour, nMin;
    uint32_t nRest;

    nRest = (nValue * 100) / nFeedrate;
    nDay = nRest / (60 * 60 * 24);
    nRest = nRest % (60 * 60 * 24);
    nHour = nRest / (60 * 60);
    nRest = nRest % (60 * 60);
    nMin = nRest / 60;
    if (nDay > 0) {
        sprintf_P(sStr, PSTR("%dd %dh"), nDay, nHour);
    } else if (nHour > 0) {
        sprintf_P(sStr, PSTR("%dh %dm"), nHour, nMin);
    } else {
        sprintf_P(sStr, PSTR("%dm"), nMin);
    }
}

void ClProgressData::mInit(void) {
    const auto mode_specific = [](ModeSpecificData &d) {
        d.percent_done.mInit();
        d.time_to_end.mInit();
        d.time_to_pause.mInit();
    };
    mode_specific(standard_mode);
    mode_specific(stealth_mode);
}

#if ENABLED(M73_PRUSA)

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M73: Set/Get build percentage <a href="https://reprap.org/wiki/G-code#M73:_Set.2FGet_build_percentage">M73: Set/Get build percentage</a>
 *
 * The machine is expected to display this on its display.
 *
 *#### Usage
 *
 *    M73 [ P | Q | R | S | C | D ]
 *
 *#### Parameters
 *
 *  - `P` - Set percentage value
 *  - `Q` - Set percentage value in stealth mode
 *  - `R` - Set time to end / percentage done [minutes]
 *  - `S` - Set time to end / percentage done in stealth mode [minutes]
 *  - `C` - Set time to pause [minutes]
 *  - `D` - Set time to pause in stealth mode [minutes]
 *  - `T` - Set time to pause [minutes] backwards compatibility
 */

void GcodeSuite::M73_PE() {
    M73_Params p;
    if (parser.seen('P')) {
        p.standard_mode.percentage = parser.value_byte();
    }
    if (parser.seen('R')) {
        p.standard_mode.time_to_end = parser.value_ulong() * 60;
    }
    if (parser.seen('C')) {
        p.standard_mode.time_to_pause = parser.value_ulong() * 60;
    }
    if (parser.seen('T')) {
        p.standard_mode.time_to_pause = parser.value_ulong() * 60;
    }

    if (parser.seen('Q')) {
        p.stealth_mode.percentage = parser.value_byte();
    }
    if (parser.seen('S')) {
        p.stealth_mode.time_to_end = parser.value_ulong() * 60;
    }
    if (parser.seen('D')) {
        p.stealth_mode.time_to_pause = parser.value_ulong() * 60;
    }

    M73_PE_no_parser(p);
}

/** @}*/

void M73_PE_no_parser(const M73_Params &params) {
    /// [s]
    const uint32_t nTimeNow = print_job_timer.duration();

    const auto mode_specific_update = [&](const M73_Params::ModeSpecificParams &mparams, ClProgressData::ModeSpecificData &mdata) {
        if (auto v = mparams.percentage) {
            mdata.percent_done.mSetValue((uint32_t)*v, nTimeNow);
        }
        if (auto v = mparams.time_to_end) {
            mdata.time_to_end.mSetValue(*v, nTimeNow); // [min] -> [s]
        }
        if (auto v = mparams.time_to_pause) {
            if (*v == 0) {
                // Pause happening now, reset the countdown (there doesn't have to be another until the end).
                mdata.time_to_pause.mInit();
            } else {
                mdata.time_to_pause.mSetValue(*v, nTimeNow); // [min] -> [s]
            }
        }
    };

    mode_specific_update(params.standard_mode, oProgressData.standard_mode);
    mode_specific_update(params.stealth_mode, oProgressData.stealth_mode);

    // Print progress report. Do not remove as third party tools might depend on this
    if (params == M73_Params {}) {
        SERIAL_ECHO_START();
        SERIAL_ECHOLNPAIR(" M73 Progress: ", marlin_vars().sd_percent_done, "%;");
        const uint32_t time_to_end = marlin_vars().time_to_end;
        if (time_to_end != marlin_server::TIME_TO_END_INVALID) {
            SERIAL_ECHOPAIR(" Time left: ", time_to_end / 60, "m;");
            SERIAL_EOL();
        }

        const uint32_t time_to_pause = oProgressData.mode_specific(marlin_vars().stealth_mode).time_to_pause.mGetValue();
        if (time_to_pause != marlin_server::TIME_TO_END_INVALID) {
            const int print_speed = marlin_vars().print_speed;
            SERIAL_ECHOPAIR(" Change: ", print_speed > 0 ? ((time_to_pause * 100) / print_speed) / 60 : 0, "m;");
            SERIAL_EOL();
        }
    }
}

#endif
