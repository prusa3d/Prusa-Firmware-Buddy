// sim_heater.cpp

#include "config.h"

#ifdef SIM_HEATER

    #include "sim_heater.h"
    #include <inttypes.h>
    #include "adc.hpp"
    #include "sim_nozzle.h"
    #include "sim_bed.h"
    #include "main.h"

static const constexpr uint8_t SIM_HEATER_MULTI = 5;   // time multiply
static const constexpr float SIM_HEATER_DELAY = 0.05F; // cycle delay in ms

extern "C" {

uint8_t sim_heater_flags = 0x00;

    #define _OV(v) v
const short _heater_table[][2] = {
    { _OV(23), 300 },
    { _OV(25), 295 },
    { _OV(27), 290 },
    { _OV(28), 285 },
    { _OV(31), 280 },
    { _OV(33), 275 },
    { _OV(35), 270 },
    { _OV(38), 265 },
    { _OV(41), 260 },
    { _OV(44), 255 },
    { _OV(48), 250 },
    { _OV(52), 245 },
    { _OV(56), 240 },
    { _OV(61), 235 },
    { _OV(66), 230 },
    { _OV(71), 225 },
    { _OV(78), 220 },
    { _OV(84), 215 },
    { _OV(92), 210 },
    { _OV(100), 205 },
    { _OV(109), 200 },
    { _OV(120), 195 },
    { _OV(131), 190 },
    { _OV(143), 185 },
    { _OV(156), 180 },
    { _OV(171), 175 },
    { _OV(187), 170 },
    { _OV(205), 165 },
    { _OV(224), 160 },
    { _OV(245), 155 },
    { _OV(268), 150 },
    { _OV(293), 145 },
    { _OV(320), 140 },
    { _OV(348), 135 },
    { _OV(379), 130 },
    { _OV(411), 125 },
    { _OV(445), 120 },
    { _OV(480), 115 },
    { _OV(516), 110 },
    { _OV(553), 105 },
    { _OV(591), 100 },
    { _OV(628), 95 },
    { _OV(665), 90 },
    { _OV(702), 85 },
    { _OV(737), 80 },
    { _OV(770), 75 },
    { _OV(801), 70 },
    { _OV(830), 65 },
    { _OV(857), 60 },
    { _OV(881), 55 },
    { _OV(903), 50 },
    { _OV(922), 45 },
    { _OV(939), 40 },
    { _OV(954), 35 },
    { _OV(966), 30 },
    { _OV(977), 25 },
    { _OV(985), 20 },
    { _OV(993), 15 },
    { _OV(999), 10 },
    { _OV(1004), 5 },
    { _OV(1008), 0 },
    { _OV(1012), -5 },
    { _OV(1016), -10 },
    { _OV(1020), -15 }
};

int sim_heater_temp2val(float temp) {
    int i;
    int v0;
    int v1;
    int t0;
    int t1;
    int cnt = sizeof(_heater_table) / (2 * sizeof(short));
    for (i = 1; i < cnt; i++) {
        v0 = _heater_table[i - 1][0];
        v1 = _heater_table[i][0];
        t0 = _heater_table[i - 1][1];
        t1 = _heater_table[i][1];
        //if ((t0 <= temp) && (t1 >= temp))
        if ((t1 <= temp) && (t0 >= temp))
            return v0 + (temp - t0) * (v1 - v0) / (t1 - t0);
    }
    return -1;
}

void sim_heater_init(void) {
    sim_nozzle_init();
    sim_bed_init();
    sim_heater_flags = 0x03;
}

void sim_heater_cycle(void) {
    if (sim_heater_flags == 0)
        return;
    float temp;
    int val;

    #ifdef SIM_HEATER_NOZZLE_ADC
    temp = sim_nozzle_cycle(SIM_HEATER_MULTI * SIM_HEATER_DELAY) - 273.15F;
    val = (sim_heater_temp2val(temp));
    AdcSet::nozzle(val);
    #endif //SIM_HEATER_NOZZLE_ADC

    #ifdef SIM_HEATER_BED_ADC
    temp = sim_bed_cycle(SIM_HEATER_MULTI * SIM_HEATER_DELAY) - 273.15F;
    val = (sim_heater_temp2val(temp));
    AdcSet::bed(val);
    #endif //SIM_HEATER_BED_ADC
}

} // extern "C"

#endif //SIM_HEATER
