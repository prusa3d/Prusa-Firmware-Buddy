// trinamic.h
#pragma once

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

extern uint8_t tmc_get_sg_mask();
extern uint8_t tmc_get_sg_axis();

extern void tmc_set_sg_mask(uint8_t mask);
extern void tmc_set_sg_axis(uint8_t axis);
extern void tmc_set_sg_sampe_cb(tmc_sg_sample_cb_t *cb);

#ifdef __cplusplus
}

#endif //__cplusplus
