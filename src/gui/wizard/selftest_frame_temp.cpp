#include "selftest_frame_temp.hpp"
#include "i18n.h"
#include <guiconfig/wizard_config.hpp>
#include "selftest_heaters_type.hpp"
#include <option/has_toolchanger.h>
#if HAS_TOOLCHANGER()
    #include "module/prusa/toolchanger.h"
#endif

#include "img_resources.hpp"
#include "marlin_server_extended_fsm_data.hpp"

namespace {
constexpr size_t icon_w = WizardDefaults::status_icon_w;
constexpr size_t text_w = WizardDefaults::status_text_w;
constexpr size_t col_0 = WizardDefaults::MarginLeft;
constexpr size_t col_1 = WizardDefaults::status_icon_X_pos;

constexpr int16_t tool_icons_x = int16_t(col_1 - HOTENDS * WindowIconOkNgArray::icon_space_width);

constexpr size_t txt_h = WizardDefaults::txt_h;
constexpr size_t row_h = WizardDefaults::row_h;

// noz
constexpr size_t row_noz_0 = WizardDefaults::row_0;
constexpr size_t row_noz_1 = row_noz_0 + row_h;
constexpr size_t row_noz_2 = row_noz_1 + WizardDefaults::progress_row_h;
constexpr size_t row_noz_3 = row_noz_2 + row_h;
// heatbreak
constexpr size_t row_heatbreak = row_noz_3 + row_h;

constexpr size_t row_info = row_heatbreak + row_h + GuiDefaults::FramePadding;

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

constexpr Rect16 info_text_rect { col_0, row_info, text_w, txt_h * 4 };

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
// dialog is for non-toolchanger printers
constexpr const char *en_text_dialog_noz_disabled = N_("The heater test will be skipped due to the failed hotend fan check. You may continue, but we strongly recommend resolving this issue before you start printing.");
// info text without a dialog is for >1 tool ToolChanger
constexpr const char *en_text_info_noz_disabled = N_("Some nozzle heater checks were disabled due to their hotend fan checks not having passed.");
} // namespace

static bool is_tested(SelftestHeaters_t &dt, SelftestHeaters_t::TestedParts part) {
    return dt.tested_parts & to_one_hot(part);
}

ScreenSelftestTemp::ScreenSelftestTemp(window_t *parent, PhasesSelftest ph, fsm::PhaseData data)
    : SelftestFrameWithRadio(parent, ph, data, 1)
#if HAS_TOOLCHANGER()
    , footer(this, 0, prusa_toolchanger.is_toolchanger_enabled() ? footer::Item::all_nozzles : footer::Item::nozzle, footer::Item::bed)
#else
    , footer(this, 0, footer::Item::nozzle, footer::Item::bed, footer::Item::heatbreak_temp)
#endif
    , test_frame(this, GetRect())
    // noz
    , text_noz(&test_frame, top_label_rect, is_multiline::no, is_closed_on_click_t::no, _(en_text_noz))
    , progress_noz(&test_frame, top_progress_rect.Top())
    , text_noz_prep(&test_frame, top_row_0_text_rect, is_multiline::no, is_closed_on_click_t::no, _(en_text_prep))
    , text_noz_heat(&test_frame, top_row_1_text_rect, is_multiline::no, is_closed_on_click_t::no, _(en_text_heat))

    // bed
    , text_bed(&test_frame, bottom_label_rect, is_multiline::no, is_closed_on_click_t::no, _(en_text_bed))
    , progress_bed(&test_frame, bottom_progress_rect.Top())
    , text_bed_prep(&test_frame, bottom_row_0_text_rect, is_multiline::no, is_closed_on_click_t::no, _(en_text_prep))
    , icon_bed_prep(&test_frame, bottom_row_0_icon_rect.TopLeft())
    , text_bed_heat(&test_frame, bottom_row_1_text_rect, is_multiline::no, is_closed_on_click_t::no, _(en_text_heat))
    , icon_bed_heat(&test_frame, bottom_row_0_icon_rect.TopLeft())
#if HAS_HEATBREAK_TEMP()
    // heatbreak
    , text_heatbreak(&test_frame, heatbreak_text_rect, is_multiline::no, is_closed_on_click_t::no, _(en_text_heatbreak))
#endif
    , text_info(&test_frame, info_text_rect, is_multiline::yes)
    , text_dialog(this, GetRect() + Rect16::X_t(WizardDefaults::MarginLeft) + Rect16::Y_t(GuiDefaults::FramePadding) - Rect16::H_t(80) - Rect16::W_t(2 * WizardDefaults::MarginLeft), is_multiline::yes, is_closed_on_click_t::no, {})
    // results
    , icons_noz_prep(&test_frame, { .x = tool_icons_x, .y = row_noz_2 }, HOTENDS)
    , icons_noz_heat(&test_frame, { .x = tool_icons_x, .y = row_noz_3 }, HOTENDS)
#if HAS_HEATBREAK_TEMP()
    , icons_heatbreak(&test_frame, { .x = tool_icons_x, .y = row_heatbreak }, HOTENDS)
