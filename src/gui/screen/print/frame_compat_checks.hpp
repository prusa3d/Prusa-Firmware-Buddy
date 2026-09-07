#pragma once

#include <feature/compatibility_checks/gcode_compatibility.hpp>
#include <inplace_vector.hpp>
#include <window_frame.hpp>
#include <fsm/print_preview_phases.hpp>
#include <window_menu_virtual.hpp>
#include <dynamic_index_mapping.hpp>
#include <window_menu_adv.hpp>
#include <radio_button_fsm.hpp>
#include <window_text.hpp>

namespace screen_print_preview {

class WindowMenuCompatibilityChecks final : public WindowMenuVirtual {
public:
    WindowMenuCompatibilityChecks(window_t *parent, Rect16 rect);

    struct GCodeMode {};
    struct FilamentMode {
        FilamentType filament;
        VirtualToolIndex tool;
        bool assume_filament_already_inserted : 1;
    };

    using Mode = std::variant<GCodeMode, FilamentMode>;

    void setup(Mode mode);

public:
    int item_count() const final;

protected:
    void setup_item(ItemVariant &variant, int index) final;

private:
    using CheckMetadata = buddy::gcode_compatibility::CheckMetadata;

    // Note: this frame only shows tool-specific incompabilities for single-tool printers
    // So we don't need to store what tool the fails relate to
    // Ambiguous tool-specific fails should be handled by the tool mapping screen
    stdext::inplace_vector<const CheckMetadata *, 16> failed_checks_;
};

class FrameCompatibilityChecks {

public:
    using GCodeMode = WindowMenuCompatibilityChecks::GCodeMode;
    using FilamentMode = WindowMenuCompatibilityChecks::FilamentMode;
    using Mode = WindowMenuCompatibilityChecks::Mode;

    FrameCompatibilityChecks(window_frame_t *parent, FSMAndPhase phase);

    void setup(Mode mode);

    void update(const fsm::PhaseData &data);

private:
    window_text_t title_;
    BasicWindow title_line_;
    WindowExtendedMenu<WindowMenuCompatibilityChecks> menu_;
    RadioButtonFSM radio_;

    const FSMAndPhase phase_;
};

} // namespace screen_print_preview
