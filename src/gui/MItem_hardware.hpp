#pragma once
#include "WindowMenuItems.hpp"
#include "i18n.h"
#include <config_store/store_instance.hpp>
#include <option/has_side_fsensor.h>
#include <option/has_toolchanger.h>
#include <option/has_emergency_stop.h>
#include <common/extended_printer_type.hpp>
#include <gui/menu_item/menu_item_select_menu.hpp>

class MI_HARDWARE_CHECK : public MenuItemSwitch {
public:
    MI_HARDWARE_CHECK(HWCheckType check_type);

protected:
    void OnChange([[maybe_unused]] size_t old_index) final;

private:
    const HWCheckType check_type;
};

class MI_HARDWARE_G_CODE_CHECKS : public IWindowMenuItem {
    static constexpr const char *const label = N_("G-Code Checks");

public:
    MI_HARDWARE_G_CODE_CHECKS();

protected:
    virtual void click(IWindowMenu &windowMenu) override;
};

#if HAS_TOOLCHANGER() && HAS_SIDE_FSENSOR()
class MI_SIDE_FSENSOR_REMAP : public WI_ICON_SWITCH_OFF_ON_t {
    static constexpr const char *const label = N_("Side FSensor Remap");

public:
    MI_SIDE_FSENSOR_REMAP();

protected:
    virtual void OnChange(size_t old_index) override;
};
#endif /*HAS_TOOLCHANGER() && HAS_SIDE_FSENSOR()*/

#if HAS_EXTENDED_PRINTER_TYPE()
class MI_EXTENDED_PRINTER_TYPE : public MenuItemSelectMenu {
public:
    MI_EXTENDED_PRINTER_TYPE();

    int item_count() const final;
    void build_item_text(int index, const std::span<char> &buffer) const final;

protected:
    bool on_item_selected(int old_index, int new_index) override;
};

#else
class MI_EXTENDED_PRINTER_TYPE : public IWiInfo {
public:
    MI_EXTENDED_PRINTER_TYPE()
        : IWiInfo(string_view_utf8::MakeCPUFLASH(PrinterModelInfo::current().id_str), _("Printer Type")) {}
};

#endif

#if HAS_EMERGENCY_STOP()
class MI_EMERGENCY_STOP_ENABLE : public WI_ICON_SWITCH_OFF_ON_t {
    static constexpr const char *const label = N_("Door Sensor");

public:
    MI_EMERGENCY_STOP_ENABLE();

protected:
    virtual void OnChange(size_t old_index) override;
};
#endif
