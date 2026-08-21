#include "DialogHandler.hpp"

#include <gui/screen/screen_filament_change.hpp>
#include "IScreenPrinting.hpp"
#include "ScreenHandler.hpp"
#include "screen_printing.hpp"
#include "config_features.h"
#include <screen/screen_print_preview.hpp>
#include "window_dlg_quickpause.hpp"
#include "window_dlg_wait.hpp"
#include "window_dlg_warning.hpp"
#include <option/has_crash_detection.h>
#include <option/has_gearbox_alignment.h>
#include <option/has_phase_stepping_calibration.h>
#include <option/has_input_shaper_calibration.h>
#include <option/has_coldpull.h>
#include <option/has_door_sensor_calibration.h>
#include <option/has_manual_belt_tuning.h>
#include <option/has_loadcell.h>
#include <option/has_indx.h>
#include <option/has_heaters_selftest_gcode.h>
#include <option/has_tool_offset_sensor.h>
#include <gui/screen/screen_preheat.hpp>
#include <gui/screen/dialog_safety_timer.hpp>

#if HAS_LOADCELL()
    #include <gui/screen/screen_nozzle_cleaning_failed.hpp>
#endif

#if HAS_INDX()
    #include <gui/screen/screen_nozzle_mismatch.hpp>
    #include <feature/indx_dock_calibration/screen_dock_calibration.hpp>
    #include <feature/indx_nozzle_cleaner_calibration/screen_nozzle_cleaner_calibration.hpp>
#endif

#if HAS_TOOL_OFFSET_SENSOR()
    #include <feature/tool_offset_wizard/screen_tool_offset_wizard.hpp>
#endif

#if HAS_MANUAL_BELT_TUNING()
    #include <screen/selftest/screen_manual_belt_tuning.hpp>
#endif

#if HAS_COLDPULL()
    #include "screen_cold_pull.hpp"
#endif

#if HAS_SELFTEST()
    #include <screen_fan_selftest.hpp>
    #if HAS_HEATERS_SELFTEST_GCODE()
        #include <screen_heaters_selftest.hpp>
    #endif
    #include "ScreenSelftest.hpp"
    #include <gui/screen/selftest/screen_selftest_fsensors.hpp>
#endif

#if HAS_CRASH_DETECTION()
    #include "screen_crash_recovery.hpp"
#endif

#include <option/has_serial_print.h>
#if HAS_SERIAL_PRINT()
    #include "screen_printing_serial.hpp"
#endif

#if HAS_PHASE_STEPPING_CALIBRATION()
    #include "screen_phase_stepping_calibration.hpp"
#endif

#if HAS_INPUT_SHAPER_CALIBRATION()
    #include "screen_input_shaper_calibration.hpp"
#endif

#if HAS_GEARBOX_ALIGNMENT()
    #include "feature/gearbox_alignment/screen_gearbox_alignment.hpp"
#endif

#if HAS_DOOR_SENSOR_CALIBRATION()
    #include <feature/door_sensor_calibration/screen_door_sensor_calibration.hpp>
#endif

#include <option/has_esp.h>
#include <bsod/bsod.h>
#if HAS_ESP()
    #include <screen_network_setup.hpp>
#endif

alignas(std::max_align_t) static std::array<uint8_t, 1300> mem_space;

// safer than make_static_unique_ptr, checks storage size
template <class T, class... Args>
static static_unique_ptr<IDialogMarlin> make_dialog_ptr(Args &&...args) {
    return make_static_unique_ptr<T>(mem_space, std::forward<Args>(args)...);
}

struct FSMScreenDefBase {
    [[nodiscard]] static bool open_base(ScreenFactory::Creator screen, [[maybe_unused]] fsm::BaseData data) {
        // We need to get out of the GUI loop to open the screen, windows are on the stack
        if (gui_get_nesting()) {
            return false;
        }

        if (Screens::Access()->IsScreenOpened(screen)) {
            // Already opened, no need to do anything
            return true;
        }

        Screens::Access()->Open(screen);

        // Do loop immediately for the window open to take effect
        // Otherwise, the change() that follows right after open() would not do anything
        Screens::Access()->Loop();

        return true;
    }
};

