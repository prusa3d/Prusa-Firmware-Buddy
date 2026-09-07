/// @file
#pragma once

#include <gui/screen/filament/screen_filament_detail.hpp>

#include <feature/openprinttag/tool_tag.hpp>

namespace buddy::openprinttag {

class ScreenOPTFilamentDetail : public ScreenFilamentDetail {

public:
    /// Used to open the screen in a read-only mode, just to display print parameters from an OpenPrintTag
    struct InfoParams {
        ToolTag tag;
    };

    struct PreheatModeParams {
        ToolTag::UIDHash uid_hash;
        VirtualToolIndex tool;
        PreheatMode mode;
    };

    /// See @p InfoParams
    ScreenOPTFilamentDetail(InfoParams params);

    /// Scans a tag for a given tool. Adds the "confirm" button that saves the filament into PendingAdHocFilamentType
    ScreenOPTFilamentDetail(PreheatModeParams params);

protected:
    void screenEvent(window_t *sender, GUI_event_t event, void *param) override;

private:
    /// Pops up a wait dialog and scans the tag for data.
    /// Then updates the data on the screen.
    /// Blocks till the scan finishes.
    /// Closes the screen on failure.
    /// @returns whether the scan was successful or not
    bool scan();

private:
    ToolTag tag_;

    /// If true, scan() will be called on the next loop event
    bool scan_pending_ : 1 = false;

    bool preheat_mode_ : 1 = false;
};

} // namespace buddy::openprinttag
