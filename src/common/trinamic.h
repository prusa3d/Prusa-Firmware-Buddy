// trinamic.h
#pragma once
#include <stdint.h>

#include <inttypes.h>

typedef void(tmc_sg_sample_cb_t)(uint8_t axis, uint16_t sg);

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

typedef struct {
    const char *cmd_name;
    uint8_t reg_adr;
    bool write;
    bool read;
} tmc_reg_t;

extern tmc_reg_t tmc_reg_map[]; //< Null terminated array of known registers

extern void init_tmc_bare_minimum(void);
extern void init_tmc(void);
extern void tmc_get_sgt();
extern void tmc_get_TPWMTHRS();
extern void tmc_get_tstep();
extern uint8_t tmc_sample();
extern uint16_t tmc_get_last_sg_sample(uint8_t axis);

extern uint8_t tmc_get_sg_mask();
extern uint8_t tmc_get_sg_axis();
extern tmc_sg_sample_cb_t *tmc_get_sg_sample_cb();

extern void tmc_set_sg_mask(uint8_t mask);
extern void tmc_set_sg_axis(uint8_t axis);
extern void tmc_set_sg_sample_cb(tmc_sg_sample_cb_t *cb);

extern void tmc_enable_wavetable(bool X, bool Y, bool Z);
extern void tmc_disable_wavetable(bool X, bool Y, bool Z);

/**
 * \brief Check stepper coils for open/short circuit
 *
 * This reports false errors when not moving or moving too fast.
 *
 * \param axis axis to check
 * \return true if all coils are ok, false otherwise
 */
extern bool tmc_check_coils(uint8_t axis);
extern bool tmc_serial_lock_acquire(void);
extern void tmc_serial_lock_release(void);
extern bool tmc_serial_lock_acquire_isr(void);
extern void tmc_serial_lock_release_isr(void);
extern bool tmc_serial_lock_held_by_isr(void);
extern void tmc_serial_lock_mark_isr_starved(void);
extern void tmc_serial_lock_clear_isr_starved(void);
extern bool tmc_serial_lock_requested_by_task(void);
#ifdef __cplusplus
}

#endif //__cplusplus
