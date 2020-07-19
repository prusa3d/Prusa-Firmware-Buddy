//window_header.hpp
#if 0
    #pragma once
    #include "gui.hpp"
    #include "window_text.hpp"
    #include "support_utils.h" //MAX_LEN_4QR
    #include <array>

    #define SPIN_DIGITS     6
    #define SPIN_PRECISION  2
    #define SPIN_INT_DIGITS (SPIN_DIGITS - SPIN_PRECISION)

struct screen_PID_data_t : public window_frame_t {
    status_footer_t footer;

    window_text_t textMenuName;

    window_text_t btAutoTuneApply_E;
    window_spin_t spinAutoTn_E;
    window_list_t list_RW_E; //choose read write PID
    window_spin_t spinKp_E[SPIN_DIGITS];
    window_spin_t spinKi_E[SPIN_DIGITS];
    window_spin_t spinKd_E[SPIN_DIGITS];

    window_text_t btAutoTuneApply_B;
    window_spin_t spinAutoTn_B;
    window_list_t list_RW_B; //choose read write PID
    window_spin_t spinKp_B[SPIN_DIGITS];
    window_spin_t spinKi_B[SPIN_DIGITS];
    window_spin_t spinKd_B[SPIN_DIGITS];

    window_text_t textExit;


    size_t list_RW_E_index_actual;
    size_t list_RW_E_index_last;
    size_t list_RW_B_index_actual;
    size_t list_RW_B_index_last;

    uint16_t dot_coordsKp_E[2];
    uint16_t dot_coordsKi_E[2];
    uint16_t dot_coordsKd_E[2];

    uint16_t dot_coordsKp_B[2];
    uint16_t dot_coordsKi_B[2];
    uint16_t dot_coordsKd_B[2];

    rect_ui16_t rect_E;
    rect_ui16_t rectKp_E;
    rect_ui16_t rectKi_E;
    rect_ui16_t rectKd_E;

    rect_ui16_t rect_B;
    rect_ui16_t rectKp_B;
    rect_ui16_t rectKi_B;
    rect_ui16_t rectKd_B;

    float autotune_temp_B;
    float autotune_temp_E;

    _PID_t _PID_E;
    _PID_t _PID_B;

    autotune_state_t autotune_state;

    bool redraw;
public:
    screen_PID_data_t();

private:
    virtual int event(window_t *sender, uint8_t event, void *param) override;
};
#endif //#if 0
