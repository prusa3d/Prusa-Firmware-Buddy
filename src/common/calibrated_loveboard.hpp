
#ifndef CALIBRATED_LOVEBOARD
#define CALIBRATED_LOVEBOARD

#include "LoveBoard_EEPROM_Struct.h"
#include "../hw/AT21CSxx.hpp"

#include "crc32.h"

#define MAX_CALIB_POINTS 3
#define MAX_READ_RETRY   20
#define MAX_CRC_RETRY    20

class CalibratedLoveboard {

public:
    CalibratedLoveboard(GPIO_TypeDef *port, uint32_t pin);
    virtual ~CalibratedLoveboard();

#ifdef LOVEBOARD_HAS_PT100
    int32_t apply_calibration(int32_t hx_raw_value);
#endif //LOVEBOARD_HAS_PT100
    bool dataValid();

    bool loadData();
#ifdef _DEBUG
    AT21CSxx *loveboard_eeprom;
#endif

#ifdef LOVEBOARD_HAS_PT100
    he_therm_cal calib_data;
#else
    struct loveboard_eeprom calib_data;
#endif //LOVEBOARD_HAS_PT100

private:
#ifndef _DEBUG
    AT21CSxx *loveboard_eeprom;
#endif
    bool data_valid = false;
#ifdef LOVEBOARD_HAS_PT100
    double poly_coefficient[MAX_CALIB_POINTS] = { 0 };
    int N; //Number of calibration points
    void calculate_calibration();
    void gauss_elimination(double A[MAX_CALIB_POINTS][MAX_CALIB_POINTS + 1], int size, double result[]);

    double powi(double x, int y);
#endif //LOVEBOARD_HAS_PT100
};

#if (BOARD_IS_XBUDDY && defined LOVEBOARD_HAS_EEPROM)
extern CalibratedLoveboard *LoveBoard;

#endif

#endif //CALIBRATED_LOVEBOARD
