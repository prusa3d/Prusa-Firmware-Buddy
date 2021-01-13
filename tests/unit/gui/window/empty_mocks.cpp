/**
 * @file empty_mocks.cpp
 * @author Radek Vana
 * @brief dummy definitions of unused functions, so tests can be compilled
 * @date 2021-01-13
 */

#include "sound_enum.h"
#include "ScreenHandler.hpp"
#include "cmsis_os.h" //HAL_GetTick
#include "mock_windows.hpp"
#include <memory>

void gui_timers_delete_by_window(window_t *pWin) {}
void gui_invalidate(void) {}
EventLock::EventLock(const char *event_method_name, window_t *sender, GUI_event_t event) {}
void Sound_Play(eSOUND_TYPE eSoundType) {}
void gui_loop() {}
extern "C" void marlin_notify_server_about_encoder_move() {}
extern "C" void marlin_notify_server_about_knob_click() {}
