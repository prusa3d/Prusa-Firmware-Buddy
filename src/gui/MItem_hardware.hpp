#pragma once
#include "WindowMenuItems.hpp"
#include "i18n.h"
#include <window_menu_enum_switch.hpp>
#include <config_store/store_instance.hpp>
#include <option/has_side_fsensor.h>
#include <option/has_toolchanger.h>
#include <common/extended_printer_type.hpp>

class MI_HARDWARE_CHECK : public WI_SWITCH_t<3> {
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
class MI_EXTENDED_PRINTER_TYPE : public WiStoreEnumSwitch<&config_store_ns::CurrentStore::extended_printer_type> {
public:
    MI_EXTENDED_PRINTER_TYPE();
    void OnChange(size_t diff) override;
};

#else
class MI_EXTENDED_PRINTER_TYPE : public IWiInfo {
public:
    MI_EXTENDED_PRINTER_TYPE()
        : IWiInfo(string_view_utf8::MakeCPUFLASH(PRINTER_MODEL), _("Printer Type")) {}
};

#endif
