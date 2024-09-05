#include "screen_belt_tuning_wizard.hpp"

#include <span>

#include <img_resources.hpp>
#include <meta_utils.hpp>
#include <gui/auto_layout.hpp>
#include <display.hpp>

#include <gui/standard_frame/frame_prompt.hpp>
#include <gui/standard_frame/frame_qr_prompt.hpp>
#include <feature/belt_tuning/belt_tuning_wizard.hpp>
#include <feature/belt_tuning/printer_belt_parameters.hpp>

using Phase = PhaseBeltTuning;

namespace {

using FrameAskForGantryAlign = WithConstructorArgs<FrameQRPrompt, Phase::ask_for_gantry_align, N_("Please follow the XY gantry alignment process in the manual."), "belt-tuning"_tstr>;
using FramePreparing = WithConstructorArgs<FramePrompt, Phase::preparing, N_("Preparing"), N_("Setting the printer up for the calibration.\n\nPlease wait.")>;

// TODO: Maybe add progress bar?
using FrameCalibratingAccelerometer = WithConstructorArgs<FramePrompt, Phase::calibrating_accelerometer, N_("Calibrating accelerometer"), N_("Please wait.")>;

using FrameAskForDampenersInstallation = WithConstructorArgs<FramePrompt, Phase::ask_for_dampeners_installation, N_("Install belt dampeners"), N_("Please install the belt dampeners to the left and right side of the printer.")>;
using FrameAskForDampenersUninstallation = WithConstructorArgs<FramePrompt, Phase::ask_for_dampeners_uninstallation, N_("Remove belt dampeners"), N_("Please remove the belt dampeners.")>;
using FrameError = WithConstructorArgs<FramePrompt, Phase::error, N_("Error"), N_("An error occurred during the belt tuning procedure.\nTry running the calibration again.")>;

// This is a bit of a hack - FrameDefitinList::create_frame passes the same arguments to all frame types
// We however need to pass the screen pointer to some of our frames,
// but at the same time we need to keep the parameter compatbility with the standard ones.
// So I've created this class that holds the screen data, but implicitly casts to window_t* parent pointer that is expected by the standard frames
struct FrameParent {
    window_t *parent;
    ScreenBeltTuningWizard *screen;

    inline operator window_t *() const {
        return parent;
    }
};

class WindowGraphView final : public window_t {

public:
    WindowGraphView(window_t *parent)
        : window_t(parent, {}) {}

    void set_data(const std::span<uint8_t> &data) {
        data_ = data;
        Invalidate();
    }

protected:
    void unconditionalDraw() override {
        const Rect16 rect = GetRect();

        // Fill background
        display::fill_rect(rect, GetBackColor());

        uint16_t x = rect.Left();

        // Draw data where we know them
        if (!data_.empty()) {
            uint8_t prev_val = data_[0];
            for (const uint8_t val : data_) {
                display::draw_line(point_ui16_t { x, static_cast<uint16_t>(rect.Bottom() - prev_val) }, point_ui16_t { x, static_cast<uint16_t>(rect.Bottom() - val) }, COLOR_WHITE);
                prev_val = val;
                x++;
            }
        }

        if (x < rect.Right()) {
            // Draw a line indicating the remaining size of the graph
            display::draw_line(point_ui16_t { static_cast<uint16_t>(x), static_cast<uint16_t>(rect.Bottom()) }, point_ui16_t { static_cast<uint16_t>(rect.Right()), static_cast<uint16_t>(rect.Bottom()) }, COLOR_DARK_GRAY);
        }
    }

private:
    /// Height of the column (in pixels) for each X coordinate of the graph
    std::span<uint8_t> data_;
};

class FrameMeasuring : public FramePrompt {

public:
    FrameMeasuring(FrameParent parent)
        : FramePrompt(parent, Phase::measuring, N_("Measuring belt tension"), nullptr)
        , progress_bar(this, {}, COLOR_ORANGE)
        , graph(this)
        , screen(*parent.screen) //
    {
        static constexpr std::initializer_list layout {
            // Title
            StackLayoutItem { .height = 64 },

            // Progress bar
            standard_stack_layout::for_progress_bar,

            // Info
            StackLayoutItem { .height = StackLayoutItem::stretch, .margin_side = 16 },

            // Graph
            StackLayoutItem { .height = ScreenBeltTuningWizard::graph_height, .width = ScreenBeltTuningWizard::graph_width, .margin_bottom = 32 },

            // Radio
            standard_stack_layout::for_radio,
        };
        layout_vertical_stack(GetRect(), { &title, &progress_bar, &info, &graph, &radio }, layout);

        // Reset the graph data, we will be sequentially filling it during the measuring
        screen.graph_data.fill(0);
    }

