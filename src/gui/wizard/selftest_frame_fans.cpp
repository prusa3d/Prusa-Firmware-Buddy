/**
 * @file selftest_frame_fans.cpp
 * @author Radek Vana
 * @date 2021-12-03
 */

#include "selftest_frame_fans.hpp"
#include "i18n.h"
#include <guiconfig/wizard_config.hpp>
#include "selftest_fans_type.hpp"
#include "img_resources.hpp"
#include <guiconfig/guiconfig.h>
#if HAS_TOOLCHANGER()
    #include "module/prusa/toolchanger.h"
#endif
#include <option/has_switched_fan_test.h>

static constexpr size_t col_texts = WizardDefaults::col_after_icon;
static constexpr size_t col_results = WizardDefaults::status_icon_X_pos;
static constexpr size_t col_texts_w = col_results - col_texts;

static constexpr size_t row_2 = WizardDefaults::row_1 + WizardDefaults::progress_row_h;
static constexpr size_t row_3 = row_2 + WizardDefaults::row_h;
static constexpr size_t row_4 = row_3 + WizardDefaults::row_h;
static constexpr size_t row_5 = row_4 + WizardDefaults::row_h + 20;

static constexpr const char *en_text_fan_test = N_("Fan Test");
#if HAS_MINI_DISPLAY()
static constexpr const char *en_text_hotend_fan = N_("Hotend fan");
static constexpr const char *en_text_print_fan = N_("Print fan");
static constexpr const char *en_text_fans_switched = N_("Switched fans");
#else
static constexpr const char *en_text_hotend_fan = N_("Hotend fan RPM test");
static constexpr const char *en_text_print_fan = N_("Print fan RPM test");
static constexpr const char *en_text_fans_switched = N_("Checking for switched fans");
#endif

#if PRINTER_IS_PRUSA_MK3_5()
static constexpr const char *en_text_manual_check_hotend = N_("Is Hotend fan (left) spinning?");
#endif

static constexpr const char *en_text_info = N_("Fan test in progress, please wait.");
static constexpr const char *en_text_info_rpm_failed = N_("The RPM test has failed, check both fans are free to spin and connected correctly.");
#if HAS_SWITCHED_FAN_TEST()
static constexpr const char *en_text_info_switched = N_("Based on the test it looks like the fans connectors are switched. Double check your wiring and repeat the test.");
#endif /* HAS_SWITCHED_FAN_TEST() */

SelftestFrameFans::fan_state_t SelftestFrameFans::make_fan_row(size_t index) {
    constexpr static int16_t ICON_SPACING = 20;

    [[maybe_unused]] unsigned int disabled = 0;
#if HAS_TOOLCHANGER()
    // when toolchanger is enabled, count how many tools after this one are disabled, and shift icons to the right over the hidden ones

    for (size_t i = index; i < HOTENDS; i++) {
        if (!prusa_toolchanger.is_tool_enabled(i)) {
            ++disabled;
        }
    }
#endif

    // Align it to the right (to the col_results), omit disabled tools
    const int16_t y = col_results - HOTENDS * ICON_SPACING + (index + 1 + disabled) * ICON_SPACING;

    return {
        .icon_heatbreak_fan_state = WindowIcon_OkNg(this, { y, row_2 }),
        .icon_print_fan_state = WindowIcon_OkNg(this, { y, row_3 }),

#if HAS_SWITCHED_FAN_TEST()
        .icon_fans_switched_state = WindowIcon_OkNg(this, { y, row_4 }),
#endif /* HAS_SWITCHED_FAN_TEST */
    };
}

template <size_t... Is>
std::array<SelftestFrameFans::fan_state_t, sizeof...(Is)> SelftestFrameFans::make_fan_row_array(std::index_sequence<Is...>) {
    //  this is just fancy template way to init array in constructor initializer_list
    return { (make_fan_row(Is))... };
}

SelftestFrameFans::SelftestFrameFans(window_t *parent, PhasesSelftest ph, fsm::PhaseData data)
    : SelftestFrameNamedWithRadio(parent, ph, data, _(en_text_fan_test), 1)
#if HAS_TOOLCHANGER()
    // when toolchanger is enabled, do not show footer with fan RPM, because its likely that no tool will be picked and it would just show zero RPM
    , footer(this, 0)
#else
    , footer(this, 0, footer::Item::print_fan, footer::Item::heatbreak_fan)

