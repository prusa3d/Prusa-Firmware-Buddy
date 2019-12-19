// sim_nozzle.h
#ifndef _SIM_NOZZLE_H
#define _SIM_NOZZLE_H

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern void sim_nozzle_init(void);

extern float sim_nozzle_cycle(float dt);

extern void sim_nozzle_set_power(float P);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // _SIM_NOZZLE_H
