/**
 * @file selftest_frame_fans.cpp
 * @author Radek Vana
 * @date 2021-12-03
 */

#include "selftest_frame_fans.hpp"
#include "i18n.h"
#include "wizard_config.hpp"
#include "selftest_fans_type.hpp"
#include "png_resources.hpp"
#if HAS_TOOLCHANGER()
    #include "module/prusa/toolchanger.h"
#endif

static constexpr size_t col_texts = WizardDefaults::col_after_icon;
static constexpr size_t col_results = WizardDefaults::status_icon_X_pos;
static constexpr size_t col_texts_w = col_results - col_texts;

static constexpr size_t row_2 = WizardDefaults::row_1 + WizardDefaults::progress_row_h;
static constexpr size_t row_3 = row_2 + WizardDefaults::row_h;
static constexpr size_t row_4 = row_3 + WizardDefaults::row_h + 20;

static constexpr Rect16 rc = Rect16(WizardDefaults::col_0, row_4, WizardDefaults::X_space, WizardDefaults::RectSelftestFrame.Height() - WizardDefaults::RectSelftestFrame.Top() - row_4);

static constexpr const char *en_text_fan_test = N_("Fan Test");
static constexpr const char *en_text_hotend_fan = N_("Hotend Fan");
static constexpr const char *en_text_print_fan = N_("Print Fan");
static constexpr const char *en_text_info = N_("During the test, the expected fan speeds are checked for 30 seconds."); // TODO calculate time, fine test takes longer

SelftestFrameFans::fan_state_t SelftestFrameFans::make_fan_row(size_t index) {
    constexpr static int16_t ICON_SPACING = 20;

    [[maybe_unused]] unsigned int disabled = 0;
#if HAS_TOOLCHANGER()
    // when toolchanger is enabled, count how many tools after this one are disabled, and shift icons to the right over the hidden ones

    for (size_t i = index; i < HOTENDS; i++) {
        if (!prusa_toolchanger.is_tool_enabled(i))
            ++disabled;
    }
#endif

    // Align it to the right (to the col_results), omit disabled tools
    const int16_t y = col_results - HOTENDS * ICON_SPACING + (index + 1 + disabled) * ICON_SPACING;

    return {
        .icon_print_fan_state = WindowIcon_OkNg(this, { y, row_3 }),
        .icon_heatbreak_fan_state = WindowIcon_OkNg(this, { y, row_2 }),
    };
}

template <size_t... Is>
std::array<SelftestFrameFans::fan_state_t, sizeof...(Is)> SelftestFrameFans::make_fan_row_array(std::index_sequence<Is...>) {
    //  this is just fancy template way to init array in constructor initializer_list
    return { (make_fan_row(Is))... };
}

SelftestFrameFans::SelftestFrameFans(window_t *parent, PhasesSelftest ph, fsm::PhaseData data)
    : AddSuperWindow<SelftestFrameNamed>(parent, ph, data, _(en_text_fan_test))
#if HAS_TOOLCHANGER()
    // when toolchanger is enabled, do not show footer with fan RPM, because its likely that no tool will be picked and it would just show zero RPM
    , footer(this, 0)
#else
    , footer(this, 0, footer::Item::PrintFan, footer::Item::HeatbreakFan)

#endif
    , progress(this, WizardDefaults::row_1)
    , text_info(this, Rect16(col_texts, row_4, WizardDefaults::X_space, GetRect().Height() - GetRect().Top() - row_4), is_multiline::yes, is_closed_on_click_t::no, _(en_text_info))
    , icon_hotend_fan(this, &png::fan_16x16, point_i16_t({ WizardDefaults::col_0, row_2 }))
    , text_hotend_fan(this, Rect16(col_texts, row_2, col_texts_w, WizardDefaults::txt_h), is_multiline::no, is_closed_on_click_t::no, _(en_text_hotend_fan))
    , icon_print_fan(this, &png::turbine_16x16, point_i16_t({ WizardDefaults::col_0, row_3 }))
    , text_print_fan(this, Rect16(col_texts, row_3, col_texts_w, WizardDefaults::txt_h), is_multiline::no, is_closed_on_click_t::no, _(en_text_print_fan))
    , fan_states(make_fan_row_array(std::make_index_sequence<HOTENDS>())) {
#if HAS_TOOLCHANGER()
    // when toolchanger enabled, hide results of tools that are not connected
    for (size_t i = 0; i < HOTENDS; i++) {

        if (!prusa_toolchanger.is_tool_enabled(i)) {
            fan_states[i].icon_heatbreak_fan_state.Hide();
            fan_states[i].icon_print_fan_state.Hide();
        }
    }
#endif

    change();
}

void SelftestFrameFans::change() {
    SelftestFans_t dt(data_current);

    for (size_t i = 0; i < HOTENDS; i++) {
        fan_states[i].icon_print_fan_state.SetState(dt.print_fan_state[i]);
        fan_states[i].icon_heatbreak_fan_state.SetState(dt.heatbreak_fan_state[i]);
    }
    progress.SetProgressPercent(dt.tot_progress);
};
