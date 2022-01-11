//screen_test_graph.hpp
#pragma once
#include "gui.hpp"
#include "window_text.hpp"
#include "window_temp_graph.hpp"
#include "screen.hpp"

struct screen_test_data_t : public AddSuperWindow<screen_t> {
    window_text_t test;
    window_text_t back;
    window_text_button_t tst_eeprom;
    window_text_button_t tst_gui;
    window_text_button_t tst_term;
    window_text_button_t tst_msgbox;
    window_text_button_t tst_wizard_icons;
    window_text_button_t tst_safety_dlg;
#if 0
    window_text_button_t tst_graph;
    window_text_button_t tst_temperature;
    window_text_button_t tst_heat_err;
    window_text_button_t tst_disp_memory;
#endif // 0
    window_text_button_t tst_stack_overflow;
    window_text_button_t tst_stack_div0;
    int8_t id_tim;
    int8_t id_tim1;

public:
    screen_test_data_t();
};
