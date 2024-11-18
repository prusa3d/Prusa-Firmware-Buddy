/**
 * @file empty_mocks.cpp
 * @author Radek Vana
 * @brief dummy definitions of unused functions, so tests can be compiled
 * @date 2021-01-13
 */

#include "guitypes.hpp"
#include "sound_enum.h"
#include "ScreenHandler.hpp"
#include "cmsis_os.h" //HAL_GetTick
#include "mock_windows.hpp"
#include "img_resources.hpp"
#include <memory>

void gui_invalidate(void) {}
void Sound_Play(eSOUND_TYPE eSoundType) {}
void gui_loop() {}

namespace marlin_client {
void notify_server_about_encoder_move() {}
void notify_server_about_knob_click() {}
} // namespace marlin_client

FILE *img::get_resource_file() { return nullptr; }

void _bsod(const char *fmt, const char *fine_name, int line_number, ...) {
    while (1)
        ;
}

GUI_event_t last_gui_input_event = GUI_event_t::_count;
