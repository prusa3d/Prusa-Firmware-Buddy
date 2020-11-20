// sim_bed.c

#include "sim_bed.h"
#include <inttypes.h>
#include <stdlib.h>

typedef struct
{
    float C;  // [J/K] total heat capacity of entire heat block
    float R;  // [K/W] absolute thermal resistance between heat block and ambient
    float Cs; // [J/K] total heat capacity of sensor
    float Rs; // [K/W] absolute thermal resistance between sensor and heat block
    float Ta; // [K] ambient temperature
    float T;  // [K] heat block temperature (avg)
    float Ts; // [K] sensor temperature (avg)
    float P;  // [W] heater power
} sim_bed_t;

sim_bed_t sim_bed;

void sim_bed_init(void) {
    sim_bed.C = 150.0F;     // [J/K] total heat capacity of entire heat block
    sim_bed.R = 0.8F;       // [K/W] absolute thermal resistance between heat block and ambient
    sim_bed.Cs = 0.2F;      // [J/K] total heat capacity of sensor
    sim_bed.Rs = 50.0F;     // [K/W] absolute thermal resistance between sensor and heat block
    sim_bed.Ta = 298.15F;   // [K] ambient temperature
    sim_bed.T = sim_bed.Ta; // [K] heat block temperature (avg)
    sim_bed.Ts = sim_bed.T; // [K] sensor temperature (avg)
    sim_bed.P = 0;          // [W] heater power
}

float sim_bed_cycle(float dt) {
    float E = sim_bed.C * sim_bed.T;                  //[J] total heat energy stored in heater block
    float Es = sim_bed.Cs * sim_bed.Ts;               //[J] total heat energy stored in sensor
    float Pl = (sim_bed.T - sim_bed.Ta) / sim_bed.R;  //[W] power from heater block to ambient (leakage power)
    float Ps = (sim_bed.T - sim_bed.Ts) / sim_bed.Rs; //[W] power from heater to sensor
    float Ed = (sim_bed.P - (Pl + Ps)) * dt;          //[J] heater block energy increase
    float Esd = (Ps * dt);                            //[J] sensor energy increase
    E += Ed;                                          //[J] heater block result total heat energy
    Es += Esd;                                        //[J] sensor result total heat energy
    sim_bed.T = E / sim_bed.C;                        //[K] result heater temperature
    sim_bed.Ts = Es / sim_bed.Cs;                     //[K] result sensor temperature
    return sim_bed.Ts;
}

void sim_bed_set_power(float P) {
    sim_bed.P = P;
}
