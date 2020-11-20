/*
 * filament_sensor.hpp
 *
 *  Created on: 2019-12-16
 *      Author: Radek Vana
 */

#pragma once

#include "stdint.h"

enum class fsensor_t : uint8_t {
    NotInitialized, //enable enters this state too
    HasFilament,
    NoFilament,
    NotConnected,
    Disabled
};

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

uint8_t fs_get__send_M600_on__and_disable();
void fs_restore__send_M600_on(uint8_t send_M600_on);
fsensor_t fs_wait_initialized();
void fs_clr_sent();

//not thread safe functions
void fs_init_on_edge();
void fs_init_on_level();
void fs_init_never();
void fs_cycle(); //call it in thread, max call speed 1MHz

//for debug
int fs_was_M600_send();
char fs_get_send_M600_on();