#endif
{

#if HAS_TOOLCHANGER()
    // when toolchanger enabled, hide results of tools that are not connected
    for (size_t i = 0; i < HOTENDS; i++) {
        if (!prusa_toolchanger.is_tool_enabled(i)) {
            icons_noz_prep.SetIconHidden(i, true);
            icons_noz_heat.SetIconHidden(i, true);
    #if HAS_HEATBREAK_TEMP()
            icons_heatbreak.SetIconHidden(i, true);
    #endif
        }
    }
#endif
    footer.Erase(FOOTER_ITEMS_PER_LINE__); // Erase all footer items
    SelftestHeaters_t dt;
    if (FSMExtendedDataManager::get(dt)) {
        if (is_tested(dt, SelftestHeaters_t::TestedParts::noz) && is_tested(dt, SelftestHeaters_t::TestedParts::bed)) {
            // Nozzle & Bed are both tested
#if HAS_TOOLCHANGER()
            footer.Create(prusa_toolchanger.is_toolchanger_enabled() ? footer::Item::all_nozzles : footer::Item::nozzle, 0);
#else
            footer.Create(footer::Item::nozzle, 0);
#endif
            footer.Create(footer::Item::bed, 1);

            text_bed.SetRect(bottom_label_rect);
            progress_bed.SetRect(bottom_progress_rect);
            text_bed_prep.SetRect(bottom_row_0_text_rect);
            icon_bed_prep.SetRect(bottom_row_0_icon_rect);
            text_bed_heat.SetRect(bottom_row_1_text_rect);
            icon_bed_heat.SetRect(bottom_row_1_icon_rect);

        } else if (is_tested(dt, SelftestHeaters_t::TestedParts::noz)) {
            // ONLY nozzle is tested
#if HAS_TOOLCHANGER()
            footer.Create(prusa_toolchanger.is_toolchanger_enabled() ? footer::Item::all_nozzles : footer::Item::nozzle, 0);
#else
            footer.Create(footer::Item::nozzle, 0);
#endif
        } else if (is_tested(dt, SelftestHeaters_t::TestedParts::bed)) {
            // ONLY bed is tested
            footer.Create(footer::Item::bed, 0);

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
                    icons_noz_prep.SetIconHidden(i, false);
                    icons_noz_heat.SetIconHidden(i, false);
#if HAS_HEATBREAK_TEMP()
                    icons_heatbreak.SetIconHidden(i, false);
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
                    icons_noz_prep.SetIconHidden(i, true);
                    icons_noz_heat.SetIconHidden(i, true);
#if HAS_HEATBREAK_TEMP()
                    icons_heatbreak.SetIconHidden(i, true);
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

        radio.Hide();
        text_dialog.Hide();
    }

    change();
}

void ScreenSelftestTemp::change() {
    switch (phase_current) {

    case PhasesSelftest::Heaters_AskBedSheetAfterFail: {
        test_frame.Hide();
        text_dialog.Show();
        text_dialog.SetText(_("Bed heater selftest failed.\n\nIf you forgot to put the steel sheet on the heatbed, place it on and press Retry."));
        radio.Show();
        break;
    }

    case PhasesSelftest::HeatersDisabledDialog:
#if HAS_TOOLCHANGER()
        if (prusa_toolchanger.get_num_enabled_tools() > 1) {
            [[fallthrough]]; // >1 tool toolchanger falls through
        } else
#endif
        {
            test_frame.Hide();
            text_dialog.Show();
            text_dialog.SetText(_(en_text_dialog_noz_disabled));
            radio.Show();
            break;
        }

    case PhasesSelftest::Heaters: {
        text_dialog.Hide();
        test_frame.Show();
        radio.Hide();

#if HAS_TOOLCHANGER()
        // only multitool has access to text_info
        if (prusa_toolchanger.get_num_enabled_tools() > 1 && phase_current == PhasesSelftest::HeatersDisabledDialog) {
            text_info.SetText(_(en_text_info_noz_disabled));
        } else
#endif
        { // otherwise we don't want to show it
            text_info.Hide();
        }

        SelftestHeaters_t dt;
        if (FSMExtendedDataManager::get(dt)) {
            if (is_tested(dt, SelftestHeaters_t::TestedParts::noz)) {

                uint8_t progress = std::numeric_limits<decltype(progress)>::max();

                for (size_t i = 0; i < HOTENDS; i++) {
#if HAS_TOOLCHANGER()
                    if (prusa_toolchanger.is_tool_enabled(i))
#endif
                    {
                        icons_noz_prep.SetState(dt.noz[i].prep_state, i);
                        icons_noz_heat.SetState(dt.noz[i].heat_state, i);
#if HAS_HEATBREAK_TEMP()
                        icons_heatbreak.SetState(dt.noz[i].heatbreak_error ? SelftestSubtestState_t::not_good : SelftestSubtestState_t::ok, i);
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
        break;
    }
    default:
        break;
    }
}
