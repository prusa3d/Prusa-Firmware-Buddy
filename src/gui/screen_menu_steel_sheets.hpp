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

using sheet_index_0 = std::integral_constant<uint32_t, 0>;
using sheet_index_1 = std::integral_constant<uint32_t, 1>;
using sheet_index_2 = std::integral_constant<uint32_t, 2>;
using sheet_index_3 = std::integral_constant<uint32_t, 3>;
using sheet_index_4 = std::integral_constant<uint32_t, 4>;
using sheet_index_5 = std::integral_constant<uint32_t, 5>;
using sheet_index_6 = std::integral_constant<uint32_t, 6>;
using sheet_index_7 = std::integral_constant<uint32_t, 7>;

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

class MI_SHEET_SELECT : public WI_LABEL_t {
    static constexpr const char *const label = N_("Select");

public:
    MI_SHEET_SELECT();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_SHEET_CALIBRATE : public WI_LABEL_t {
    static constexpr const char *const label = N_("First Layer Calibration");

public:
    MI_SHEET_CALIBRATE();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_SHEET_RENAME : public WI_LABEL_t {
    static constexpr const char *const label = N_("Rename");

public:
    MI_SHEET_RENAME();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_SHEET_RESET : public WI_LABEL_t {
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

template <typename Index>
class SheetProfileMenuScreenT : public ISheetProfileMenuScreen {
public:
    using index_type = Index;
    SheetProfileMenuScreenT()
        : ISheetProfileMenuScreen(Index::value) {
    }
};

struct IProfileRecord : public WI_LABEL_t {
    void name_sheet(uint32_t value, char *buff);
    void click_index(uint32_t index);
    IProfileRecord();
};

template <typename Index>
struct ProfileRecord : public IProfileRecord {
    char name[MAX_SHEET_NAME_LENGTH + 1];
    ProfileRecord() {
        memset(name, 0, MAX_SHEET_NAME_LENGTH + 1);
        name_sheet(Index::value, name);

        // string_view_utf8::MakeRAM is safe. "name" is member var, exists until ProfileRecord is destroyed
        SetLabel(string_view_utf8::MakeRAM((const uint8_t *)name));
    };

protected:
    virtual void click([[maybe_unused]] IWindowMenu &window_menu) override {
        click_index(Index::value);
    }
};

using ScreenMenuSteelSheets__ = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN,
    ProfileRecord<sheet_index_0>, ProfileRecord<sheet_index_1>, ProfileRecord<sheet_index_2>, ProfileRecord<sheet_index_3>, ProfileRecord<sheet_index_4>, ProfileRecord<sheet_index_5>, ProfileRecord<sheet_index_6>, ProfileRecord<sheet_index_7>>;

class ScreenMenuSteelSheets : public ScreenMenuSteelSheets__ {
public:
    constexpr static const char *label = N_("Steel Sheets");
    ScreenMenuSteelSheets();
};
