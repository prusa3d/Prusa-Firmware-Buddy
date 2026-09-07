#pragma once
#include <array>
#include <utility>

#include "WindowMenuItems.hpp"
#include "i18n.h"
#include <config_store/store_instance.hpp>
#include <option/has_side_fsensor_remap.h>
#include <option/has_toolchanger.h>
#include <option/has_emergency_stop.h>
#include <option/has_chamber_vents.h>
#include <option/has_expansion_joints_gen_2.h>
#include <option/has_nozzle_cleaner_lite.h>
#include <option/has_15gt_belts.h>
#include <option/has_switchable_homing_calibration.h>
#include <common/extended_printer_type.hpp>
#include <common/printer_variant/printer_variant.hpp>
#include <dynamic_index_mapping.hpp>
#include <gui/menu_item/menu_item_select_menu.hpp>

class MI_HARDWARE_CHECK : public MenuItemSwitch {
public:
    MI_HARDWARE_CHECK(HWCheckType check_type);

protected:
    void OnChange([[maybe_unused]] size_t old_index) final;

private:
    const HWCheckType check_type;
};

#if HAS_SIDE_FSENSOR_REMAP()
class MI_SIDE_FSENSOR_REMAP : public WI_ICON_SWITCH_OFF_ON_t {
    static constexpr const char *const label = N_("Side FSensor Remap");

public:
    MI_SIDE_FSENSOR_REMAP();

protected:
    virtual void OnChange(size_t old_index) override;
};
#endif

#if IS_EXTENDED_PRINTER_TYPE_CONFIGURABLE()
class MI_EXTENDED_PRINTER_TYPE : public MenuItemSelectMenu {
public:
    MI_EXTENDED_PRINTER_TYPE();

    int item_count() const final;
    string_view_utf8 build_item_text(int index, ItemTextParams &params) const final;

protected:
    bool on_item_selected(const OnItemSelectedArgs &args) override;
};

#else
// Display-only: either there is just a single model variant, or, on XL, the
// type is auto-detected at boot (XL-CAN probe).
class MI_EXTENDED_PRINTER_TYPE : public IWiInfo {
public:
    MI_EXTENDED_PRINTER_TYPE()
        : IWiInfo(string_view_utf8::MakeCPUFLASH(PrinterModelInfo::current().display_str()), _("Printer Type")) {}
};

#endif

#if HAS_PRINTER_VARIANT()
/// Printer feature edition. Not persisted - selecting an edition prepicks the feature-flag defaults.
/// The displayed "current" is derived from the flags; when they match no edition, a "Custom" row is shown.
class MI_PRINTER_VARIANT : public MenuItemSelectMenu {
public:
    MI_PRINTER_VARIANT();

    int item_count() const final;
    string_view_utf8 build_item_text(int index, ItemTextParams &params) const final;

protected:
    bool on_item_selected(const OnItemSelectedArgs &args) override;

    /// A sibling hardware toggle can change the flags this edition is derived from; keep it in sync.
    void Loop() override;

private:
    enum class Item {
        variant,
        /// Display-only row, present only when the current flags match no edition.
        custom,
    };

    static constexpr auto items = std::to_array<DynamicIndexMappingRecord<Item>>({
        { Item::variant, DynamicIndexMappingType::static_section, std::to_underlying(PrinterVariant::_cnt) },
        { Item::custom, DynamicIndexMappingType::optional_item },
    });

    DynamicIndexMapping<items> index_mapping;
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

#if HAS_CHAMBER_VENTS()
class MI_SWITCH_VENT_MECHANISM : public MenuItemSwitch {
public:
    MI_SWITCH_VENT_MECHANISM();

protected:
    virtual void OnChange(size_t old_index) override;

    /// Keep in sync when an edition selection changes the vent control.
    void Loop() override;
};
#endif

#if HAS_SWITCHABLE_HOMING_CALIBRATION()
/// Option whether we should automatically calibrate precise homing when needed
class MI_AUTO_PRECISE_HOMING_CALIBRATION : public MenuItemSwitch {
public:
    MI_AUTO_PRECISE_HOMING_CALIBRATION();

protected:
    virtual void OnChange(size_t) override;
};
#endif

#if HAS_EXPANSION_JOINTS_GEN_2()
/// Whether the Expansion Joints Gen 2 (frame expansion joints) is installed. Affects bed-frame heat absorption.
class MI_EXPANSION_JOINTS_GEN_2 : public WI_ICON_SWITCH_OFF_ON_t {
    static constexpr const char *const label = N_("Expansion Joints Gen 2");

public:
    MI_EXPANSION_JOINTS_GEN_2();

protected:
    virtual void OnChange(size_t old_index) override;

    /// Keep in sync when an edition selection changes the flag.
    void Loop() override;
};
#endif

#if HAS_NOZZLE_CLEANER_LITE()
class MI_NOZZLE_CLEANER_LITE : public WI_ICON_SWITCH_OFF_ON_t {
    // Nozzle wiper is the product name - we call it nozzle cleaner lite
    static constexpr const char *const label = N_("Nozzle Wiper");

public:
    MI_NOZZLE_CLEANER_LITE();

protected:
    virtual void OnChange(size_t) override;

    /// Keep in sync when an edition selection changes the flag.
    void Loop() override;
};
#endif

#if HAS_15GT_BELTS()
class MI_BELTS_15GT : public WI_ICON_SWITCH_OFF_ON_t {
    static constexpr const char *const label = N_("1.5GT Belts");

public:
    MI_BELTS_15GT();

protected:
    virtual void OnChange(size_t old_index) override;
};
#endif
