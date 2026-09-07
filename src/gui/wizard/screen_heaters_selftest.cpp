#include "screen_heaters_selftest.hpp"

#include <i18n.h>
#include <guiconfig/wizard_config.hpp>
#include <guiconfig/GuiDefaults.hpp>
#include <img_resources.hpp>
#include <window_wizard_icon.hpp>
#include <window_text.hpp>
#include <window_wizard_progress.hpp>
#include <status_footer.hpp>
#include <marlin_server_extended_fsm_data.hpp>
#include <selftest_heaters_type.hpp>
#include <standard_frame/frame_text_prompt.hpp>
#include <standard_frame/frame_wait.hpp>
#include <option/has_heatbreak_temp.h>
#include <option/has_indx.h>
#include <option/has_heaters_selftest_bed_sheet_retry.h>
#include <option/has_heaters_selftest_revise.h>

#if HAS_HEATERS_SELFTEST_REVISE()
    #include <ScreenHandler.hpp>
    #include <gui/screen_printer_setup.hpp>
#endif

namespace {

// Layout (single hotend), modelled on selftest_frame_temp.cpp.
constexpr size_t txt_h = WizardDefaults::txt_h;
constexpr size_t row_h = WizardDefaults::row_h;
constexpr size_t col_0 = WizardDefaults::MarginLeft;
constexpr size_t col_1 = WizardDefaults::status_icon_X_pos;
constexpr size_t text_w = WizardDefaults::status_text_w;

constexpr const img::Resource &reference_icon { img::dash_18x18 };

// nozzle rows
constexpr size_t row_noz_0 = WizardDefaults::row_0; // label
constexpr size_t row_noz_1 = row_noz_0 + row_h; // progress
constexpr size_t row_noz_2 = row_noz_1 + WizardDefaults::progress_row_h; // "Preparing"
constexpr size_t row_noz_3 = row_noz_2 + row_h; // "Heater testing"
constexpr size_t row_heatbreak = row_noz_3 + row_h; // "Heatbreak status"
// bed rows
constexpr size_t row_bed_0 = row_heatbreak + row_h + 16; // label
constexpr size_t row_bed_1 = row_bed_0 + row_h; // progress
constexpr size_t row_bed_2 = row_bed_1 + WizardDefaults::progress_row_h; // "Preparing"
constexpr size_t row_bed_3 = row_bed_2 + row_h; // "Heater testing"

constexpr Rect16 noz_label_rect { col_0, row_noz_0, WizardDefaults::X_space, txt_h };
constexpr Rect16 noz_progress_rect { WizardDefaults::progress_LR_margin, row_noz_1, WizardDefaults::progress_width, WizardDefaults::progress_h };
constexpr Rect16 noz_prep_text_rect { col_0, row_noz_2, text_w, txt_h };
constexpr Rect16 noz_prep_icon_rect { col_1, row_noz_2, reference_icon.w, reference_icon.h };
constexpr Rect16 noz_heat_text_rect { col_0, row_noz_3, text_w, txt_h };
constexpr Rect16 noz_heat_icon_rect { col_1, row_noz_3, reference_icon.w, reference_icon.h };
#if HAS_HEATBREAK_TEMP()
constexpr Rect16 heatbreak_text_rect { col_0, row_heatbreak, text_w, txt_h };
constexpr Rect16 heatbreak_icon_rect { col_1, row_heatbreak, reference_icon.w, reference_icon.h };
#endif
constexpr Rect16 bed_label_rect { col_0, row_bed_0, WizardDefaults::X_space, txt_h };
constexpr Rect16 bed_progress_rect { WizardDefaults::progress_LR_margin, row_bed_1, WizardDefaults::progress_width, WizardDefaults::progress_h };
constexpr Rect16 bed_prep_text_rect { col_0, row_bed_2, text_w, txt_h };
constexpr Rect16 bed_prep_icon_rect { col_1, row_bed_2, reference_icon.w, reference_icon.h };
constexpr Rect16 bed_heat_text_rect { col_0, row_bed_3, text_w, txt_h };
constexpr Rect16 bed_heat_icon_rect { col_1, row_bed_3, reference_icon.w, reference_icon.h };

constexpr const char *en_text_noz = N_("Nozzle heater check");
constexpr const char *en_text_bed = N_("Heatbed heater check");
constexpr const char *en_text_prep = N_("Preparing");
constexpr const char *en_text_heat = N_("Heater testing");
#if HAS_HEATBREAK_TEMP()
constexpr const char *en_text_heatbreak = N_("Heatbreak status");
#endif
#if HAS_INDX()
constexpr auto txt_picking_tool = N_("Picking up tool");
#endif
constexpr auto txt_fan_failed = N_("The heater test will be skipped due to the failed hotend fan check. You may continue, but we strongly recommend resolving this issue before you start printing.");
#if HAS_HEATERS_SELFTEST_BED_SHEET_RETRY()
constexpr auto txt_ask_bed_sheet = N_("Bed heater selftest failed.\n\nIf you forgot to put the steel sheet on the heatbed, place it on and press Retry.");
#endif
#if HAS_HEATERS_SELFTEST_REVISE()
constexpr auto txt_revise_ask_revise = N_("Attention, the test has failed.\nThis could have been caused by a wrong configuration.\n\nDo you want to revise your printer configuration?");
constexpr auto txt_revise_ask_retry = N_("Do you wish to retry the failed selftest?");
#endif

/// Live "heating" frame: nozzle + bed progress and prep/heat state icons, driven by HeatersSelftestData.
class FrameHeating {
    window_text_t text_noz;
    window_wizard_progress_t progress_noz;
    window_text_t text_noz_prep;
    WindowIcon_OkNg icon_noz_prep;
    window_text_t text_noz_heat;
    WindowIcon_OkNg icon_noz_heat;
#if HAS_HEATBREAK_TEMP()
    window_text_t text_heatbreak;
    WindowIcon_OkNg icon_heatbreak;
#endif
    window_text_t text_bed;
    window_wizard_progress_t progress_bed;
    window_text_t text_bed_prep;
    WindowIcon_OkNg icon_bed_prep;
    window_text_t text_bed_heat;
    WindowIcon_OkNg icon_bed_heat;

public:
    explicit FrameHeating(window_frame_t *parent)
        : text_noz(parent, noz_label_rect, is_multiline::no, is_closed_on_click_t::no, _(en_text_noz))
        , progress_noz(parent, noz_progress_rect.Top())
        , text_noz_prep(parent, noz_prep_text_rect, is_multiline::no, is_closed_on_click_t::no, _(en_text_prep))
        , icon_noz_prep(parent, noz_prep_icon_rect.TopLeft())
        , text_noz_heat(parent, noz_heat_text_rect, is_multiline::no, is_closed_on_click_t::no, _(en_text_heat))
        , icon_noz_heat(parent, noz_heat_icon_rect.TopLeft())
#if HAS_HEATBREAK_TEMP()
        , text_heatbreak(parent, heatbreak_text_rect, is_multiline::no, is_closed_on_click_t::no, _(en_text_heatbreak))
        , icon_heatbreak(parent, heatbreak_icon_rect.TopLeft())
#endif
        , text_bed(parent, bed_label_rect, is_multiline::no, is_closed_on_click_t::no, _(en_text_bed))
        , progress_bed(parent, bed_progress_rect.Top())
        , text_bed_prep(parent, bed_prep_text_rect, is_multiline::no, is_closed_on_click_t::no, _(en_text_prep))
        , icon_bed_prep(parent, bed_prep_icon_rect.TopLeft())
        , text_bed_heat(parent, bed_heat_text_rect, is_multiline::no, is_closed_on_click_t::no, _(en_text_heat))
        , icon_bed_heat(parent, bed_heat_icon_rect.TopLeft()) {
        update({});
    }

