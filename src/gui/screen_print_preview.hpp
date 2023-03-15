// screen_print_preview.hpp
#pragma once
#include "gui.hpp"
#include "window_header.hpp"
#include "window_roll_text.hpp"
#include "window_thumbnail.hpp"
#include "screen.hpp"
#include "display.h"
#include "printers.h"
#include "gcode_info.hpp"
#include "gcode_description.hpp"
#include "fs_event_autolock.hpp"
#include "static_alocation_ptr.hpp"
#include "fsm_base_types.hpp"
#include "radio_button_fsm.hpp"

class ScreenPrintPreview : public AddSuperWindow<screen_t> {
    constexpr static const char *labelWarning = N_("Warning");

    static constexpr const char *txt_fil_not_detected = N_("Filament not detected. Load filament now?\nSelect NO to cancel the print.\nSelect DISABLE FS to disable the filament sensor and continue print.");
    static constexpr const char *txt_fil_detected_mmu = N_("Filament detected. Unload filament now? Select NO to cancel.");
    static constexpr const char *txt_wrong_fil_type =
#if PRINTER_TYPE == PRINTER_PRUSA_XL
        N_("A filament specified in the G-code is either not loaded or wrong type.");
#else
        N_("This G-code was set up for another filament type.");
#endif

    static ScreenPrintPreview *ths; // to be accessible in dialog handler

#ifdef USE_ILI9488
    window_header_t header;
#endif // USE_ILI9488

    window_roll_text_t title_text;
    RadioButtonFsm<PhasesPrintPreview> radio; // shows 2 mutually exclusive buttons Print and Back

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
