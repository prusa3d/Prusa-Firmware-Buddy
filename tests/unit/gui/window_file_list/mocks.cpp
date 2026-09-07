#include <WindowMenuItems.hpp>
#include <conserve_cpu.hpp>
#include <sound.hpp>
#include <window_event.hpp>

void gui_invalidate() {}

void sound::play(SoundType) {}

GUI_event_t last_gui_input_event = GUI_event_t::_count;

buddy::ConserveCpu &buddy::conserve_cpu() {
    static ConserveCpu instance;
    return instance;
}

namespace marlin_client {
void notify_server_about_encoder_move_up() {}
void notify_server_about_encoder_move_down() {}
} // namespace marlin_client

// Avoids pulling in WindowMenuItems.cpp with all its dependencies
MI_RETURN::MI_RETURN()
    : IWindowMenuItem(string_view_utf8::MakeCPUFLASH(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
    has_return_behavior_ = true;
}

void MI_RETURN::click(IWindowMenu &) {}
