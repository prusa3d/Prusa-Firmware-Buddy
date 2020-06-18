// Marlin_PID_wrapper.c

#include "../Marlin/src/module/temperature.h"
//Kp is not scaled
float get_Kp_Bed() {
    return Temperature::temp_bed.pid.Kp;
}

//Ki is scaled
float get_Ki_Bed() {
    return unscalePID_i(Temperature::temp_bed.pid.Ki);
}

//Kd is scaled
float get_Kd_Bed() {
    return unscalePID_d(Temperature::temp_bed.pid.Kd);
}

//Kp is not scaled
float get_Kp_Noz() {
    return PID_PARAM(Kp, 0);
}

//Ki is scaled
float get_Ki_Noz() {
    return unscalePID_i(PID_PARAM(Ki, 0));
}

//Kd is scaled
float get_Kd_Noz() {
    return unscalePID_d(PID_PARAM(Kd, 0));
}