template <ClientFSM fsm_, typename Screen>
struct FSMScreenDef : public FSMScreenDefBase {
    static constexpr ClientFSM fsm = fsm_;
    static constexpr auto screen = ScreenFactory::Screen<Screen>;

    [[nodiscard]] static bool open(fsm::BaseData data) {
        return open_base(screen, data);
    }

    static void close() {
        debug_assert(Screens::Access()->IsScreenOnStack<Screen>());
        Screens::Access()->Close<Screen>();
    }

    [[nodiscard]] static bool change(fsm::BaseData data) {
        // DO NOT change screens while in the nested loop
        // We would be potentially changing phases while the GUI widgets are on the stack
        if (gui_get_nesting()) {
            return false;
        }

        if (auto s = Screens::Access()->get<Screen>()) {
            s->Change(data);
            return true;

        } else {
            // The screen is not on the top of the stack, we cannot notify it about the change
            // TODO: This should be okay as the screen should read the current state at the creation, but it's not doing that currently
            return false;
        }
    }
};

struct FSMDialogDefBase {

    static void close() {
        // Ptr is a static_unique_ptr, it will call the destructor
        DialogHandler::Access().ptr = nullptr;
    }

    [[nodiscard]] static bool change(fsm::BaseData data) {
        // We CANNOT check for gui nesting for dialogs - dialogs can be shown blockingly over screens
        // One just has to hope that noone would call DialogHandler::Loop() inside a FSM dialog function - there's no nice way to check it
        // debug_assert(!gui_get_nesting())

        if (auto &ptr = DialogHandler::Access().ptr) {
            ptr->Change(data);
            return true;

        } else {
            // The dialog is not on the stact - should not happen
            debug_assert(false);
            return false;
        }
    }
};

template <ClientFSM fsm_, typename Dialog_>
struct FSMDialogDef : public FSMDialogDefBase {
    using Dialog = Dialog_;
    static constexpr ClientFSM fsm = fsm_;

    [[nodiscard]] static bool open(fsm::BaseData data) {
        auto &ptr = DialogHandler::Access().ptr;
        if (ptr) {
            // This should never happen - appropriate close() should always be called before
            bsod("Opening 2nd FSM dialog");
        }

        // If we're about to close some screen, do it now, so that we can spawn the dialog directly over the new top
        // This reduces redraws
        if (!gui_get_nesting()) {
            Screens::Access()->Loop();
        }

        ptr = make_dialog_ptr<Dialog>(data);
        return true;
    }
};

template <ClientFSM fsm_>
struct FSMPrintDef {
    static constexpr ClientFSM fsm = fsm_;

    [[nodiscard]] static bool open([[maybe_unused]] fsm::BaseData data) {
        // We need to get out of the GUI loop to open the screen, windows are on the stack
        if (gui_get_nesting()) {
            return false;
        }

        ScreenFactory::Creator screen;

#if HAS_SERIAL_PRINT()
        if constexpr (fsm == ClientFSM::Serial_printing) {
            screen = ScreenFactory::Screen<screen_printing_serial_data_t>;
            if (Screens::Access()->IsScreenOpened(screen)) {
                // Already opened, no need to do anything
                return true;
            }

            Screens::Access()->ClosePrinting();

        } else
#endif
            if constexpr (fsm == ClientFSM::Printing) {
            screen = ScreenFactory::Screen<screen_printing_data_t>;
            if (Screens::Access()->IsScreenOpened(screen)) {
                // Already opened, no need to do anything
                return true;
            }

            Screens::Access()->CloseAll();

        } else {
            static_assert(0);
        }

        Screens::Access()->Open(screen);

        // Do loop immediately for the window open to take effect
        // Otherwise, the change() that follows right after open() would not do anything
        Screens::Access()->Loop();
        return true;
    }

