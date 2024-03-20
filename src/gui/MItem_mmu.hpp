/*****************************************************************************/
// Menu items related to the MMU
#pragma once
#include "WindowMenuItems.hpp"
#include "WindowMenuInfo.hpp"
#include "WindowItemFormatableLabel.hpp"
#include "i18n.h"

class MI_MMU_PRELOAD_ADVANCED : public IWindowMenuItem {
    static constexpr const char *const label = N_("Preload to MMU Advanced");

public:
    MI_MMU_PRELOAD_ADVANCED();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_MMU_PRELOAD : public IWindowMenuItem {
    static constexpr const char *const label = N_("Preload to MMU");

public:
    MI_MMU_PRELOAD();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_MMU_LOAD_TEST_FILAMENT : public IWindowMenuItem {
    static constexpr const char *const label = N_("Loading Test");

public:
    MI_MMU_LOAD_TEST_FILAMENT();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_MMU_LOAD_TO_NOZZLE : public IWindowMenuItem {
    static constexpr const char *const label = N_("Load to Nozzle");

public:
    MI_MMU_LOAD_TO_NOZZLE();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_MMU_EJECT_FILAMENT : public IWindowMenuItem {
    static constexpr const char *const label = N_("Eject Filament");

public:
    MI_MMU_EJECT_FILAMENT();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_MMU_CUT_FILAMENT : public IWindowMenuItem {
    static constexpr const char *const label = N_("Cut Filament");

public:
    MI_MMU_CUT_FILAMENT();

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
    MI_MMU_ISSUE_GCODE(const char *lbl, const char *gcode, is_hidden_t hidden = is_hidden_t::no);

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_MMU_ISSUE_GCODE_SLOT : public IWindowMenuItem {

public:
    /// \param slot_i Slot index, starting at 0. For label, +1 is shown
    /// \param gcode_fmt Format used for snprintf. An '%i' parameter is passed, being the 0-indexed slot number.
    MI_MMU_ISSUE_GCODE_SLOT(uint8_t slot_i, const char *label_prefix, const char *gcode_fmt);

protected:
    void click(IWindowMenu &window_menu) final;

private:
    // We need a buffer to store the formatted string
    // because the label_base needs to go through snprintf
    std::array<char, 32> label_;

    const char *const gcode_fmt_;

    const uint8_t slot_i_;
};

template <uint8_t slot_i>
class MI_MMU_EJECT_FILAMENT_I : public MI_MMU_ISSUE_GCODE_SLOT {

public:
    MI_MMU_EJECT_FILAMENT_I()
        : MI_MMU_ISSUE_GCODE_SLOT(slot_i, N_("Eject Filament"), "M705 P%i") {}
};

class MI_MMU_PRELOAD_SLOT_1 : public MI_MMU_ISSUE_GCODE {
    static constexpr const char *const label = N_("Preload slot 1");

public:
    MI_MMU_PRELOAD_SLOT_1()
        : MI_MMU_ISSUE_GCODE(label, "M704 P0") {}
};

class MI_MMU_PRELOAD_SLOT_2 : public MI_MMU_ISSUE_GCODE {
    static constexpr const char *const label = N_("Preload slot 2");

public:
    MI_MMU_PRELOAD_SLOT_2()
        : MI_MMU_ISSUE_GCODE(label, "M704 P1") {}
};

class MI_MMU_PRELOAD_SLOT_3 : public MI_MMU_ISSUE_GCODE {
    static constexpr const char *const label = N_("Preload slot 3");

public:
    MI_MMU_PRELOAD_SLOT_3()
        : MI_MMU_ISSUE_GCODE(label, "M704 P2") {}
};

class MI_MMU_PRELOAD_SLOT_4 : public MI_MMU_ISSUE_GCODE {
    static constexpr const char *const label = N_("Preload slot 4");

public:
    MI_MMU_PRELOAD_SLOT_4()
        : MI_MMU_ISSUE_GCODE(label, "M704 P3") {}
};

class MI_MMU_PRELOAD_SLOT_5 : public MI_MMU_ISSUE_GCODE {
    static constexpr const char *const label = N_("Preload slot 5");

public:
    MI_MMU_PRELOAD_SLOT_5()
        : MI_MMU_ISSUE_GCODE(label, "M704 P4") {}
};

class MI_MMU_LOAD_TO_NOZZLE_1 : public MI_MMU_ISSUE_GCODE {
    static constexpr const char *const label = N_("Load Filament 1");

public:
    MI_MMU_LOAD_TO_NOZZLE_1()
        : MI_MMU_ISSUE_GCODE(label, "M701 W2 P0") {} // load filament slot 0 with preheat
};

class MI_MMU_LOAD_TO_NOZZLE_2 : public MI_MMU_ISSUE_GCODE {
    static constexpr const char *const label = N_("Load Filament 2");

public:
    MI_MMU_LOAD_TO_NOZZLE_2()
        : MI_MMU_ISSUE_GCODE(label, "M701 W2 P1") {}
};

class MI_MMU_LOAD_TO_NOZZLE_3 : public MI_MMU_ISSUE_GCODE {
    static constexpr const char *const label = N_("Load Filament 3");

public:
    MI_MMU_LOAD_TO_NOZZLE_3()
        : MI_MMU_ISSUE_GCODE(label, "M701 W2 P2") {}
};

class MI_MMU_LOAD_TO_NOZZLE_4 : public MI_MMU_ISSUE_GCODE {
    static constexpr const char *const label = N_("Load Filament 4");

public:
    MI_MMU_LOAD_TO_NOZZLE_4()
        : MI_MMU_ISSUE_GCODE(label, "M701 W2 P3") {}
};

class MI_MMU_LOAD_TO_NOZZLE_5 : public MI_MMU_ISSUE_GCODE {
    static constexpr const char *const label = N_("Load Filament 5");

public:
    MI_MMU_LOAD_TO_NOZZLE_5()
        : MI_MMU_ISSUE_GCODE(label, "M701 W2 P4") {}
};

class MI_MMU_CUT_FILAMENT_1 : public MI_MMU_ISSUE_GCODE {
    static constexpr const char *const label = N_("Cut Filament 1");

public:
    MI_MMU_CUT_FILAMENT_1()
        : MI_MMU_ISSUE_GCODE(label, "M706 P0") {}
};

class MI_MMU_CUT_FILAMENT_2 : public MI_MMU_ISSUE_GCODE {
    static constexpr const char *const label = N_("Cut Filament 2");

public:
    MI_MMU_CUT_FILAMENT_2()
        : MI_MMU_ISSUE_GCODE(label, "M706 P1") {}
};
class MI_MMU_CUT_FILAMENT_3 : public MI_MMU_ISSUE_GCODE {
    static constexpr const char *const label = N_("Cut Filament 3");

public:
    MI_MMU_CUT_FILAMENT_3()
        : MI_MMU_ISSUE_GCODE(label, "M706 P2") {}
};
class MI_MMU_CUT_FILAMENT_4 : public MI_MMU_ISSUE_GCODE {
    static constexpr const char *const label = N_("Cut Filament 4");

public:
    MI_MMU_CUT_FILAMENT_4()
        : MI_MMU_ISSUE_GCODE(label, "M706 P3") {}
};
class MI_MMU_CUT_FILAMENT_5 : public MI_MMU_ISSUE_GCODE {
    static constexpr const char *const label = N_("Cut Filament 5");

public:
    MI_MMU_CUT_FILAMENT_5()
        : MI_MMU_ISSUE_GCODE(label, "M706 P4") {}
};

class MI_MMU_UNLOAD_FILAMENT : public MI_MMU_ISSUE_GCODE {
    static constexpr const char *const label = N_("Unload Filament");

public:
    MI_MMU_UNLOAD_FILAMENT()
        : MI_MMU_ISSUE_GCODE(label, "M702 W2") {}
};

class MI_MMU_LOAD_TEST_ALL : public IWindowMenuItem {
    static constexpr const char *const label = N_("Test All");

public:
    MI_MMU_LOAD_TEST_ALL();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_MMU_LOAD_TEST_FILAMENT_1 : public MI_MMU_ISSUE_GCODE {
    static constexpr const char *const label = N_("Test Filament 1");

public:
    MI_MMU_LOAD_TEST_FILAMENT_1()
        : MI_MMU_ISSUE_GCODE(label, "M1704 P0") {}
};

class MI_MMU_LOAD_TEST_FILAMENT_2 : public MI_MMU_ISSUE_GCODE {
    static constexpr const char *const label = N_("Test Filament 2");

public:
    MI_MMU_LOAD_TEST_FILAMENT_2()
        : MI_MMU_ISSUE_GCODE(label, "M1704 P1") {}
};

class MI_MMU_LOAD_TEST_FILAMENT_3 : public MI_MMU_ISSUE_GCODE {
    static constexpr const char *const label = N_("Test Filament 3");

public:
    MI_MMU_LOAD_TEST_FILAMENT_3()
        : MI_MMU_ISSUE_GCODE(label, "M1704 P2") {}
};

class MI_MMU_LOAD_TEST_FILAMENT_4 : public MI_MMU_ISSUE_GCODE {
    static constexpr const char *const label = N_("Test Filament 4");

public:
    MI_MMU_LOAD_TEST_FILAMENT_4()
        : MI_MMU_ISSUE_GCODE(label, "M1704 P3") {}
};

class MI_MMU_LOAD_TEST_FILAMENT_5 : public MI_MMU_ISSUE_GCODE {
    static constexpr const char *const label = N_("Test Filament 5");

public:
    MI_MMU_LOAD_TEST_FILAMENT_5()
        : MI_MMU_ISSUE_GCODE(label, "M1704 P4") {}
};

class MI_MMU_SW_RESET : public MI_MMU_ISSUE_GCODE {
    static constexpr const char *const label = N_("SW Reset");

public:
    MI_MMU_SW_RESET()
        : MI_MMU_ISSUE_GCODE(label, "M709 X0", is_hidden_t::dev) {}
};

class MI_MMU_HW_RESET : public MI_MMU_ISSUE_GCODE {
    static constexpr const char *const label = N_("HW Reset");

public:
    MI_MMU_HW_RESET()
        : MI_MMU_ISSUE_GCODE(label, "M709 X1", is_hidden_t::dev) {}
};

class MI_MMU_POWER_CYCLE : public MI_MMU_ISSUE_GCODE {
    static constexpr const char *const label = N_("Power Cycle");

public:
    MI_MMU_POWER_CYCLE()
        : MI_MMU_ISSUE_GCODE(label, "M709 X2", is_hidden_t::dev) {}
};

// The following homing modes are not yet supported in the MMU FW
// class MI_MMU_HOME1 : public MI_MMU_ISSUE_GCODE {
//    static constexpr const char *const label = N_("Force home Idler");

// public:
//     MI_MMU_HOME1()
//         : MI_MMU_ISSUE_GCODE(label, "M1400 S10 H1") {}
// };

// class MI_MMU_HOME2 : public MI_MMU_ISSUE_GCODE {
//     static constexpr const char *const label = N_("Force home Selector");

// public:
//     MI_MMU_HOME2()
//         : MI_MMU_ISSUE_GCODE(label, "M1400 S10 H2") {}
// };

class MI_MMU_ENABLE : public WI_ICON_SWITCH_OFF_ON_t {
    constexpr static const char *const label = N_("MMU Unit");

public:
    MI_MMU_ENABLE();
    virtual void OnChange(size_t old_index) override;
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
    constexpr static const char *const label = N_("Load Fails in Print");

public:
    MI_MMU_LOAD_FAILS();
};

class MI_MMU_TOTAL_LOAD_FAILS : public WI_INFO_t {
    constexpr static const char *const label = N_("Total Load Fails");

public:
    MI_MMU_TOTAL_LOAD_FAILS();
};

class MI_MMU_GENERAL_FAILS : public WI_INFO_t {
    constexpr static const char *const label = N_("General Fails in Print");

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
/// This switch gives allows the user to set if the printer has the stock extruder or the MMU reworked one.
class MI_MMU_NEXTRUDER_REWORK : public WI_SWITCH_t<2> {
#if HAS_LOADCELL()
    static constexpr const char *const label = N_("Nextruder");
#else
    // for MK3.5 we call this option simply "Extruder"
    static constexpr const char *const label = N_("Extruder");
#endif
    static constexpr const char *const val_0 = N_("Stock");
    static constexpr const char *const val_1 = N_("MMU");

public:
    MI_MMU_NEXTRUDER_REWORK();

protected:
    virtual void OnChange(size_t old_index) override;
};

class MI_INFO_FINDA : public WI_FORMATABLE_LABEL_t<bool> {
    static constexpr const char *label = N_("FINDA");

public:
    MI_INFO_FINDA();
};