    void update(fsm::PhaseData) {
        HeatersSelftestData dt;
        if (!FSMExtendedDataManager::get(dt)) {
            return;
        }
        progress_noz.set_progress_percent(dt.noz.progress);
        icon_noz_prep.SetState(dt.noz.prep_state);
        icon_noz_heat.SetState(dt.noz.heat_state);
#if HAS_HEATBREAK_TEMP()
        icon_heatbreak.SetState(dt.noz.heatbreak_error ? SelftestSubtestState_t::not_good : SelftestSubtestState_t::ok);
#endif
        progress_bed.set_progress_percent(dt.bed.progress);
        icon_bed_prep.SetState(dt.bed.prep_state);
        icon_bed_heat.SetState(dt.bed.heat_state);
    }
};

#if HAS_INDX()
// Reason texts incl. the measured value, so support can diagnose from a screenshot. Each
// overload owns its whole text, making the dispatch over NozzleError total — no fallback needed.
string_view_utf8 nozzle_fail_text(std::monostate, StringViewUtf8ParamBase &) {
    return _("Nozzle heater test failed.");
}
string_view_utf8 nozzle_fail_text(const heaters_selftest::ThermalProtectionTripped &e, StringViewUtf8ParamBase &params) {
    return _("Nozzle heater test failed.\n\nThe nozzle did not heat up as expected. Check that the nozzle is seated correctly in the head and try again.\n\nError code: %u").formatted(params, e.error_code);
}
string_view_utf8 nozzle_fail_text(const heaters_selftest::HeatTimeout &e, StringViewUtf8ParamBase &params) {
    return _("Nozzle heater test failed.\n\nThe nozzle did not reach the target temperature in time. Check that the nozzle is seated correctly in the head and try again.\n\nReached temperature: %u\xC2\xB0\x43").formatted(params, e.temperature_c);
}
// Which side of the window was violated is evident from the measured value, so both share one text.
string_view_utf8 nozzle_fail_coil_text(uint16_t current_ma, StringViewUtf8ParamBase &params) {
    return _("Nozzle heater test failed.\n\nThe heater coil current was out of the expected range. Check that the nozzle is seated correctly in the head. If the issue persists, inspect the coil for damage.\n\nMeasured current: %u mA").formatted(params, current_ma);
}
string_view_utf8 nozzle_fail_text(const heaters_selftest::CoilUndercurrent &e, StringViewUtf8ParamBase &params) {
    return nozzle_fail_coil_text(e.current_ma, params);
}
string_view_utf8 nozzle_fail_text(const heaters_selftest::CoilOvercurrent &e, StringViewUtf8ParamBase &params) {
    return nozzle_fail_coil_text(e.current_ma, params);
}

/// nozzle_failed_dialog phase: why the nozzle failed arrives as heaters_selftest::NozzleError.
class FrameNozzleFailed : public FrameTextPrompt {
public:
    explicit FrameNozzleFailed(window_frame_t *parent)
        : FrameTextPrompt(parent, PhasesHeatersSelftest::nozzle_failed_dialog, {}) {}

