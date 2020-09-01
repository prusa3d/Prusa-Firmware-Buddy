// trinamic.h
#ifndef _TRYNAMIC_H
#define _TRYNAMIC_H

#include <inttypes.h>

typedef void(tmc_sg_sample_cb_t)(uint8_t axis, uint16_t sg);

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern void init_tmc(void);
extern void tmc_set_mres();
extern void tmc_set_move(uint8_t tmc, uint32_t step, uint8_t dir, uint8_t speed);
extern uint8_t tmc_get_diag();
extern void tmc_set_sgthrs(uint16_t SGT);
extern void tmc_get_sgt();
extern void tmc_get_TPWMTHRS();
extern void tmc_get_tstep();
extern uint8_t tmc_sample();

extern uint8_t tmc_sg_mask;
extern uint8_t tmc_sg_axis;
extern tmc_sg_sample_cb_t *tmc_sg_sampe_cb;

#ifdef __cplusplus
}

#endif //__cplusplus
#endif