    static void close() {
        Screens::Access()->CloseAll();
    }

    [[nodiscard]] static bool change([[maybe_unused]] fsm::BaseData data) {
        // Do nothing
        return true;
    }
};

// Just so that we have something at the end of the list and don't have to care about commas
struct FSMEndDef {
    static constexpr ClientFSM fsm = ClientFSM::_count;

    [[nodiscard]] static bool open(fsm::BaseData) {
        return true;
    }

    static void close() {}

    [[nodiscard]] static bool change(fsm::BaseData) {
        return true;
    }
};

template <class... T>
struct FSMDisplayConfigDef {
};

using FSMDisplayConfig = FSMDisplayConfigDef<
    FSMDialogDef<ClientFSM::Wait, window_dlg_wait_t>,
    FSMDialogDef<ClientFSM::SafetyTimer, DialogSafetyTimer>,
#if HAS_SERIAL_PRINT()
    FSMPrintDef<ClientFSM::Serial_printing>,
#endif
    FSMScreenDef<ClientFSM::Load_unload, ScreenFilamentChange>,
    FSMScreenDef<ClientFSM::Preheat, ScreenPreheat>,
#if HAS_SELFTEST()
    FSMScreenDef<ClientFSM::Selftest, ScreenSelftest>,
    FSMScreenDef<ClientFSM::FansSelftest, ScreenFanSelftest>,
    FSMScreenDef<ClientFSM::SelftestFSensors, ScreenSelftestFSensors>,
#endif
#if HAS_HEATERS_SELFTEST_GCODE()
    FSMScreenDef<ClientFSM::HeatersSelftest, ScreenHeatersSelftest>,
#endif
#if HAS_ESP()
    FSMScreenDef<ClientFSM::NetworkSetup, ScreenNetworkSetup>,
#endif
    FSMPrintDef<ClientFSM::Printing>,
#if HAS_CRASH_DETECTION()
    FSMScreenDef<ClientFSM::CrashRecovery, ScreenCrashRecovery>,
#endif
    FSMDialogDef<ClientFSM::QuickPause, DialogQuickPause>,
    FSMDialogDef<ClientFSM::Warning, DialogWarning>,
    FSMScreenDef<ClientFSM::PrintPreview, ScreenPrintPreview>,
#if HAS_COLDPULL()
    FSMScreenDef<ClientFSM::ColdPull, ScreenColdPull>,
#endif
#if HAS_PHASE_STEPPING_CALIBRATION()
    FSMScreenDef<ClientFSM::PhaseSteppingCalibration, ScreenPhaseSteppingCalibration>,
#endif
#if HAS_INPUT_SHAPER_CALIBRATION()
    FSMScreenDef<ClientFSM::InputShaperCalibration, ScreenInputShaperCalibration>,
#endif
#if HAS_GEARBOX_ALIGNMENT()
    FSMScreenDef<ClientFSM::GearboxAlignment, ScreenGearboxAlignment>,
#endif
#if HAS_DOOR_SENSOR_CALIBRATION()
    FSMScreenDef<ClientFSM::DoorSensorCalibration, ScreenDoorSensorCalibration>,
#endif
#if HAS_MANUAL_BELT_TUNING()
    FSMScreenDef<ClientFSM::ManualBeltTuning, ScreenManualBeltTuning>,
#endif
#if HAS_LOADCELL()
    FSMScreenDef<ClientFSM::NozzleCleaningFailed, ScreenNozzleCleaningFailed>,
#endif
#if HAS_INDX()
    FSMScreenDef<ClientFSM::NozzleMismatch, ScreenNozzleMismatch>,
    FSMScreenDef<ClientFSM::DockCalibration, ScreenDockCalibration>,
    FSMScreenDef<ClientFSM::NozzleCleanerCalibration, ScreenNozzleCleanerCalibration>,
#endif
#if HAS_TOOL_OFFSET_SENSOR()
    FSMScreenDef<ClientFSM::ToolOffsetsCalibration, ScreenToolOffsetWizard>,
