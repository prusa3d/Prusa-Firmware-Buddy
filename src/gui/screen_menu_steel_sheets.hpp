/**
 * @file screen_menu_steel_sheets.hpp
 */

#pragma once

#include "screen_menu.hpp"
#include "WindowItemFormatableLabel.hpp"

// TODO there is nowhere displayed currently selected sheet
// entire menu is confusing

enum class profile_action : uint32_t {
    Select = 1,
    Calibrate = 2,
    Reset = 3,
    Rename = 4
};

class MI_SHEET_OFFSET : public WI_LAMBDA_LABEL_t {
    static constexpr const char *const label = N_("Offset");
    float offset = 0;
    bool calib = false;
    static constexpr char const *const notCalibrated = N_("Not Calib");
    void printExtension(Rect16 extension_rect, color_t color_text, color_t color_back, ropfn raster_op) const override;

public:
    MI_SHEET_OFFSET();

    void SetOffset(float offs);
    void Reset();
};

class MI_SHEET_SELECT : public IWindowMenuItem {
    static constexpr const char *const label = N_("Select");

public:
    MI_SHEET_SELECT();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_SHEET_CALIBRATE : public IWindowMenuItem {
    static constexpr const char *const label = N_("First Layer Calibration");

public:
    MI_SHEET_CALIBRATE();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_SHEET_RENAME : public IWindowMenuItem {
    static constexpr const char *const label = N_("Rename");

public:
    MI_SHEET_RENAME();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_SHEET_RESET : public IWindowMenuItem {
    static constexpr const char *const label = N_("Reset");

public:
    MI_SHEET_RESET();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

using SheetProfileMenuScreen__ = ScreenMenu<EFooter::On, MI_RETURN, MI_SHEET_SELECT, MI_SHEET_CALIBRATE,
#if _DEBUG // todo remove #if _DEBUG after rename is finished
    MI_SHEET_RENAME,
#endif // _DEBUG
    MI_SHEET_RESET, MI_SHEET_OFFSET>;

// TODO there is no way to tell which sheet I am currently calibrating
class ISheetProfileMenuScreen : public SheetProfileMenuScreen__ {
    uint32_t value;

public:
    constexpr static const char *label = N_("Sheet Profile");
    ISheetProfileMenuScreen(uint32_t value);

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t ev, void *param) override;
};

template <uint8_t sheet_index>
class SheetProfileMenuScreenT : public ISheetProfileMenuScreen {
public:
    SheetProfileMenuScreenT()
        : ISheetProfileMenuScreen(sheet_index) {
    }
};

class I_MI_SHEET_PROFILE : public IWindowMenuItem {
protected:
    I_MI_SHEET_PROFILE(int sheet_index);

    void click(IWindowMenu &window_menu) override;

private:
    std::array<char, MAX_SHEET_NAME_LENGTH + 1> label_str;

    uint8_t sheet_index;
};

template <uint8_t sheet_index_>
class MI_SHEET_PROFILE : public I_MI_SHEET_PROFILE {
public:
    MI_SHEET_PROFILE()
        : I_MI_SHEET_PROFILE(sheet_index_) {}
};

using ScreenMenuSteelSheets__ = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN,
    MI_SHEET_PROFILE<0>, MI_SHEET_PROFILE<1>, MI_SHEET_PROFILE<2>, MI_SHEET_PROFILE<3>, MI_SHEET_PROFILE<4>, MI_SHEET_PROFILE<5>, MI_SHEET_PROFILE<6>, MI_SHEET_PROFILE<7>>;

class ScreenMenuSteelSheets : public ScreenMenuSteelSheets__ {
public:
    constexpr static const char *label = N_("Steel Sheets");
    ScreenMenuSteelSheets();
};
