// screen_print_preview.hpp
#pragma once
#include "screen_print_preview_base.hpp"
#include "window_thumbnail.hpp"
#include "gcode_info.hpp"
#include "gcode_description.hpp"
#include "fs_event_autolock.hpp"
#include "static_alocation_ptr.hpp"
#include <common/fsm_base_types.hpp>
#include <guiconfig/guiconfig.h>
#include <option/has_toolchanger.h>
#include <option/has_mmu2.h>
#include <find_error.hpp>
#if HAS_TOOLCHANGER() || HAS_MMU2()
    #include "screen_tools_mapping.hpp"
#endif

// inherited from ScreenPrintPreviewBase just to handel different display sizes
// do not use AddSuperWindow<ScreenPrintPreviewBase>
class ScreenPrintPreview : public ScreenPrintPreviewBase {
    // Apart from FIle error they all have the same warning
    constexpr static const char *label_unfinished_selftest = find_error(ErrCode::CONNECT_PRINT_PREVIEW_UNFINISHED_SELFTEST).err_title;
    constexpr static const char *label_fil_not_detected = find_error(ErrCode::CONNECT_PRINT_PREVIEW_NO_FILAMENT).err_title;
#if HAS_MMU2()
    constexpr static const char *label_fil_detected_mmu = find_error(ErrCode::CONNECT_PRINT_PREVIEW_MMU_FILAMENT_INSERTED).err_title;
#endif
    constexpr static const char *label_file_error = find_error(ErrCode::CONNECT_PRINT_PREVIEW_FILE_ERROR).err_title;
    constexpr static const char *label_wrong_printer = find_error(ErrCode::CONNECT_PRINT_PREVIEW_WRONG_PRINTER).err_title;
    constexpr static const char *label_wrong_filament = find_error(ErrCode::CONNECT_PRINT_PREVIEW_WRONG_FILAMENT).err_title;

    static constexpr const char *txt_unfinished_selftest = find_error(ErrCode::CONNECT_PRINT_PREVIEW_UNFINISHED_SELFTEST).err_text;
    static constexpr const char *txt_fil_not_detected = find_error(ErrCode::CONNECT_PRINT_PREVIEW_NO_FILAMENT).err_text;
#if HAS_MMU2()
    static constexpr const char *txt_fil_detected_mmu = find_error(ErrCode::CONNECT_PRINT_PREVIEW_MMU_FILAMENT_INSERTED).err_text;
#endif

    static constexpr const char *txt_new_fw_available = N_(find_error(ErrCode::CONNECT_PRINT_PREVIEW_NEW_FW).err_text);
    static constexpr const char *txt_wrong_fil_type = find_error(ErrCode::CONNECT_PRINT_PREVIEW_WRONG_FILAMENT).err_text;

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
