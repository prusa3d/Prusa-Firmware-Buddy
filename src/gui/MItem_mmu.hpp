/// @file
#pragma once

#include <array>

#include "WindowMenuItems.hpp"
#include "WindowMenuInfo.hpp"
#include "WindowItemFormatableLabel.hpp"
#include "i18n.h"
#include <option/has_nextruder.h>
#include <tool_index.hpp>
#include <gui_utils.hpp>
#include <gui/menu_item/menu_item_select_menu.hpp>
#include <gui/menu_item/menu_item_virtual_submenu.hpp>

class MI_MMU_PRELOAD : public IWindowMenuItem {
    static constexpr const char *const label = N_("Preload To MMU");

public:
    MI_MMU_PRELOAD();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_MMU_PRELOAD_ALL : public IWindowMenuItem {
    static constexpr const char *const label = N_("Preload All");

public:
    MI_MMU_PRELOAD_ALL();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

// @@TODO unify the individual slot menu items, since they only differ by the slot's number
class MI_MMU_ISSUE_GCODE : public IWindowMenuItem {
    const char *gcode;

public:
    MI_MMU_ISSUE_GCODE(TranslatableString label, const char *gcode, is_hidden_t hidden = is_hidden_t::no);

protected:
    virtual void click(IWindowMenu &) override;
};

class MI_MMU_ITEM_WITH_SLOT : public IWindowMenuItem {

public:
    /// \param slot_i Slot index, starting at 0. For label, +1 is shown
    MI_MMU_ITEM_WITH_SLOT(uint8_t slot_i, TranslatableString label);

protected:
    StringViewUtf8Parameters<3> label_params_;

    const uint8_t slot_i_;
};

class MI_MMU_ISSUE_GCODE_SLOT_FMT : public MI_MMU_ITEM_WITH_SLOT {

public:
    /// \param slot_i Slot index, starting at 0. For label, +1 is shown
    /// \param gcode_fmt Format used for snprintf. An '%i' parameter is passed, being the 0-indexed slot number.
    MI_MMU_ISSUE_GCODE_SLOT_FMT(uint8_t slot_i, TranslatableString label, const char *gcode_fmt);

protected:
    virtual void click(IWindowMenu &) override;

    const char *const gcode_fmt_;
};

class MI_MMU_EJECT_FILAMENT_I : public MI_MMU_ISSUE_GCODE_SLOT_FMT {

public:
    MI_MMU_EJECT_FILAMENT_I(uint8_t slot_i)
        : MI_MMU_ISSUE_GCODE_SLOT_FMT(slot_i, N_("Eject From MMU %i"), "M705 P%i") {}
};

class MI_MMU_EJECT_FILAMENT : public MenuItemVirtualSubmenu<N_("Eject From MMU"), MI_MMU_EJECT_FILAMENT_I, VirtualToolIndex::count> {

public:
    MI_MMU_EJECT_FILAMENT();
};

/// Loads the filament from the slot to the nozzle, with autopreheat based on the filament type stored for the slot.
/// Shows the stored filament in the extension.
class MI_MMU_LOAD_TO_NOZZLE_I : public MI_MMU_ITEM_WITH_SLOT {

public:
    MI_MMU_LOAD_TO_NOZZLE_I(VirtualToolIndex tool);

protected:
    void click(IWindowMenu &) final;
    void printExtension(Rect16 extension_rect, Color color_text, Color color_back, ropfn raster_op) const final;

private:
    std::array<char, 32> info_buffer_;
    MenuItemColorSwatchExtension extension_;
    FilamentType filament_type_;
};

using MI_MMU_LOAD_TO_NOZZLE = MenuItemVirtualSubmenu<N_("Load To Nozzle"), MI_MMU_LOAD_TO_NOZZLE_I, VirtualToolIndex::count, VirtualToolIndex::from_raw>;

class MI_MMU_CUT_FILAMENT_I : public MI_MMU_ISSUE_GCODE_SLOT_FMT {

public:
    MI_MMU_CUT_FILAMENT_I(uint8_t slot_i)
        : MI_MMU_ISSUE_GCODE_SLOT_FMT(slot_i, N_("Cut Filament %i"), "M706 P%i") {}
};

class MI_MMU_CUT_FILAMENT : public MenuItemVirtualSubmenu<N_("Cut Filament"), MI_MMU_CUT_FILAMENT_I, VirtualToolIndex::count> {

public:
    MI_MMU_CUT_FILAMENT();
};

class MI_MMU_LOAD_TEST_FILAMENT_I : public MI_MMU_ISSUE_GCODE_SLOT_FMT {

public:
    MI_MMU_LOAD_TEST_FILAMENT_I(uint8_t slot_i)
        : MI_MMU_ISSUE_GCODE_SLOT_FMT(slot_i, N_("Test Filament %i"), "M1704 P%i M0") {}
};

class MI_MMU_LOAD_TEST_FILAMENT : public MenuItemVirtualSubmenu<N_("Loading Test"), MI_MMU_LOAD_TEST_FILAMENT_I, VirtualToolIndex::count> {

public:
    MI_MMU_LOAD_TEST_FILAMENT();
};

class MI_MMU_UNLOAD_FILAMENT : public MI_MMU_ISSUE_GCODE {

public:
    MI_MMU_UNLOAD_FILAMENT()
        : MI_MMU_ISSUE_GCODE(N_("Unload Filament"), "M702 W2") {}
};

class MI_MMU_SW_RESET : public MI_MMU_ISSUE_GCODE {

public:
    MI_MMU_SW_RESET()
        : MI_MMU_ISSUE_GCODE(N_("SW Reset"), "M709 X0", is_hidden_t::dev) {}
};

class MI_MMU_HW_RESET : public MI_MMU_ISSUE_GCODE {

public:
    MI_MMU_HW_RESET()
        : MI_MMU_ISSUE_GCODE(N_("HW Reset"), "M709 X1", is_hidden_t::dev) {}
};

class MI_MMU_POWER_CYCLE : public MI_MMU_ISSUE_GCODE {

public:
    MI_MMU_POWER_CYCLE()
        : MI_MMU_ISSUE_GCODE(N_("Power Cycle"), "M709 X2", is_hidden_t::dev) {}
};

class MI_MMU_ENABLE : public WI_ICON_SWITCH_OFF_ON_t {
    constexpr static const char *const label = N_("MMU Unit");

public:
    MI_MMU_ENABLE();
    virtual void OnChange(size_t old_index) override;
    virtual void Loop() override;
};

class MI_MMU_CUTTER : public WI_ICON_SWITCH_OFF_ON_t {
    constexpr static const char *const label = N_("Cutter");

public:
    MI_MMU_CUTTER();
    virtual void OnChange(size_t old_index) override;
};

class MI_MMU_STEALTH_MODE : public WI_ICON_SWITCH_OFF_ON_t {
    constexpr static const char *const label = N_("Stealth Mode");

public:
    MI_MMU_STEALTH_MODE();
    virtual void OnChange(size_t old_index) override;
};

class MI_MMU_LOAD_FAILS : public WI_INFO_t {
    constexpr static const char *const label = N_("Load Fails In Print");

public:
    MI_MMU_LOAD_FAILS();
};

class MI_MMU_INVOKE_MAINTENANCE : public IWindowMenuItem {
    constexpr static const char *const label = N_("Invoke MMU Maintenance");

public:
    MI_MMU_INVOKE_MAINTENANCE();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_MMU_TOTAL_LOAD_FAILS : public WI_INFO_t {
    constexpr static const char *const label = N_("Total Load Fails");

public:
    MI_MMU_TOTAL_LOAD_FAILS();
};

class MI_MMU_GENERAL_FAILS : public WI_INFO_t {
    constexpr static const char *const label = N_("General Fails In Print");

public:
    MI_MMU_GENERAL_FAILS();
    ;
};

class MI_MMU_TOTAL_GENERAL_FAILS : public WI_INFO_t {
    constexpr static const char *const label = N_("Total General Fails");

public:
    MI_MMU_TOTAL_GENERAL_FAILS();
};

/// For Mk4, MMU requires replacement of some parts of the nextruder to work properly
/// (fsensor has to trigger only after the extruder wheels engage the filament)
/// This select allows the user to set whether the printer has the stock extruder or the MMU reworked one.
class MI_MMU_NEXTRUDER_REWORK : public MenuItemSelectMenu {
public:
    MI_MMU_NEXTRUDER_REWORK();

    int item_count() const override;
    string_view_utf8 build_item_text(int index, StringViewUtf8ParamBase &params) const override;

protected:
    bool on_item_selected(const OnItemSelectedArgs &args) override;
};

class MI_INFO_FINDA : public MenuItemAutoUpdatingLabel<bool> {
public:
    MI_INFO_FINDA();
};

class MI_MMU_BOOTLOADER_RESULT : public WI_FORMATABLE_LABEL_t<bool> {
    static constexpr const char *label = N_("MMU BL Status");

public:
    MI_MMU_BOOTLOADER_RESULT();
};

class MI_DONE_EXTRUDER_MAINTENANCE : public IWindowMenuItem {
#if HAS_NEXTRUDER()
    constexpr static const char *const label = N_("Nextruder Maintenance");
#else
    constexpr static const char *const label = N_("Extruder Maintenance");
#endif
public:
    MI_DONE_EXTRUDER_MAINTENANCE();

    virtual void click(IWindowMenu &) override;
};
