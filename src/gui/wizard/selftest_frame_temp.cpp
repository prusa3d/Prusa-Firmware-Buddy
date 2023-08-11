#include "selftest_frame_temp.hpp"
#include "i18n.h"
#include "wizard_config.hpp"
#include "selftest_heaters_type.hpp"
#include <option/has_toolchanger.h>
#if HAS_TOOLCHANGER()
    #include "module/prusa/toolchanger.h"
#endif

#include "img_resources.hpp"

namespace {
constexpr size_t icon_w = WizardDefaults::status_icon_w;
constexpr size_t text_w = WizardDefaults::status_text_w;
constexpr size_t col_0 = WizardDefaults::MarginLeft;
constexpr size_t col_1 = WizardDefaults::status_icon_X_pos;

constexpr size_t txt_h = WizardDefaults::txt_h;
constexpr size_t row_h = WizardDefaults::row_h;

// noz
constexpr size_t row_noz_0 = WizardDefaults::row_0;
constexpr size_t row_noz_1 = row_noz_0 + row_h;
constexpr size_t row_noz_2 = row_noz_1 + WizardDefaults::progress_row_h;
constexpr size_t row_noz_3 = row_noz_2 + row_h;
// heatbreak
constexpr size_t row_heatbreak = row_noz_3 + row_h;
// bed
constexpr size_t row_bed_0 = row_heatbreak + row_h + 16;
constexpr size_t row_bed_1 = row_bed_0 + row_h;
constexpr size_t row_bed_2 = row_bed_1 + WizardDefaults::progress_row_h;
constexpr size_t row_bed_3 = row_bed_2 + row_h;

constexpr const img::Resource &reference_icon { img::dash_18x18 };

constexpr Rect16 top_label_rect { col_0, row_noz_0, WizardDefaults::X_space, txt_h };
constexpr Rect16 top_progress_rect { WizardDefaults::progress_LR_margin, row_noz_1, WizardDefaults::progress_width, WizardDefaults::progress_h };
constexpr Rect16 top_row_0_text_rect { col_0, row_noz_2, text_w, txt_h };
constexpr Rect16 top_row_0_icon_rect { col_1, row_noz_2, reference_icon.w, reference_icon.h }; // valid for one icon
constexpr Rect16 top_row_1_text_rect { col_0, row_noz_3, text_w, txt_h };
constexpr Rect16 top_row_1_icon_rect { col_1, row_noz_3, reference_icon.w, reference_icon.h }; // valid for one icon
constexpr Rect16 heatbreak_text_rect { col_0, row_heatbreak, text_w, txt_h };

constexpr Rect16 bottom_label_rect { col_0, row_bed_0, WizardDefaults::X_space, txt_h };
constexpr Rect16 bottom_progress_rect { WizardDefaults::progress_LR_margin, row_bed_1, WizardDefaults::progress_width, WizardDefaults::progress_h };
constexpr Rect16 bottom_row_0_text_rect { col_0, row_bed_2, text_w, txt_h };
constexpr Rect16 bottom_row_0_icon_rect { col_1, row_bed_2, reference_icon.w, reference_icon.h };
constexpr Rect16 bottom_row_1_text_rect { col_0, row_bed_3, text_w, txt_h };
constexpr Rect16 bottom_row_1_icon_rect { col_1, row_bed_3, reference_icon.w, reference_icon.h };

constexpr const char *en_text_noz = N_("Nozzle heater check");
constexpr const char *en_text_bed = N_("Heatbed heater check");
constexpr const char *en_text_prep = N_("Preparing");
constexpr const char *en_text_heat = N_("Heater testing");
constexpr const char *en_text_heatbreak = N_("Heatbreak status");
} // namespace

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
#if HAS_HEATBREAK_TEMP()
        .icon_heatbreak = WindowIcon_OkNg(this, { y, row_heatbreak }),
#endif
    };
}

