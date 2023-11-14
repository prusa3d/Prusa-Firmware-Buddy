// screen_print_preview.hpp
#pragma once
#include "screen_print_preview_base.hpp"
#include "window_thumbnail.hpp"
#include "gcode_info.hpp"
#include "gcode_description.hpp"
#include "fs_event_autolock.hpp"
#include "static_alocation_ptr.hpp"
#include "fsm_base_types.hpp"
#include <option/has_toolchanger.h>
#include <option/has_mmu2.h>
#if HAS_TOOLCHANGER() || HAS_MMU2()
    #include "screen_tools_mapping.hpp"
#endif

// inherited from ScreenPrintPreviewBase just to handel different display sizes
// do not use AddSuperWindow<ScreenPrintPreviewBase>
class ScreenPrintPreview : public ScreenPrintPreviewBase {
    constexpr static const char *labelWarning = N_("Warning");

    static constexpr const char *txt_unfinished_selftest = N_("Please complete Calibrations & Tests before using the printer.");
    static constexpr const char *txt_fil_not_detected = N_("Filament not detected. Load filament now?\nSelect NO to cancel the print.\nSelect DISABLE FS to disable the filament sensor and continue print.");
    static constexpr const char *txt_fil_detected_mmu = N_("Filament detected. Unload filament now? Select NO to cancel.");

#if defined(USE_ST7789)
    static constexpr const char *txt_new_fw_available = N_("New FW available");
#else
    static constexpr const char *txt_new_fw_available = N_("New firmware available");
#endif

    static ScreenPrintPreview *ths; // to be accessible in dialog handler

    GCodeInfo &gcode;
    GCodeInfoWithDescription gcode_description; // cannot be first
    WindowPreviewThumbnail thumbnail; // draws preview image

    // Set to invalid value by default so that the Change() always triggers on the first call.
    PhasesPrintPreview phase = static_cast<PhasesPrintPreview>(-1);

    using UniquePtrBox = static_unique_ptr<AddSuperWindow<MsgBoxIconned>>;
    UniquePtrBox pMsgbox;

#if HAS_TOOLCHANGER() || HAS_MMU2()
    using UniquePtrMapping = static_unique_ptr<ToolsMappingBody>;
    UniquePtrMapping tools_mapping;

    using MsgBoxMemSpace = std::aligned_union<0, MsgBoxTitled, ToolsMappingBody>::type;
#else
    using MsgBoxMemSpace = std::aligned_union<0, MsgBoxTitled>::type;
#endif
    MsgBoxMemSpace msgBoxMemSpace;

public:
    ScreenPrintPreview();
    virtual ~ScreenPrintPreview() override;
    static ScreenPrintPreview *GetInstance();
    void Change(fsm::BaseData data);

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;

private:
    void hide_main_dialog();
    void show_main_dialog();
    void show_tools_mapping();
};
