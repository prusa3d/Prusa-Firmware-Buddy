/*
 * filament_sensor.h
 *
 *  Created on: 2019-12-16
 *      Author: Radek Vana
 */

#ifndef _FILAMENT_SENSOR_H
#define _FILAMENT_SENSOR_H

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

#pragma pack(push)
#pragma pack(1) //makes enum 1 BYTE
typedef enum {
    FS_NOT_INICIALIZED, //enable enters this state too
    FS_HAS_FILAMENT,
    FS_NO_FILAMENT,
    FS_NOT_CONNECTED,
    FS_DISABLED
} fsensor_t;
#pragma pack(pop)

//thread safe functions
fsensor_t fs_get_state();
int fs_did_filament_runout(); //for arduino / marlin

//switch behavior when M600 should be send
void fs_send_M600_on_edge(); //default behavior
void fs_send_M600_on_level();
void fs_send_M600_never();

//thread safe functions, but cannot be called from interrupt
void fs_enable();
void fs_disable();
fsensor_t fs_wait_inicialized();

//not thread safe functions
void fs_init_on_edge();
void fs_init_on_level();
void fs_init_never();
void fs_cycle(); //call it in thread, max call speed 1MHz

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_FILAMENT_SENSOR_H
