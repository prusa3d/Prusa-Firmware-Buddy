/**
 * @file selftest_frame_fans.hpp
 * @author Radek Vana
 * @brief part of screen containing fan selftest
 * @date 2021-12-03
 */

#pragma once

#include "selftest_frame.hpp"
#include "window_icon.hpp"
#include "window_wizard_progress.hpp"
#include "status_footer.hpp"
#include "printers.h"

class SelftestFrameFans : public SelftestFrameNamedWithRadio {
    FooterLine footer;
    window_wizard_progress_t progress;
    window_text_t text_info;

    window_icon_t icon_hotend_fan;
    window_text_t text_hotend_fan;

    window_icon_t icon_print_fan;
    window_text_t text_print_fan;

#if not PRINTER_IS_PRUSA_MINI()
    window_text_t text_fans_switched;
#endif
#if PRINTER_IS_PRUSA_MK3_5()
    window_text_t text_question;
#endif

    struct fan_state_t {
        WindowIcon_OkNg icon_heatbreak_fan_state;
        WindowIcon_OkNg icon_print_fan_state;
#if not PRINTER_IS_PRUSA_MINI()
        WindowIcon_OkNg icon_fans_switched_state;
#endif
    };
    std::array<fan_state_t, HOTENDS> fan_states;

    fan_state_t make_fan_row(size_t index);

    template <size_t... Is>
    std::array<fan_state_t, sizeof...(Is)> make_fan_row_array(std::index_sequence<Is...> index_sequence);

protected:
    virtual void change() override;

public:
    SelftestFrameFans(window_t *parent, PhasesSelftest ph, fsm::PhaseData data);
};