template <size_t... Is>
std::array<ScreenSelftestTemp::hotend_result_t, sizeof...(Is)> ScreenSelftestTemp::make_hotend_result_array(std::index_sequence<Is...>) {
    //  this is just fancy template way to init array in constructor initializer_list
    return { (make_hotend_result_row(Is))... };
}

static bool is_tested(SelftestHeaters_t &dt, SelftestHeaters_t::TestedParts part) {
    return dt.tested_parts & to_one_hot(part);
}

ScreenSelftestTemp::ScreenSelftestTemp(window_t *parent, PhasesSelftest ph, fsm::PhaseData data)
    : AddSuperWindow<SelftestFrame>(parent, ph, data)
#if HAS_TOOLCHANGER()
    , footer(this, 0, prusa_toolchanger.is_toolchanger_enabled() ? footer::Item::AllNozzles : footer::Item::Nozzle, footer::Item::Bed)
#else
    , footer(this, 0, footer::Item::Nozzle, footer::Item::Bed, footer::Item::Heatbreak)
#endif
    // noz
    , text_noz(this, top_label_rect, is_multiline::no, is_closed_on_click_t::no, _(en_text_noz))
    , progress_noz(this, top_progress_rect.Top())
    , text_noz_prep(this, top_row_0_text_rect, is_multiline::no, is_closed_on_click_t::no, _(en_text_prep))
    , text_noz_heat(this, top_row_1_text_rect, is_multiline::no, is_closed_on_click_t::no, _(en_text_heat))

    // bed
    , text_bed(this, bottom_label_rect, is_multiline::no, is_closed_on_click_t::no, _(en_text_bed))
    , progress_bed(this, bottom_progress_rect.Top())
    , text_bed_prep(this, bottom_row_0_text_rect, is_multiline::no, is_closed_on_click_t::no, _(en_text_prep))
    , icon_bed_prep(this, bottom_row_0_icon_rect.TopLeft())
    , text_bed_heat(this, bottom_row_1_text_rect, is_multiline::no, is_closed_on_click_t::no, _(en_text_heat))
    , icon_bed_heat(this, bottom_row_0_icon_rect.TopLeft())
#if HAS_HEATBREAK_TEMP()
    // heatbreak
    , text_heatbreak(this, heatbreak_text_rect, is_multiline::no, is_closed_on_click_t::no, _(en_text_heatbreak))
