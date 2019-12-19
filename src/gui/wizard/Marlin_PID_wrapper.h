// Marlin_PID_wrapper.h
#ifndef _MARLIN_PID_WRAPPER_
#define _MARLIN_PID_WRAPPER_

#ifdef __cplusplus
extern "C" {
#endif

float get_Kp_Bed();
float get_Ki_Bed();
float get_Kd_Bed();
float get_Kp_Noz();
float get_Ki_Noz();
float get_Kd_Noz();

#ifdef __cplusplus
} //extern "C"
#endif

#endif //Marlin_PID_wrapper
