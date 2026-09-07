#include "screen_selftest_fsensors.hpp"

#include <marlin_server_types/fsm/selftest_fsensors_phases.hpp>
#include <gui/footer/footer_line.hpp>
#include <img_resources.hpp>
#include <guiconfig/wizard_config.hpp>
#include <standard_frame/frame_wait.hpp>
#include <standard_frame/frame_prompt.hpp>
#include <selftest/fsensor/selftest_fsensors_config.hpp>
#include <option/has_extruder_fsensor.h>

namespace {

// Don't blame me, copied over from the old fsensor selftest
// Changhing the visuals is out of scope
constexpr size_t row_2 = 140;
constexpr size_t row_3 = 200;

constexpr size_t col_0 = WizardDefaults::MarginLeft;

constexpr size_t text_icon_space = 24;
constexpr size_t icon_left_width = 100;
constexpr size_t icon_right_width = 150;
constexpr size_t text_left_width = WizardDefaults::X_space - icon_right_width - text_icon_space;
constexpr size_t text_right_width = WizardDefaults::X_space - icon_left_width - text_icon_space;

constexpr size_t top_of_changeable_area = WizardDefaults::row_1 + WizardDefaults::progress_h;
constexpr size_t height_of_changeable_area = WizardDefaults::RectRadioButton(1).Top() - top_of_changeable_area;
constexpr Rect16 ChangeableRect = { col_0, top_of_changeable_area, WizardDefaults::X_space, height_of_changeable_area };

#if HAS_TOOLCHANGER()
constexpr size_t tool_description_h = 24;
constexpr Rect16 ToolDescriptionRect = { col_0, top_of_changeable_area, WizardDefaults::X_space, tool_description_h };
constexpr size_t top_of_body_area = top_of_changeable_area + tool_description_h;
constexpr size_t height_of_body_area = height_of_changeable_area - tool_description_h;
#else
constexpr size_t top_of_body_area = top_of_changeable_area;
constexpr size_t height_of_body_area = height_of_changeable_area;
#endif

const char *selftest_title() {
#if HAS_SIDE_FSENSOR() && HAS_EXTRUDER_FSENSOR()
    return N_("Filament sensors calibration");
#else
    return N_("Filament sensor calibration");
#endif
}

using Phase = PhaseSelftestFSensors;

class FrameBase {

public:
    FrameBase(window_frame_t *parent, Phase phase, [[maybe_unused]] PhysicalToolIndex tool)
        :
#if HAS_TOOLCHANGER()
        tool_description(parent, ToolDescriptionRect, is_multiline::no)
        ,
#endif
        radio(parent, WizardDefaults::RectRadioButton(1), phase)
        , footer(parent, 0
#if HAS_EXTRUDER_FSENSOR()
              ,
              footer::Item::f_s_value
#endif
#if HAS_SIDE_FSENSOR()
              ,
              footer::Item::f_s_value_side
#endif
          ) {
#if HAS_TOOLCHANGER()
        tool_description.SetAlignment(Align_t::LeftCenter());
        tool_description.SetText(tool.display_name(tool_description_params));
#endif
        parent->CaptureNormalWindow(radio);
    }

protected:
#if HAS_TOOLCHANGER()
    window_text_t tool_description;
    PhysicalToolIndex::DisplayNameParams tool_description_params;
#endif
    RadioButtonFSM radio;
    FooterLine footer;
};

#if HAS_LARGE_DISPLAY()

/// Copied over from ye old fsensor selftest
class FrameTextAndImage : public FrameBase {

public:
    FrameTextAndImage(window_frame_t *parent, Phase phase, PhysicalToolIndex tool, const string_view_utf8 &text, const img::Resource *image)
        : FrameBase(parent, phase, tool)
        , text_left(parent, Rect16(col_0, top_of_body_area, text_left_width, height_of_body_area), is_multiline::yes)
        , icon_right(parent, Rect16(col_0 + text_left_width + text_icon_space, top_of_body_area, icon_right_width, height_of_body_area), image)

