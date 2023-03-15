#include "selftest_frame_temp.hpp"
#include "i18n.h"
#include "wizard_config.hpp"
#include "selftest_heaters_type.hpp"
#include <option/has_toolchanger.h>
#if HAS_TOOLCHANGER()
    #include "module/prusa/toolchanger.h"
#endif

static constexpr size_t icon_w = WizardDefaults::status_icon_w;
static constexpr size_t text_w = WizardDefaults::status_text_w;
static constexpr size_t col_0 = WizardDefaults::MarginLeft;
static constexpr size_t col_1 = WizardDefaults::status_icon_X_pos;

static constexpr size_t txt_h = WizardDefaults::txt_h;
static constexpr size_t row_h = WizardDefaults::row_h;

//noz
static constexpr size_t row_noz_0 = WizardDefaults::row_0;
static constexpr size_t row_noz_1 = row_noz_0 + row_h;
static constexpr size_t row_noz_2 = row_noz_1 + WizardDefaults::progress_row_h;
static constexpr size_t row_noz_3 = row_noz_2 + row_h;
//bed
static constexpr size_t row_bed_0 = row_noz_3 + row_h + 16;
static constexpr size_t row_bed_1 = row_bed_0 + row_h;
static constexpr size_t row_bed_2 = row_bed_1 + WizardDefaults::progress_row_h;
static constexpr size_t row_bed_3 = row_bed_2 + row_h;
//heatbreak
static constexpr size_t row_heatbreak = row_bed_3 + row_h + 16;

static constexpr const char *en_text_noz = N_("Nozzle heater check");
static constexpr const char *en_text_bed = N_("Heatbed heater check");
static constexpr const char *en_text_prep = N_("Preparing");
static constexpr const char *en_text_heat = N_("Heater testing");
static constexpr const char *en_text_heatbreak = N_("Heatbreak status");

ScreenSelftestTemp::hotend_result_t ScreenSelftestTemp::make_hotend_result_row(size_t index) {
    constexpr static int16_t ICON_SPACING = 20;

    [[maybe_unused]] unsigned int disabled = 0;
#if HAS_TOOLCHANGER()
    // when toolchanger is enabled, count how many tools after this one are disabled, and shift icons to the right over the hidden ones

    for (size_t i = index; i < HOTENDS; i++) {
        if (!prusa_toolchanger.is_tool_enabled(i))
            ++disabled;
    }
#endif

    // Align it to the right (to the col_1), omit disabled tools
    const int16_t y = col_1 - HOTENDS * ICON_SPACING + (index + 1 + disabled) * ICON_SPACING;

    return {
        .icon_noz_prep = WindowIcon_OkNg(this, { y, row_noz_2 }),
        .icon_noz_heat = WindowIcon_OkNg(this, { y, row_noz_3 }),
        .icon_heatbreak = WindowIcon_OkNg(this, { y, row_heatbreak }),
    };
}

template <size_t... Is>
std::array<ScreenSelftestTemp::hotend_result_t, sizeof...(Is)> ScreenSelftestTemp::make_hotend_result_array(std::index_sequence<Is...> index_sequence) {
    //  this is just fancy template way to init array in constructor initializer_list
    return { (make_hotend_result_row(Is))... };
}

ScreenSelftestTemp::ScreenSelftestTemp(window_t *parent, PhasesSelftest ph, fsm::PhaseData data)
    : AddSuperWindow<SelftestFrame>(parent, ph, data)
#if HAS_TOOLCHANGER()
    , footer(this, 0, prusa_toolchanger.is_toolchanger_enabled() ? footer::items::ItemAllNozzles : footer::items::ItemNozzle, footer::items::ItemBed)
#else
    , footer(this, 0, footer::items::ItemNozzle, footer::items::ItemBed, footer::items::ItemHeatbreak)
#endif
    //noz
    , text_noz(this, Rect16(col_0, row_noz_0, WizardDefaults::X_space, txt_h), is_multiline::no, is_closed_on_click_t::no, _(en_text_noz))
    , progress_noz(this, row_noz_1)
    , text_noz_prep(this, Rect16(col_0, row_noz_2, text_w, txt_h), is_multiline::no, is_closed_on_click_t::no, _(en_text_prep))
    , text_noz_heat(this, Rect16(col_0, row_noz_3, text_w, txt_h), is_multiline::no, is_closed_on_click_t::no, _(en_text_heat))

    //bed
    , text_bed(this, Rect16(col_0, row_bed_0, WizardDefaults::X_space, txt_h), is_multiline::no, is_closed_on_click_t::no, _(en_text_bed))
    , progress_bed(this, row_bed_1)
    , text_bed_prep(this, Rect16(col_0, row_bed_2, text_w, txt_h), is_multiline::no, is_closed_on_click_t::no, _(en_text_prep))
    , icon_bed_prep(this, { col_1, row_bed_2 })
    , text_bed_heat(this, Rect16(col_0, row_bed_3, text_w, txt_h), is_multiline::no, is_closed_on_click_t::no, _(en_text_heat))
    , icon_bed_heat(this, { col_1, row_bed_3 })
    //heatbreak
    , text_heatbreak(this, Rect16(col_0, row_heatbreak, text_w, txt_h), is_multiline::no, is_closed_on_click_t::no, _(en_text_heatbreak))

    // results
    , hotend_results(make_hotend_result_array(std::make_index_sequence<HOTENDS>())) {

#if HAS_TOOLCHANGER()
    // when toolchanger enabled, hide results of tools that are not connected
    for (size_t i = 0; i < HOTENDS; i++) {

        if (!prusa_toolchanger.is_tool_enabled(i)) {
            hotend_results[i].icon_noz_prep.Hide();
            hotend_results[i].icon_noz_heat.Hide();
            hotend_results[i].icon_heatbreak.Hide();
        }
    }
#endif
    change();
}

void ScreenSelftestTemp::change() {
    SelftestHeaters_t dt;
    if (FSMExtendedDataManager::get(dt)) {
        uint8_t progress = std::numeric_limits<decltype(progress)>::max();

        for (size_t i = 0; i < HOTENDS; i++) {
            hotend_results[i].icon_noz_prep.SetState(dt.noz[i].prep_state);
            hotend_results[i].icon_noz_heat.SetState(dt.noz[i].heat_state);
            hotend_results[i].icon_heatbreak.SetState(dt.noz[i].heatbreak_error ? SelftestSubtestState_t::not_good : SelftestSubtestState_t::ok);
            progress = std::min(dt.noz[i].progress, dt.noz[i].progress);
        }
        progress_noz.SetProgressPercent(progress);

        icon_bed_prep.SetState(dt.bed.prep_state);
        icon_bed_heat.SetState(dt.bed.heat_state);
        progress_bed.SetProgressPercent(dt.bed.progress);
    }
};