    void update(fsm::PhaseData data) {
        const auto error = fsm::deserialize_data<heaters_selftest::NozzleError>(data);
        info.SetText(std::visit([&](const auto &e) { return nozzle_fail_text(e, params); }, error));
    }

private:
    StringViewUtf8Parameters<8> params;
};
#endif

#if HAS_HEATERS_SELFTEST_REVISE()
/// revise_revise phase: opens the printer setup screen on top; it sends Response::Done on "Done".
/// Opened unconditionally: a ScreenPrinterSetup buried lower in the stack must not suppress the
/// open — it is not interactive there, so the phase would be a blank screen nobody can answer.
class FrameRevise {
public:
    explicit FrameRevise(window_frame_t *) {
        Screens::Access()->Open<ScreenPrinterSetup>();
    }
    void update(fsm::PhaseData) {}
};
#endif

// The last entry must be unconditional so the template argument list never ends with a trailing comma.
using Frames = FrameDefinitionList<ScreenHeatersSelftest::FrameStorage,
#if HAS_INDX()
    FrameDefinition<PhasesHeatersSelftest::picking_tool, FrameWait, txt_picking_tool>,
    FrameDefinition<PhasesHeatersSelftest::nozzle_failed_dialog, FrameNozzleFailed>,
#endif
#if HAS_HEATERS_SELFTEST_BED_SHEET_RETRY()
    FrameDefinition<PhasesHeatersSelftest::ask_bed_sheet_after_fail, FrameTextPrompt, PhasesHeatersSelftest::ask_bed_sheet_after_fail, txt_ask_bed_sheet>,
#endif
#if HAS_HEATERS_SELFTEST_REVISE()
    FrameDefinition<PhasesHeatersSelftest::revise_ask_revise, FrameTextPrompt, PhasesHeatersSelftest::revise_ask_revise, txt_revise_ask_revise>,
    FrameDefinition<PhasesHeatersSelftest::revise_revise, FrameRevise>,
    FrameDefinition<PhasesHeatersSelftest::revise_ask_retry, FrameTextPrompt, PhasesHeatersSelftest::revise_ask_retry, txt_revise_ask_retry>,
#endif
    FrameDefinition<PhasesHeatersSelftest::heating, FrameHeating>,
    FrameDefinition<PhasesHeatersSelftest::hotend_fan_failed_dialog, FrameTextPrompt, PhasesHeatersSelftest::hotend_fan_failed_dialog, txt_fan_failed>>;

} // namespace

ScreenHeatersSelftest::ScreenHeatersSelftest()
    : ScreenFSM { N_("HEATER TEST"), GuiDefaults::RectScreenBody }
#if HAS_HEATBREAK_TEMP()
    , footer(this, 0, footer::Item::nozzle, footer::Item::bed, footer::Item::heatbreak_temp)
#else
    , footer(this, 0, footer::Item::nozzle, footer::Item::bed)
#endif
{
    header.SetIcon(&img::selftest_16x16);
    create_frame();
}

ScreenHeatersSelftest::~ScreenHeatersSelftest() {
    destroy_frame();
}

void ScreenHeatersSelftest::create_frame() {
    Frames::create_frame(frame_storage, get_phase(), &inner_frame);
}

void ScreenHeatersSelftest::destroy_frame() {
    Frames::destroy_frame(frame_storage, get_phase());
}

void ScreenHeatersSelftest::update_frame() {
    Frames::update_frame(frame_storage, get_phase(), fsm_base_data.GetData());
}
