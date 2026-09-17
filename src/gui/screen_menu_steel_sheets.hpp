#pragma once

#include "screen_menu.hpp"
#include "WindowItemFormatableLabel.hpp"
#include <config_store/constants.hpp>
#include <gui/menu_item/menu_item_virtual_submenu.hpp>
#include <guiconfig/GuiDefaults.hpp>

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
    void printExtension(Rect16 extension_rect, Color color_text, Color color_back, ropfn raster_op) const override;

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
    MI_SHEET_RENAME,
    MI_SHEET_RESET, MI_SHEET_OFFSET>;

// TODO there is no way to tell which sheet I am currently calibrating
class SheetProfileMenuScreen : public SheetProfileMenuScreen__ {
    uint32_t value;

public:
    SheetProfileMenuScreen(uint32_t value);

protected:
    void update_title();

    virtual void windowEvent(window_t *sender, GUI_event_t ev, void *param) override;

private:
    /// Holds string "Sheet: (NAME)"
    std::array<char, 32> label_buffer;
};

class MI_SHEET_PROFILE : public IWindowMenuItem {
public:
    MI_SHEET_PROFILE(uint8_t sheet_index);

protected:
    void click(IWindowMenu &window_menu) override;

private:
    std::array<char, SHEET_NAME_BUFFER_SIZE> label_str;

    uint8_t sheet_index;
};

using MI_STEEL_SHEETS = MenuItemVirtualSubmenu<N_("Steel Sheets"), MI_SHEET_PROFILE, config_store_ns::sheets_num>;
