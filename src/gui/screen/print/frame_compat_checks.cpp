#include "frame_compat_checks.hpp"

#include <window_menu_callback_item.hpp>
#include <feature/compatibility_checks/gcode_compatibility.hpp>
#include <feature/compatibility_checks/filament_compatibility.hpp>
#include <tools_mapping.hpp>
#include <client_response_texts.hpp>
#include <marlin_client.hpp>
#include <img_resources.hpp>
#include <window_msgbox.hpp>
#include <gui/auto_layout.hpp>

using namespace buddy;

namespace screen_print_preview {

WindowMenuCompatibilityChecks::WindowMenuCompatibilityChecks(window_t *parent, Rect16 rect)
    : WindowMenuVirtual(parent, rect, CloseScreenReturnBehavior::no) {
}

void WindowMenuCompatibilityChecks::setup(Mode mode) {
    failed_checks_.clear();

    const auto add_items_from_report = [&](const auto &report, auto... visitor_args) {
        report.visit_failed_checks([this](const auto &fail) {
            failed_checks_.push_back(fail.meta);
            return failed_checks_.size() != failed_checks_.max_size();
        },
            visitor_args...);
    };

    match(
        mode, //
        [&](GCodeMode) {
            gcode_compatibility::CompatibilityReport report;
            if (tools_mapping::is_tool_mapping_possible()) {
                // Only report non-tool related problems
                // Tool checks will be handled on the tool mapping screen
                report.generate_without_toolmapping();

            } else {
                // There will be no separate tooomapping screen,
                // so show all problems, with the naive 1:1 toolmapping
                report.generate_full({});
            }

            add_items_from_report(report, gcode_compatibility::CompatibilityReport::AggregateTools::yes); //
        },
        [&](FilamentMode m) {
            filament_compatibility::CompatibilityReport report;
            report.generate_noclear({
                .filament = m.filament.parameters(),
                .tools = m.tool,
                .assume_filament_already_inserted = m.assume_filament_already_inserted,
            });

            add_items_from_report(report); //
        });

    setup_items();
}

int screen_print_preview::WindowMenuCompatibilityChecks::item_count() const {
    return failed_checks_.size();
}

void screen_print_preview::WindowMenuCompatibilityChecks::setup_item(ItemVariant &variant, int index) {
    const auto &meta = *failed_checks_[index];
    const auto compatibility_level = meta.evaluate_compatibility();

    const auto cb = [&meta] {
        if (meta.description) {
            MsgBoxInfo(_(meta.description), Responses_Ok);
        }
    };

    auto &item = variant.emplace<WindowMenuCallbackItem>(_(meta.title), cb);
    item.set_color_scheme(buddy::compatibility_checks::compatibility_level_menu_item_color_schemes[compatibility_level]);

#if HAS_MINI_DISPLAY()
    item.setLabelFont(Font::small);
#else
    item.SetIconId(buddy::compatibility_checks::compatibility_level_icons[compatibility_level]);
#endif

    if (meta.description) {
        item.set_show_expand_icon();
    }
}

FrameCompatibilityChecks::FrameCompatibilityChecks(window_frame_t *parent, FSMAndPhase phase)
    : title_(parent, Rect16 {}, is_multiline::yes)
    , title_line_(parent, {})
    , menu_(parent, Rect16 {})
    , radio_(parent, Rect16 {}, phase)
    , phase_(phase) {

    title_.SetAlignment(Align_t::LeftBottom());
#if HAS_MINI_DISPLAY()
    title_.set_font(Font::small);
#endif

    title_line_.SetBackColor(COLOR_DARK_GRAY);

    static constexpr std::array layout {
        // Title
        StackLayoutItem {
            .height = 32,
            .margin_side = 16,
            .margin_top = 4,
            .margin_bottom = 4,
        },

        // Title line
        StackLayoutItem {
            .height = 2,
            .margin_side = 16,
            .margin_bottom = 4,
        },

        // Incompatibilities list (menu)
        StackLayoutItem {
            .height = StackLayoutItem::stretch,
#if !HAS_MINI_DISPLAY()
            .margin_side = 16,
#endif
        },

        // Radio
        standard_stack_layout::for_radio,
    };
    auto windows = std::to_array<window_t *>({
        &title_,
        &title_line_,
        &menu_,
        &radio_,
    });

    layout_vertical_stack(menu_.GetParent()->GetRect(), windows, layout);

    // Do NOT capture anything - the window frame itself should be able to handle passing events through
    // and make KNOB events focus previous/next element
    // parent->CaptureNormalWindow(nullptr);
    // But we gotta set focus to the radio
    radio_.SetFocus();
}

void FrameCompatibilityChecks::setup(Mode mode) {
    match(
        mode, //
        [this](GCodeMode) { title_.SetText(_("G-Code incompatibilities detected")); }, //
        [this](FilamentMode) { title_.SetText(_("Filament incompatibility detected")); } //
    );

    menu_.menu.setup(mode);
}

void FrameCompatibilityChecks::update(const fsm::PhaseData &data) {
    debug_assert(phase_.fsm == ClientFSM::PrintPreview);
    switch (static_cast<PhasesPrintPreview>(phase_.phase)) {

    case PhasesPrintPreview::filament_incompatible_fatal:
    case PhasesPrintPreview::filament_incompatible_warning: {
        const auto d = fsm::deserialize_data<fsm_print_preview::FilamentIncompatibleData>(data);
        setup(FilamentMode {
            .filament = EncodedFilamentType::from_data(d.encoded_filament).decode(),
            .tool = VirtualToolIndex::from_raw(d.target_virtual_tool),
            .assume_filament_already_inserted = d.assume_filament_already_inserted,
        });
        break;
    }

    case PhasesPrintPreview::gcode_incompatible_fatal:
    case PhasesPrintPreview::gcode_incompatible_warning:
        setup(GCodeMode {});
        break;

    default:
        bsod_unreachable();
    }
}

} // namespace screen_print_preview
