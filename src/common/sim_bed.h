// sim_bed.h
#ifndef _SIM_BED_H
#define _SIM_BED_H

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern void sim_bed_init(void);

extern float sim_bed_cycle(float dt);

extern void sim_bed_set_power(float P);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // _SIM_BED_H