#endif
    , progress(this, WizardDefaults::row_1)
    , text_info(this, Rect16(col_texts, row_5, col_texts_w, GetRect().Height() - GetRect().Top() - row_5 - 20), is_multiline::yes, is_closed_on_click_t::no, _(en_text_info))
    , icon_hotend_fan(this, &img::fan_16x16, point_i16_t({ WizardDefaults::col_0, row_2 }))
    , text_hotend_fan(this, Rect16(col_texts, row_2, col_texts_w, WizardDefaults::txt_h), is_multiline::no, is_closed_on_click_t::no, _(en_text_hotend_fan))
    , icon_print_fan(this, &img::turbine_16x16, point_i16_t({ WizardDefaults::col_0, row_3 }))
    , text_print_fan(this, Rect16(col_texts, row_3, col_texts_w, WizardDefaults::txt_h), is_multiline::no, is_closed_on_click_t::no, _(en_text_print_fan))
#if HAS_SWITCHED_FAN_TEST()
    , text_fans_switched(this, Rect16(col_texts, row_4, col_texts_w, WizardDefaults::txt_h), is_multiline::no, is_closed_on_click_t::no, _(en_text_fans_switched))
#endif /* HAS_SWITCHED_FAN_TEST*/
#if PRINTER_IS_PRUSA_MK3_5()
    // The question should cover whole text_info - so we should take the values from it
    , text_question(this, text_info.GetRect(), is_multiline::yes, is_closed_on_click_t::no, _(en_text_manual_check_hotend))
#endif
    , fan_states(make_fan_row_array(std::make_index_sequence<HOTENDS>())) {
#if PRINTER_IS_PRUSA_MK3_5()
    text_question.Hide();
#endif
#if HAS_TOOLCHANGER()
    // when toolchanger enabled, hide results of tools that are not connected
    for (size_t i = 0; i < HOTENDS; i++) {

        if (!prusa_toolchanger.is_tool_enabled(i)) {
            fan_states[i].icon_heatbreak_fan_state.Hide();
            fan_states[i].icon_print_fan_state.Hide();
    #if HAS_SWITCHED_FAN_TEST()
            fan_states[i].icon_fans_switched_state.Hide();
    #endif /* HAS_SWITCHED_FAN_TEST() */
        }
    }
#endif

#if HAS_MINI_DISPLAY()
    text_info.set_font(GuiDefaults::FontMenuSpecial);
#endif

    change();
}

void SelftestFrameFans::change() {
    SelftestFansResult result;

#if PRINTER_IS_PRUSA_MK3_5()
    switch (phase_current) {
    case PhasesSelftest::Fans_manual:
        text_question.Show();
        break;
    case PhasesSelftest::Fans:
    case PhasesSelftest::Fans_second:
        text_question.Hide();
        break;
    default:
        break;
    }
#endif

    if (FSMExtendedDataManager::get(result)) {
#if HAS_SWITCHED_FAN_TEST()
        bool fan_switch_detected_on_at_least_one_hotend { false };
#endif /* HAS_SWITCHED_FAN_TEST() */
        bool rpm_failed_on_at_least_one_hotend { false };
        bool rpm_test_still_in_progress { false };
        for (size_t i = 0; i < fan_states.size(); i++) {
            fan_states[i].icon_print_fan_state.SetState(result.hotend_results[i].print_fan_state);
            fan_states[i].icon_heatbreak_fan_state.SetState(result.hotend_results[i].heatbreak_fan_state);

#if HAS_SWITCHED_FAN_TEST()
            fan_states[i].icon_fans_switched_state.SetState(result.hotend_results[i].fans_switched_state);
            if (result.hotend_results[i].fans_switched_state == SelftestSubtestState_t::not_good) {
                fan_switch_detected_on_at_least_one_hotend = true;
            }
#endif /* HAS_SWITCHED_FAN_TEST */

            if (result.hotend_results[i].print_fan_state == SelftestSubtestState_t::running || result.hotend_results[i].heatbreak_fan_state == SelftestSubtestState_t::running) {
                rpm_test_still_in_progress = true;
            }

            if (result.hotend_results[i].print_fan_state == SelftestSubtestState_t::not_good || result.hotend_results[i].heatbreak_fan_state == SelftestSubtestState_t::not_good) {
                rpm_failed_on_at_least_one_hotend = true;
            }
        }

        if (rpm_failed_on_at_least_one_hotend && !rpm_test_still_in_progress) {
            text_info.SetText(_(en_text_info_rpm_failed));
        }
#if HAS_SWITCHED_FAN_TEST()
        else if (fan_switch_detected_on_at_least_one_hotend) {
            text_info.SetText(_(en_text_info_switched));
        }
#endif /* HAS_SWITCHED_FAN_TEST() */

        progress.SetProgressPercent(result.progress);
    }
};
