// sim_motion.h

#ifndef _SIM_MOTION_H
#define _SIM_MOTION_H

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern int32_t sim_motion_pos[4];

extern void sim_motion_cycle(void);

extern int sim_motion_get_diag(uint8_t axis);

extern int sim_motion_get_min_end(uint8_t axis);

extern int sim_motion_get_max_end(uint8_t axis);

extern void sim_motion_set_stp(uint8_t axis, int state);

extern void sim_motion_set_dir(uint8_t axis, int state);

extern void sim_motion_set_ena(uint8_t axis, int state);

extern void sim_motion_print_buff(void);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_SIM_MOTION_H
