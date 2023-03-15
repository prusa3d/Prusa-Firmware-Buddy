/**
 * @file
*/
#include "config_buddy_2209_02.h"
#include "wdt.h"
#if (BOARD_IS_XBUDDY && defined LOVEBOARD_HAS_EEPROM)

    #include "calibrated_loveboard.hpp"
    #include <cmath>
    #include <cstring>
    #include "FreeRTOS.h"
    #include "task.h"
CalibratedLoveboard::CalibratedLoveboard(GPIO_TypeDef *port, uint32_t pin) {

    loveboard_eeprom = new AT21CSxx(port, pin);
    loadData();
    #ifndef _DEBUG
    delete loveboard_eeprom;
    #endif
}

bool CalibratedLoveboard::loadData() {
    //Read data from EEPROM
    wdt_iwdg_refresh();
    data_valid = false;
    memset(&calib_data, 0, sizeof(calib_data));

    for (int i = 0; i < MAX_CRC_RETRY; i++) {
        for (int j = 0; j < MAX_READ_RETRY; j++) {
            if (loveboard_eeprom->read_block(0, (uint8_t *)&calib_data, sizeof(calib_data)) == 1) {
                data_valid = 1;
                break;
            }
            wdt_iwdg_refresh();
        }

    #ifdef LOVEBOARD_HAS_PT100
        //Check CRC
        uint32_t eeprom_crc = calib_data.crc;
        calib_data.crc = 0;
        uint32_t computed_crc = crc32_calc((uint8_t *)&calib_data, sizeof(he_therm_cal));
        calib_data.crc = eeprom_crc;

        if ((eeprom_crc != computed_crc) && (calib_data.struct_ver > 0)) { //From version 1 onward, CRC is required
    #else                                                                  //LOVEBOARD_HAS_PT100
        if (calib_data.struct_size != sizeof(calib_data) || calib_data.datamatrix_id[0] != '0' || calib_data.datamatrix_id[1] != '0') { // heuristics: if read failed somehow, these values are probably not OK
    #endif                                                                 //LOVEBOARD_HAS_PT100
            data_valid = 0;
            wdt_iwdg_refresh();
        } else {
            break;
        }
        wdt_iwdg_refresh();
        loveboard_eeprom->reset_discovery();
    }
    #ifdef LOVEBOARD_HAS_PT100
    calculate_calibration();
    #endif //LOVEBOARD_HAS_PT100
    return data_valid;
}

    #ifdef LOVEBOARD_HAS_PT100
/**
 * @brief Calculate calibration constants and save them to poly_coefficient.
 * Uses N-order polynomial to get resistance from HX717 output.
 * This function calculates constants for this polynomial.
 */
void CalibratedLoveboard::calculate_calibration() {
    //Fill matrix for gauss elimination
    N = CALIB_COUNT;

    double calib_matrix[MAX_CALIB_POINTS][MAX_CALIB_POINTS + 1] = { { 0 } };
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            calib_matrix[i][j] = pow(calib_data.calib_data[i].hx_data, j);
        }
        calib_matrix[i][N] = (double)calib_data.calib_data[i].resistance;
    }

    gauss_elimination(calib_matrix, N, poly_coefficient);

    //Check if calibration result makes sense
    if (poly_coefficient[0] > 1000 || poly_coefficient[0] < -1000) {
        data_valid = false;
    }
}
    #endif //LOVEBOARD_HAS_PT100

bool CalibratedLoveboard::dataValid() {
    return data_valid;
}

CalibratedLoveboard::~CalibratedLoveboard() {
    #ifdef _DEBUG
    delete loveboard_eeprom;
    #endif
}

    #ifdef LOVEBOARD_HAS_PT100

/**
 * @brief Apply calibration onto raw HX717 data
 * @param hx_raw_value Raw data from HX717
 * @return Measured resistance in milliohms.
 */
int32_t CalibratedLoveboard::apply_calibration(int32_t hx_raw_value) {
    if (!data_valid) {
        return 100 * 1000; //If data not valid, return 100Ohms to allow running without loveboard connected.
    }

    hx_raw_value >>= 8; // HX717 only has 15.8 bits of noise free data(out of 24 bits). Discard the noisy lower 8 bits.
    double res = poly_coefficient[0] + poly_coefficient[1] * hx_raw_value + poly_coefficient[2] * hx_raw_value * hx_raw_value;
    if (res < 0) { //Resistance can't possibly be les than 0 Ohms.
        return -1;
    }
    return (int32_t)(res * 1000);
}

//Calculate x^y where y is nonzero positive integer.
double CalibratedLoveboard::powi(double x, int y) {
    double res = x;
    for (int i = 0; i < y - 1; i++) {
        res = res * x;
    }
    return res;
}

//Calculate gauss elimination of matrix A of size up to[N, N+1] and store resoult in resoult[](buffer of size N)
void CalibratedLoveboard::gauss_elimination(double A[MAX_CALIB_POINTS][MAX_CALIB_POINTS + 1], int size, double result[]) {

    int i, j, k, n = size;
    double b;

    for (j = 0; j < n; j++) {
        for (i = 0; i < n; i++) {
            if (i != j) {
                b = A[i][j] / A[j][j];
                for (k = 0; k < n + 1; k++) {
                    A[i][k] = A[i][k] - b * A[j][k];
                }
            }
        }
    }

    for (i = 0; i < n; i++) {
        result[i] = A[i][n] / A[i][i];
    }

    return;
}

    #endif //LOVEBOARD_HAS_PT100

#endif //(BOARD_IS_XBUDDY && defined LOVEBOARD_HAS_EEPROM)