#endif
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
    footer.Erase(FOOTER_ITEMS_PER_LINE__); // Erase all footer items
    SelftestHeaters_t dt;
    if (FSMExtendedDataManager::get(dt)) {
        if (is_tested(dt, SelftestHeaters_t::TestedParts::noz) && is_tested(dt, SelftestHeaters_t::TestedParts::bed)) {
            // Nozzle & Bed are both tested
#if HAS_TOOLCHANGER()
            footer.Create(prusa_toolchanger.is_toolchanger_enabled() ? footer::Item::AllNozzles : footer::Item::Nozzle, 0);
#else
            footer.Create(footer::Item::Nozzle, 0);
#endif
            footer.Create(footer::Item::Bed, 1);

            text_bed.SetRect(bottom_label_rect);
            progress_bed.SetRect(bottom_progress_rect);
            text_bed_prep.SetRect(bottom_row_0_text_rect);
            icon_bed_prep.SetRect(bottom_row_0_icon_rect);
            text_bed_heat.SetRect(bottom_row_1_text_rect);
            icon_bed_heat.SetRect(bottom_row_1_icon_rect);

        } else if (is_tested(dt, SelftestHeaters_t::TestedParts::noz)) {
            // ONLY nozzle is tested
#if HAS_TOOLCHANGER()
            footer.Create(prusa_toolchanger.is_toolchanger_enabled() ? footer::Item::AllNozzles : footer::Item::Nozzle, 0);
#else
            footer.Create(footer::Item::Nozzle, 0);
#endif
        } else if (is_tested(dt, SelftestHeaters_t::TestedParts::bed)) {
            // ONLY bed is tested
            footer.Create(footer::Item::Bed, 0);

            text_bed.SetRect(top_label_rect);
            progress_bed.SetRect(top_progress_rect);
            text_bed_prep.SetRect(top_row_0_text_rect);
            icon_bed_prep.SetRect(top_row_0_icon_rect);
            text_bed_heat.SetRect(top_row_1_text_rect);
            icon_bed_heat.SetRect(top_row_1_icon_rect);
        }

        if (is_tested(dt, SelftestHeaters_t::TestedParts::noz)) {
            // Nozzle is tested - all cases
            text_noz.Show();
            progress_noz.Show();
            text_noz_prep.Show();
            text_noz_heat.Show();
#if HAS_HEATBREAK_TEMP()
            text_heatbreak.Show();
#endif
            for (size_t i = 0; i < HOTENDS; i++) {
#if HAS_TOOLCHANGER()
                if (prusa_toolchanger.is_tool_enabled(i))
#endif
                {
                    hotend_results[i].icon_noz_prep.Show();
                    hotend_results[i].icon_noz_heat.Show();
#if HAS_HEATBREAK_TEMP()
                    hotend_results[i].icon_heatbreak.Show();
#endif
                }
            }
        } else {
            text_noz.Hide();
            progress_noz.Hide();
            text_noz_prep.Hide();
            text_noz_heat.Hide();
#if HAS_HEATBREAK_TEMP()
            text_heatbreak.Hide();
#endif
            for (size_t i = 0; i < HOTENDS; i++) {
#if HAS_TOOLCHANGER()
                if (prusa_toolchanger.is_tool_enabled(i))
#endif
                {
                    hotend_results[i].icon_noz_prep.Hide();
                    hotend_results[i].icon_noz_heat.Hide();
#if HAS_HEATBREAK_TEMP()
                    hotend_results[i].icon_heatbreak.Hide();
#endif
                }
            }
        }

        if (is_tested(dt, SelftestHeaters_t::TestedParts::bed)) {
            // Bed is tested - all cases
            text_bed.Show();
            progress_bed.Show();
            text_bed_prep.Show();
            icon_bed_prep.Show();
            text_bed_heat.Show();
            icon_bed_heat.Show();
        } else {
            text_bed.Hide();
            progress_bed.Hide();
            text_bed_prep.Hide();
            icon_bed_prep.Hide();
            text_bed_heat.Hide();
            icon_bed_heat.Hide();
        }
    }

    change();
}

void ScreenSelftestTemp::change() {
    SelftestHeaters_t dt;
    if (FSMExtendedDataManager::get(dt)) {
        if (is_tested(dt, SelftestHeaters_t::TestedParts::noz)) {

            uint8_t progress = std::numeric_limits<decltype(progress)>::max();

            for (size_t i = 0; i < HOTENDS; i++) {
#if HAS_TOOLCHANGER()
                if (prusa_toolchanger.is_tool_enabled(i))
#endif
                {
                    hotend_results[i].icon_noz_prep.SetState(dt.noz[i].prep_state);
                    hotend_results[i].icon_noz_heat.SetState(dt.noz[i].heat_state);
#if HAS_HEATBREAK_TEMP()
                    hotend_results[i].icon_heatbreak.SetState(dt.noz[i].heatbreak_error ? SelftestSubtestState_t::not_good : SelftestSubtestState_t::ok);
#endif
                    progress = std::min(progress, dt.noz[i].progress);
                }
            }
            progress_noz.SetProgressPercent(progress);
        }

        if (is_tested(dt, SelftestHeaters_t::TestedParts::bed)) {

            icon_bed_prep.SetState(dt.bed.prep_state);
            icon_bed_heat.SetState(dt.bed.heat_state);
            progress_bed.SetProgressPercent(dt.bed.progress);
        }
    }
};
