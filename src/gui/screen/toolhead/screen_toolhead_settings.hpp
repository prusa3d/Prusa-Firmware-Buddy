#pragma once

#include <variant>
#include <cstdint>

#include "screen_toolhead_settings_common.hpp"

#include <config_store/store_definition.hpp>
#include <option/has_mmu2.h>
#include <gui/menu_item/menu_item_select_menu.hpp>

#if HAS_MMU2()
    #include <MItem_mmu.hpp>
#endif

namespace screen_toolhead_settings {

class MI_NOZZLE_DIAMETER final : public MI_TOOLHEAD_SPECIFIC_SPIN<MI_NOZZLE_DIAMETER> {
public:
    MI_NOZZLE_DIAMETER(Toolhead toolhead = default_toolhead);
    static float read_value_impl(ToolheadIndex ix);
    static void store_value_impl(ToolheadIndex ix, float set);
};

class MI_NOZZLE_DIAMETER_HELP : public IWindowMenuItem {
public:
    MI_NOZZLE_DIAMETER_HELP();
    void click(IWindowMenu &menu) override;
};

#if HAS_HOTEND_TYPE_SUPPORT()
class MI_HOTEND_TYPE : public MI_TOOLHEAD_SPECIFIC<MI_HOTEND_TYPE, MenuItemSelectMenu> {
public:
    MI_HOTEND_TYPE(Toolhead toolhead = default_toolhead);

    int item_count() const final;
    void build_item_text(int index, const std::span<char> &buffer) const final;

protected:
    bool on_item_selected(int old_index, int new_index) override;
};

class MI_NOZZLE_SOCK : public MI_TOOLHEAD_SPECIFIC_TOGGLE<MI_NOZZLE_SOCK> {
public:
    MI_NOZZLE_SOCK(Toolhead toolhead = default_toolhead);

    static bool read_value_impl(ToolheadIndex ix);
    static void store_value_impl(ToolheadIndex ix, bool set);
};

using MI_HOTEND_SOCK_OR_TYPE = std::conditional_t<hotend_type_only_sock, MI_NOZZLE_SOCK, MI_HOTEND_TYPE>;
#endif

class MI_NOZZLE_HARDENED : public MI_TOOLHEAD_SPECIFIC_TOGGLE<MI_NOZZLE_HARDENED> {
public:
    MI_NOZZLE_HARDENED(Toolhead toolhead = default_toolhead);
    static bool read_value_impl(ToolheadIndex ix);
    static void store_value_impl(ToolheadIndex ix, bool set);
};

class MI_NOZZLE_HIGH_FLOW : public MI_TOOLHEAD_SPECIFIC_TOGGLE<MI_NOZZLE_HIGH_FLOW> {
public:
    MI_NOZZLE_HIGH_FLOW(Toolhead toolhead = default_toolhead);
    static bool read_value_impl(ToolheadIndex ix);
    static void store_value_impl(ToolheadIndex ix, bool set);
};

#if HAS_TOOLCHANGER()
class MI_DOCK : public MI_TOOLHEAD_SPECIFIC<MI_DOCK, IWindowMenuItem> {
public:
    MI_DOCK(Toolhead toolhead = default_toolhead);
    void click(IWindowMenu &) override;
};

class MI_PICK_PARK : public MI_TOOLHEAD_SPECIFIC<MI_PICK_PARK, IWindowMenuItem> {
public:
    MI_PICK_PARK(Toolhead toolhead = default_toolhead);
    void update(bool update_value = true);
    void click(IWindowMenu &) override;

private:
    bool is_picked = false;
};
#endif

class MI_FILAMENT_SENSORS : public MI_TOOLHEAD_SPECIFIC<MI_FILAMENT_SENSORS, IWindowMenuItem> {
public:
    MI_FILAMENT_SENSORS(Toolhead toolhead = default_toolhead);
    void click(IWindowMenu &) override;
};

#if FILAMENT_SENSOR_IS_ADC()
class MI_CALIBRATE_FILAMENT_SENSORS : public MI_TOOLHEAD_SPECIFIC<MI_CALIBRATE_FILAMENT_SENSORS, IWindowMenuItem> {
public:
    MI_CALIBRATE_FILAMENT_SENSORS(Toolhead toolhead = default_toolhead);
    void update();
    void click(IWindowMenu &) override;
};
#endif

class MI_NOZZLE_OFFSET : public MI_TOOLHEAD_SPECIFIC<MI_NOZZLE_OFFSET, IWindowMenuItem> {
public:
    MI_NOZZLE_OFFSET(Toolhead toolhead = default_toolhead);
    void click(IWindowMenu &) override;
};

using ScreenToolheadDetail_ = ScreenMenu<EFooter::Off,
    MI_RETURN,
    MI_NOZZLE_DIAMETER,
#if PRINTER_IS_PRUSA_XL()
    // Prusa XL was sold with .6mm nozzles and then with .4mm nozzles, so the users need to set in the FW what nozzles they have
    // This is to help them out a bit
    MI_NOZZLE_DIAMETER_HELP,
#endif
    MI_NOZZLE_HARDENED,
    MI_NOZZLE_HIGH_FLOW,
#if HAS_HOTEND_TYPE_SUPPORT()
    MI_HOTEND_SOCK_OR_TYPE,
#endif
#if HAS_MMU2()
    MI_MMU_NEXTRUDER_REWORK,
    MI_DONE_EXTRUDER_MAINTENANCE, // both for loadcell equipped printers and MK3.5
#endif
#if HAS_TOOLCHANGER()
    MI_PICK_PARK,
    MI_NOZZLE_OFFSET,
    MI_DOCK,
#endif
#if FILAMENT_SENSOR_IS_ADC()
    MI_CALIBRATE_FILAMENT_SENSORS,
#endif
    MI_FILAMENT_SENSORS //
    >;

class ScreenToolheadDetail : public ScreenToolheadDetail_ {
public:
    ScreenToolheadDetail(Toolhead toolhead = default_toolhead);

private:
    const Toolhead toolhead;
    StringViewUtf8Parameters<2> title_params;
};

class MI_TOOLHEAD : public IWindowMenuItem {
public:
    MI_TOOLHEAD(Toolhead toolhead);

protected:
    void click(IWindowMenu &) final;

private:
    const Toolhead toolhead;
    StringViewUtf8Parameters<2> label_params;
};

#if HAS_TOOLCHANGER()
template <typename>
struct ScreenToolheadSettingsList_;

template <size_t... i>
struct ScreenToolheadSettingsList_<std::index_sequence<i...>> {
    using T = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN, WithConstructorArgs<MI_TOOLHEAD, ToolheadIndex(i)>..., WithConstructorArgs<MI_TOOLHEAD, AllToolheads {}>>;
};

class ScreenToolheadSettingsList : public ScreenToolheadSettingsList_<std::make_index_sequence<toolhead_count>>::T {
public:
    ScreenToolheadSettingsList();
};
#endif

} // namespace screen_toolhead_settings

using ScreenToolheadDetail = screen_toolhead_settings::ScreenToolheadDetail;

#if HAS_TOOLCHANGER()
using ScreenToolheadSettingsList = screen_toolhead_settings::ScreenToolheadSettingsList;
#endif