    {
        text_left.SetAlignment(Align_t::LeftCenter());
        text_left.SetText(text);
    }

private:
    window_text_t text_left;
    window_icon_t icon_right;
};

/// Copied over from ye old fsensor selftest
class FrameSpoolAndText : public FrameBase {

public:
    FrameSpoolAndText(window_frame_t *parent, Phase phase, PhysicalToolIndex tool, const string_view_utf8 &text)
        : FrameBase(parent, phase, tool)
        , text_right(parent, Rect16(col_0 + icon_left_width + text_icon_space, top_of_body_area, text_right_width, height_of_body_area), is_multiline::yes)
        , icon_left(parent, Rect16(col_0, top_of_body_area, icon_left_width, height_of_body_area), &img::prusament_spool_white_100x100) {

        text_right.SetAlignment(Align_t::LeftCenter());
        text_right.SetText(text);
    }

private:
    window_text_t text_right;
    window_icon_t icon_left;
};

#else

// No images fit on the small display, show just normal prompts

class FrameTextAndImage : public FrameBase {

public:
    FrameTextAndImage(window_frame_t *parent, Phase phase, PhysicalToolIndex tool, const string_view_utf8 &text, std::nullopt_t = std::nullopt)
        : FrameBase(parent, phase, tool)
        , text_main(parent, Rect16::fromLTRB(8, parent->GetRect().Top(), parent->GetRect().Right() - 8, radio.GetRect().Top()), is_multiline::yes) {
        text_main.SetAlignment(Align_t::LeftCenter());
        text_main.SetText(text);
    }

private:
    window_text_t text_main;
};

using FrameSpoolAndText = FrameTextAndImage;

#endif

class FrameInit : public FrameWait {

public:
    FrameInit(window_frame_t *parent, Phase, PhysicalToolIndex)
        : FrameWait(parent, N_("Preparing for filament sensor calibration")) {}
};

#if PRINTER_IS_PRUSA_MINI()
using FrameAskMiniHasFsensor = WithConstructorArgs<FrameSpoolAndText, N_("Do you have a filament sensor installed?")>;
#endif

class FrameNotConnected : public FramePrompt {

public:
    FrameNotConnected(window_frame_t *parent, Phase phase, PhysicalToolIndex)
        : FramePrompt(parent, phase, _("Filament sensor not connected"), _("Turn off the printer and check that the filament sensor cables are wired correctly.")) {}
};

using FrameOfferUnload = WithConstructorArgs<FrameSpoolAndText,
#if HAS_SIDE_FSENSOR() && HAS_EXTRUDER_FSENSOR()
    N_("Please make sure there is no filament in the tool and side filament sensors.\n\nYou will need filament to finish this test later.")
#elif HAS_SIDE_FSENSOR() || HAS_EXTRUDER_FSENSOR()
    N_("Make sure there is no filament in the filament sensor.\n\nYou will need filament to complete this test later.")
#else
    N_("We need to start without the filament in the extruder. Please make sure there is no filament in the filament sensor.")
#endif
    >;

using FrameAskFilament = WithConstructorArgs<FrameSpoolAndText,
#if HAS_SIDE_FSENSOR() && HAS_EXTRUDER_FSENSOR()
    N_("Is there any filament in the tool or side filament sensors?")
#else
    N_("Is filament in the filament sensor?")
#endif
    >;

class FrameCalibrating : public FrameWait {

public:
    FrameCalibrating(window_frame_t *parent, Phase, PhysicalToolIndex)
        : FrameWait(parent, N_("Calibrating filament sensors")) {}
};

using FrameInsertFilamentNotReady = WithConstructorArgs<FrameTextAndImage,
#if SELFTEST_FSENSOR_EXTRUDER_ASSIST()
    #if HAS_SIDE_FSENSOR() && HAS_EXTRUDER_FSENSOR()
    N_("Push the filament through the side and extruder filament sensors, until the extruder engages with the filament."),
    #elif HAS_EXTRUDER_FSENSOR()
    N_("Push the filament through extruder filament sensor, until the extruder engages with the filament."),
    #else
    N_("Push the filament through filament sensor, until the extruder engages with the filament."),
    #endif
#else
    #if HAS_SIDE_FSENSOR() && HAS_EXTRUDER_FSENSOR()
    N_("Push the filament through the side and extruder filament sensors."),
    #elif HAS_SIDE_FSENSOR()
    N_("Insert the filament into the filament sensor."),
    #else
    N_("Insert the filament into the extruder filament sensor."),
    #endif
#endif
#if HAS_LARGE_DISPLAY()
    &img::hand_with_filament_150x130
#else
    std::nullopt
#endif
    >;

using FrameInsertFilamentReady = WithConstructorArgs<FrameTextAndImage,
    N_("Filament inserted, press continue."),
#if HAS_LARGE_DISPLAY()
    &img::hand_with_filament_ok_150x130
#else
    std::nullopt
#endif
    >;

using FrameRemoveFilamentNotReady = WithConstructorArgs<FrameTextAndImage,
#if HAS_SIDE_FSENSOR() && HAS_EXTRUDER_FSENSOR()
    N_("Remove the filament from both filament sensors."),
#else
    N_("Remove the filament from the filament sensor."),
#endif
#if HAS_LARGE_DISPLAY()
    &img::hand_with_filament_150x130
#else
    std::nullopt
#endif
    >;

using FrameRemoveFilamentReady = WithConstructorArgs<FrameTextAndImage,
    N_("Filament removed, press continue."),
#if HAS_LARGE_DISPLAY()
    &img::hand_with_filament_ok_150x130
#else
    std::nullopt
#endif
    >;

#if HAS_SIDE_FSENSOR() && HAS_EXTRUDER_FSENSOR()
using FrameNotReadyConfirmContinue = WithConstructorArgs<FrameSpoolAndText, N_("There is a problem with some of the sensors. You may calibrate the rest, but the selftest will fail.")>;
#endif

using FrameSuccess = WithConstructorArgs<FrameSpoolAndText, N_("Filament sensors calibrated!")>;

using FrameFailed = WithConstructorArgs<FrameSpoolAndText, N_("Selftest FAILED. Enable logging to TXT from the settings for details.")>;

using Frames
    = FrameDefinitionList<ScreenSelftestFSensors::FrameStorage,
        FrameDefinition<Phase::prepare, FrameInit>,
#if PRINTER_IS_PRUSA_MINI()
        FrameDefinition<Phase::ask_mini_has_fsensor, FrameAskMiniHasFsensor>,
#endif
        FrameDefinition<Phase::not_connected, FrameNotConnected>,
        FrameDefinition<Phase::offer_unload, FrameOfferUnload>,
        FrameDefinition<Phase::ask_filament, FrameAskFilament>,
        FrameDefinition<Phase::calibrating, FrameCalibrating>,
        FrameDefinition<Phase::insert_filament_not_ready, FrameInsertFilamentNotReady>,
        FrameDefinition<Phase::insert_filament_ready, FrameInsertFilamentReady>,
        FrameDefinition<Phase::remove_filament_not_ready, FrameRemoveFilamentNotReady>,
        FrameDefinition<Phase::remove_filament_ready, FrameRemoveFilamentReady>,
#if HAS_SIDE_FSENSOR() && HAS_EXTRUDER_FSENSOR()
        FrameDefinition<Phase::not_ready_confirm_continue, FrameNotReadyConfirmContinue>,
#endif
        FrameDefinition<Phase::success, FrameSuccess>,
        FrameDefinition<Phase::failed, FrameFailed> //
        >;

} // namespace

ScreenSelftestFSensors::ScreenSelftestFSensors()
    : ScreenFSM(selftest_title(), GuiDefaults::RectScreenNoHeader) {
    header.SetIcon(&img::selftest_16x16);
    CaptureNormalWindow(inner_frame);
    create_frame();
}

ScreenSelftestFSensors::~ScreenSelftestFSensors() {
    destroy_frame();
    IFooterItem::set_tool_override(std::nullopt);
}

void ScreenSelftestFSensors::create_frame() {
    const auto phase = get_phase();
    const auto tool = PhysicalToolIndex::from_raw(fsm_base_data.GetData()[0]);
    Frames::create_frame(frame_storage, phase, &inner_frame, phase, tool);
}

void ScreenSelftestFSensors::destroy_frame() {
    Frames::destroy_frame(frame_storage, get_phase());
}

void ScreenSelftestFSensors::update_frame() {
    const auto tool = PhysicalToolIndex::from_raw(fsm_base_data.GetData()[0]);
    Frames::update_frame(frame_storage, get_phase(), fsm_base_data.GetData());
    IFooterItem::set_tool_override(tool);
}
