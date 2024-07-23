#ifndef SRC_GCODE_LCD_M73_PE_H_
#define SRC_GCODE_LCD_M73_PE_H_

#include <stdint.h>
#include <optional>

#include "config.h"

#if PRINTER_IS_PRUSA_XL()
    // XL needs more time for initial 5 tool preheat/cleanup and MBL
    #define PROGRESS_DATA_VALIDITY_PERIOD (60 * 10) // [s] ~ 10min
#else
    #define PROGRESS_DATA_VALIDITY_PERIOD (60 * 5) // [s] ~ 5min
#endif

class ClValidityValue {
protected:
    uint32_t nValue;
    uint32_t nTime = 0; // [s]
    bool bIsUsed = false;

public:
    void mSetValue(uint32_t nN, uint32_t nNow);
    uint32_t mGetValue(void) const;
    void mInit(void);
    bool mIsActual(uint32_t nNow) const;
    bool mIsActual(uint32_t nNow, uint16_t nPeriod) const;
    bool mIsUsed(void) const;
    // void mFormatSeconds(char *sStr,uint16_t nFeedrate);
};

class ClValidityValueSec : public ClValidityValue {
public:
    void mFormatSeconds(char *sStr, uint16_t nFeedrate);
};

class ClProgressData {
public:
    struct ModeSpecificData {
        ClValidityValue percent_done;
        ClValidityValueSec time_to_end;
        ClValidityValueSec time_to_pause;
    };

    ModeSpecificData standard_mode;
    ModeSpecificData stealth_mode;

    void mInit(void);

    ModeSpecificData &mode_specific(bool stealth) {
        return stealth ? stealth_mode : standard_mode;
    }
};

extern ClProgressData oProgressData;

struct M73_Params {
    struct ModeSpecificParams {
        std::optional<uint8_t> percentage;
        std::optional<uint32_t> time_to_end;
        std::optional<uint32_t> time_to_pause;

        bool operator==(const ModeSpecificParams &) const = default;
    };

    ModeSpecificParams standard_mode;
    ModeSpecificParams stealth_mode;

    bool operator==(const M73_Params &) const = default;
};

void M73_PE_no_parser(const M73_Params &params);

#endif /* SRC_GCODE_LCD_M73_PE_H_ */
