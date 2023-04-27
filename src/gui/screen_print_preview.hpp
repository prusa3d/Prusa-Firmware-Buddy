// screen_print_preview.hpp
#pragma once
#include "screen_print_preview_base.hpp"
#include "window_thumbnail.hpp"
#include "gcode_info.hpp"
#include "gcode_description.hpp"
#include "fs_event_autolock.hpp"
#include "static_alocation_ptr.hpp"
#include "fsm_base_types.hpp"

// inherited from ScreenPrintPreviewBase just to handel different display sizes
// do not use AddSuperWindow<ScreenPrintPreviewBase>
class ScreenPrintPreview : public ScreenPrintPreviewBase {
    constexpr static const char *labelWarning = N_("Warning");

    static constexpr const char *txt_fil_not_detected = N_("Filament not detected. Load filament now?\nSelect NO to cancel the print.\nSelect DISABLE FS to disable the filament sensor and continue print.");
    static constexpr const char *txt_fil_detected_mmu = N_("Filament detected. Unload filament now? Select NO to cancel.");

    static ScreenPrintPreview *ths; // to be accessible in dialog handler

    GCodeInfo &gcode;
    GCodeInfoWithDescription gcode_description; // cannot be first
    WindowPreviewThumbnail thumbnail;           // draws preview png

    PhasesPrintPreview phase;

    using UniquePtr = static_unique_ptr<AddSuperWindow<MsgBoxIconned>>;
    UniquePtr pMsgbox;

    using MsgBoxMemSpace = std::aligned_union<0, MsgBoxTitled>::type;
    MsgBoxMemSpace msgBoxMemSpace;

    UniquePtr makeMsgBox(string_view_utf8 caption, string_view_utf8 text);

public:
    ScreenPrintPreview();
    virtual ~ScreenPrintPreview() override;
    static ScreenPrintPreview *GetInstance();
    void Change(fsm::BaseData data);

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;

private:
    void on_enter();

    bool event_in_progress { false };
    bool first_event { true };
};