    void update(const fsm::PhaseData &data_) {
        const auto data = fsm::deserialize_data<BeltTuninigWizardMeasuringData>(data_);
        progress_bar.SetProgressPercent(data.progress_0_255 / 255.0f * 100.0f);

        if (data.frequency) {
            info.SetText(string_view_utf8::MakeCPUFLASH("%d Hz").formatted(info_params, (int)data.frequency));
            info.Invalidate();

            // Update the graph data
            {
                size_t start = std::max<size_t>(graph_data_size, 1) - 1;
                const size_t end = std::clamp<size_t>(std::ceil(data.progress_0_255 / 255.0f * ScreenBeltTuningWizard::graph_width), 0, ScreenBeltTuningWizard::graph_width);

                const float end_val = data.last_amplitude_percent / 100.0f * ScreenBeltTuningWizard::graph_height;
                const float start_val = graph_data_size ? screen.graph_data[start] : end_val;

                for (size_t i = start; i < end; i++) {
                    screen.graph_data[i] = std::max(screen.graph_data[i], std::clamp<uint8_t>(start_val + (end_val - start_val) * (i - start) / (end - start), 0, ScreenBeltTuningWizard::graph_height - 1));
                }

                graph_data_size = end;
                graph.set_data(std::span(screen.graph_data.data(), graph_data_size));
            }
        }
    }

private:
    window_numberless_progress_t progress_bar;
    WindowGraphView graph;

private:
    StringViewUtf8Parameters<4> info_params;
    ScreenBeltTuningWizard &screen;
    size_t graph_data_size = 0;
};

class FrameResults : public FramePrompt {

public:
    FrameResults(FrameParent parent)
        : FramePrompt(parent, Phase::results, N_("Your belts are tensioned to:"), nullptr)
        , graph(this)
        , screen(*parent.screen) //
    {
        static constexpr std::initializer_list layout {
            // Title
            StackLayoutItem { .height = 48 },

            // Info
            StackLayoutItem { .height = StackLayoutItem::stretch, .margin_side = 16, .margin_top = 16 },

            // Graph
            StackLayoutItem { .height = ScreenBeltTuningWizard::graph_height, .width = ScreenBeltTuningWizard::graph_width, .margin_bottom = 16 },

            // Radio
            standard_stack_layout::for_radio,
        };
        layout_vertical_stack(GetRect(), { &title, &info, &graph, &radio }, layout);
    }

    void update(const fsm::PhaseData &serialized_data) {
        const auto data = fsm::deserialize_data<BeltTuningWizardResultsData>(serialized_data);
        const auto &params = printer_belt_parameters.belt_system[0];
        const float tension = static_cast<float>(data.tension) / BeltTuningWizardResultsData::tension_mult;

        std::array<char, 16> target_str;
        _("Target").copyToRAM(target_str);
        info.SetText(string_view_utf8::MakeCPUFLASH("%.1f N (%i Hz)\n\n%s: %.1f +- %.1f N").formatted(info_params, tension, (int)data.frequency, target_str.data(), params.target_tension_force_n, params.target_tension_force_dev_n));
        info.Invalidate(); // Annoying reference comparison in SetText

        graph.set_data(screen.graph_data);
    }

private:
    WindowGraphView graph;

private:
    StringViewUtf8Parameters<32> info_params;
    ScreenBeltTuningWizard &screen;
};

using Frames = FrameDefinitionList<ScreenBeltTuningWizard::FrameStorage,
    FrameDefinition<Phase::ask_for_gantry_align, FrameAskForGantryAlign>,
    FrameDefinition<Phase::preparing, FramePreparing>,
    FrameDefinition<Phase::ask_for_dampeners_installation, FrameAskForDampenersInstallation>,
    FrameDefinition<Phase::calibrating_accelerometer, FrameCalibratingAccelerometer>,
    FrameDefinition<Phase::measuring, FrameMeasuring>,
    FrameDefinition<Phase::results, FrameResults>,
    FrameDefinition<Phase::ask_for_dampeners_uninstallation, FrameAskForDampenersUninstallation>,
    FrameDefinition<Phase::error, FrameError> //
    >;

} // namespace

ScreenBeltTuningWizard::ScreenBeltTuningWizard()
    : ScreenFSM(N_("BELT TUNING"), GuiDefaults::RectScreenNoHeader) //
{
    header.SetIcon(&img::wizard_16x16);
    CaptureNormalWindow(inner_frame);
    create_frame();
}

ScreenBeltTuningWizard::~ScreenBeltTuningWizard() {
    destroy_frame();
}

void ScreenBeltTuningWizard::create_frame() {
    Frames::create_frame(frame_storage, get_phase(), FrameParent { &inner_frame, this });
}
void ScreenBeltTuningWizard::destroy_frame() {
    Frames::destroy_frame(frame_storage, get_phase());
}
void ScreenBeltTuningWizard::update_frame() {
    Frames::update_frame(frame_storage, get_phase(), fsm_base_data.GetData());
}
