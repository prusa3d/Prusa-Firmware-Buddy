// sim_motion.cpp

#include "config.h"

#ifdef SIM_MOTION

    #include "sim_motion.h"
    #include "../Marlin/src/module/stepper.h"
    #include "dbg.h"

static const constexpr uint16_t BUFF_SIZE = 256;

extern "C" {

uint8_t sim_motion_invdir = 0x04;
uint8_t sim_motion_stpdir = 0x00;
uint8_t sim_motion_endstops = 0x00;
int32_t sim_motion_pos[4] = { 10000, 10000, 1600, 0 };
int32_t sim_motion_min[4] = { 0, 0, 0, -2147483648 };
int32_t sim_motion_max[4] = { 18400, 18300, 74400, 2147483647 };
uint8_t sim_motion_buff[BUFF_SIZE];
int sim_motion_bufc = 0;
int sim_motion_bufi = 0;

void sim_motion_cycle(void) {
    static uint8_t cnt = 0;
    static int32_t x0;
    static int32_t y0;
    int32_t x = sim_motion_pos[0];
    int32_t y = sim_motion_pos[1];
    int32_t z = sim_motion_pos[2];
    if (x <= sim_motion_min[0])
        sim_motion_endstops |= 0x01;
    if (x >= sim_motion_max[0])
        sim_motion_endstops |= 0x10;
    if (y <= sim_motion_min[1])
        sim_motion_endstops |= 0x02;
    if (y >= sim_motion_max[1])
        sim_motion_endstops |= 0x20;
    if (z <= sim_motion_min[2])
        sim_motion_endstops |= 0x04;
    if (z >= sim_motion_max[2])
        sim_motion_endstops |= 0x40;
    if (stepper.axis_is_moving(X_AXIS) || stepper.axis_is_moving(Y_AXIS)) {
        if (++cnt > 10) {
            cnt = 0;
            int dx = x - x0;
            int dy = y - y0;
            x0 = x;
            y0 = y;
            int d = sqrtf((dx * dx) + (dy * dy));
            sim_motion_buff[sim_motion_bufi++] = d;
            if (sim_motion_bufi >= BUFF_SIZE)
                sim_motion_bufi = 0;
            sim_motion_bufc++;
            if (sim_motion_bufc > BUFF_SIZE)
                sim_motion_bufc = BUFF_SIZE;
        }
    } else {
        //		x0 = x;
        //		y0 = y;
        //		cnt = 10;
    }
}

int sim_motion_get_diag(uint8_t axis) {
    return (sim_motion_endstops & (0x11 << axis)) ? 1 : 0;
}

int sim_motion_get_min_end(uint8_t axis) {
    return (sim_motion_endstops & (0x01 << axis)) ? 1 : 0;
}

int sim_motion_get_max_end(uint8_t axis) {
    return (sim_motion_endstops & (0x10 << axis)) ? 1 : 0;
}

void sim_motion_set_stp(uint8_t axis, int state) {
    if (state) {
        if ((sim_motion_stpdir ^ (sim_motion_invdir << 4)) & (0x10 << axis)) {
            sim_motion_endstops &= ~(0x01 << axis); //clear min endstop
            sim_motion_pos[axis]++;                 //increment position
            if (sim_motion_pos[axis] > sim_motion_max[axis])
                sim_motion_pos[axis] = sim_motion_max[axis];
        } else {
            sim_motion_endstops &= ~(0x10 << axis); //clear max endstop
            sim_motion_pos[axis]--;                 //decrement position
            if (sim_motion_pos[axis] < sim_motion_min[axis])
                sim_motion_pos[axis] = sim_motion_min[axis];
        }
        sim_motion_stpdir |= (0x01 << axis);
    } else
        sim_motion_stpdir &= ~(0x01 << axis);
}

void sim_motion_set_dir(uint8_t axis, int state) {
    if (state)
        sim_motion_stpdir |= (0x10 << axis);
    else
        sim_motion_stpdir &= ~(0x10 << axis);
}

void sim_motion_set_ena(uint8_t axis, int state) {
}

void sim_motion_print_buff(void) {
    int i;
    if (sim_motion_bufc) {
        i = sim_motion_bufi - sim_motion_bufc;
        if (i < 0)
            i += BUFF_SIZE;
        while (sim_motion_bufc--) {
            _dbg("%3d %u", i, (unsigned int)sim_motion_buff[i]);
            i++;
            if (i >= BUFF_SIZE)
                i = 0;
        }
        sim_motion_bufi = 0;
        sim_motion_bufc = 0;
    }
}
}

#endif //SIM_MOTION