#endif
    // This is here so that we can worry-free write commas at the end of each argument
    FSMEndDef>;

void visit_display_config(ClientFSM fsm, auto f) {
    [&]<class... T>(FSMDisplayConfigDef<T...>) {
        ((fsm == T::fsm ? f(T()) : void()), ...);
    }(FSMDisplayConfig());
};

static constexpr size_t fsm_display_config_size = []<class... T>(FSMDisplayConfigDef<T...>) { return sizeof...(T); }(FSMDisplayConfig());
static_assert(fsm_display_config_size == std::to_underlying(ClientFSM::_count) + 1);

static constexpr auto dlg_size = []<typename T> {
    if constexpr (std::is_base_of_v<FSMDialogDefBase, T>) {
        return sizeof(typename T::Dialog);
    } else {
        return 0;
    }
};
static constexpr size_t required_dlg_mem_space = []<class... T>(FSMDisplayConfigDef<T...>) { return std::max(std::initializer_list<size_t> { dlg_size.operator()<T>()... }); }(FSMDisplayConfig {});
static_assert(sizeof(mem_space) < required_dlg_mem_space + 100, "DialogHandler::mem_space is unnecessarily big");

//*****************************************************************************
// method definitions
bool DialogHandler::open(ClientFSM fsm_type, fsm::BaseData data) {
    bool result = false;
    visit_display_config(fsm_type, [&]<typename Config>(Config) {
        result = Config::open(data);
    });
    return result;
}

void DialogHandler::close(ClientFSM fsm_type) {
    visit_display_config(fsm_type, []<typename Config>(Config) {
        Config::close();
    });
}

bool DialogHandler::change(ClientFSM fsm_type, fsm::BaseData data) {
    bool result = false;

    visit_display_config(fsm_type, [&]<typename Config>(Config) {
        result = Config::change(data);
    });

    return result;
}

bool DialogHandler::IsOpen() const {
    return ptr != nullptr;
}

DialogHandler &DialogHandler::Access() {
    static DialogHandler instance;
    return instance;
}

void DialogHandler::Loop() {
    update_screen();

    // If a Screen gets destroyed underneath a FSM Dialog (IDialogMarlin), we have to re-register it to the new screen
    // Do this after update_screen to reduce GUI redraws
    if (ptr && !ptr->GetParent()) {
        auto current_screen = Screens::Access()->Get();
        ptr->SetParent(current_screen);
        current_screen->RegisterSubWin(*ptr);
    }
}

void DialogHandler::update_screen() {
    const auto new_top = marlin_vars().peek_fsm_states([](const auto &states) { return states.get_top(); });

    if (new_top == current_fsm_top) {
        return;
    }

    // Shortcut - we're just changing the data
    if (new_top && current_fsm_top && new_top->fsm_type == current_fsm_top->fsm_type) {
        if (!change(new_top->fsm_type, new_top->data)) {
            // Failed to change state - try again later
            return;
        }
        current_fsm_top = new_top;
        return;
    }

    // Close the previous screen/dialog
    if (current_fsm_top) {
        // Closing should never fail
        // Dialog - destroys the dialog (we're praying that noone got the idea to call DialogHandler::Loop() inside a FSM dialog function - there's no nice way to check it)
        // Screen - just marks the screen to be destroyed on the next Screens::Access()->Loop
        close(current_fsm_top->fsm_type);
        current_fsm_top = std::nullopt;
    }

    // Do NOT do Screens::Access()->Loop(); here.
    // The ScreenHandler works faster if it can do both close and open in the same step

    // Open the new one
    if (new_top) {
        if (!open(new_top->fsm_type, new_top->data)) {
            return;
        }

        if (!change(new_top->fsm_type, new_top->data)) {
            bsod_unreachable(); // Should never fail, we should just have opened the screen
        }

        current_fsm_top = new_top;
    }

    // Make sure all changes are carried over now to reduce redraws
    if (!gui_get_nesting()) {
        Screens::Access()->Loop();
    }
}
