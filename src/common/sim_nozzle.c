// sim_nozzle.c

#include "sim_nozzle.h"
#include <inttypes.h>
#include <stdlib.h>

typedef struct
{
    float C;   // [J/K] total heat capacity of entire heat block
    float R;   // [K/W] absolute thermal resistance between heat block and ambient
    float Cs;  // [J/K] total heat capacity of sensor
    float Rs;  // [K/W] absolute thermal resistance between sensor and heat block
    float Ta;  // [K] ambient temperature
    float T;   // [K] heat block temperature (avg)
    float Ts;  // [K] sensor temperature (avg)
    float P;   // [W] heater power
    float Ex;  // [J/mm] extrussion energy factor
    float vex; // [mm/s] extrussion speed
} sim_nozzle_t;

sim_nozzle_t sim_nozzle;

void sim_nozzle_init(void) {
    sim_nozzle.C = 10.4F;         // [J/K] total heat capacity of entire heat block
    sim_nozzle.R = 22.0F;         // [K/W] absolute thermal resistance between heat block and ambient
    sim_nozzle.Cs = 0.6F;         // [J/K] total heat capacity of sensor
    sim_nozzle.Rs = 9.0F;         // [K/W] absolute thermal resistance between sensor and heat block
    sim_nozzle.Ta = 298.15F;      // [K] ambient temperature
    sim_nozzle.T = sim_nozzle.Ta; // [K] heat block temperature (avg)
    sim_nozzle.Ts = sim_nozzle.T; // [K] sensor temperature (avg)
    sim_nozzle.P = 0;             // [W] heater power
    sim_nozzle.Ex = 0.2;          // [J/mm] extrussion energy factor
    sim_nozzle.vex = 0;           // [mm/s] extrussion speed
}

float sim_nozzle_cycle(float dt) {
    float E = sim_nozzle.C * sim_nozzle.T;                     //[J] total heat energy stored in heater block
    float Es = sim_nozzle.Cs * sim_nozzle.Ts;                  //[J] total heat energy stored in sensor
    float Pl = (sim_nozzle.T - sim_nozzle.Ta) / sim_nozzle.R;  //[W] power from heater block to ambient (leakage power)
    float Ps = (sim_nozzle.T - sim_nozzle.Ts) / sim_nozzle.Rs; //[W] power from heater to sensor
    float Ed = (sim_nozzle.P - (Pl + Ps)) * dt;                //[J] heater block energy increase
    float Esd = (Ps * dt);                                     //[J] sensor energy increase
    float Ex = (sim_nozzle.Ex * sim_nozzle.vex * dt);
    Ed -= Ex;
    E += Ed;                            //[J] heater block result total heat energy
    Es += Esd;                          //[J] sensor result total heat energy
    sim_nozzle.T = E / sim_nozzle.C;    //[K] result heater temperature
    sim_nozzle.Ts = Es / sim_nozzle.Cs; //[K] result sensor temperature
    return sim_nozzle.Ts;
}

void sim_nozzle_set_power(float P) {
    sim_nozzle.P = P;
}
