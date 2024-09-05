#include "DialogHandler.hpp"

#include "DialogLoadUnload.hpp"
#include "IScreenPrinting.hpp"
#include "ScreenHandler.hpp"
#include "screen_printing.hpp"
#include "config_features.h"
#include "screen_print_preview.hpp"
#include "window_dlg_quickpause.hpp"
#include "window_dlg_warning.hpp"
#include <screen_network_setup.hpp>
#include <option/has_phase_stepping.h>
#include <option/has_input_shaper_calibration.h>
#include <option/has_coldpull.h>
#include <gui/screen/screen_preheat.hpp>

#if HAS_COLDPULL()
    #include "screen_cold_pull.hpp"
#endif

#if HAS_SELFTEST()
    #include "ScreenSelftest.hpp"
#endif

#if ENABLED(CRASH_RECOVERY)
    #include "screen_crash_recovery.hpp"
using CrashRecovery = ScreenCrashRecovery;
#else
    #include "screen_dialog_does_not_exist.hpp"
using CrashRecovery = ScreenDialogDoesNotExist;
#endif

#include <option/has_serial_print.h>
#if HAS_SERIAL_PRINT()
    #include "screen_printing_serial.hpp"
using SerialPrint = screen_printing_serial_data_t;
#else
    #include "screen_dialog_does_not_exist.hpp"
using SerialPrint = ScreenDialogDoesNotExist;
#endif

#if HAS_PHASE_STEPPING()
    #include "screen_phase_stepping.hpp"
#endif

#if HAS_INPUT_SHAPER_CALIBRATION()
    #include "screen_input_shaper_calibration.hpp"
#endif

#if HAS_BELT_TUNING()
    #include <gui/wizard/screen_belt_tuning_wizard.hpp>
#endif

alignas(std::max_align_t) static std::array<uint8_t, 2560> mem_space;

// safer than make_static_unique_ptr, checks storage size
template <class T, class... Args>
static static_unique_ptr<IDialogMarlin> make_dialog_ptr(Args &&...args) {
    static_assert(sizeof(T) <= mem_space.size(), "Error dialog does not fit");
    return make_static_unique_ptr<T>(mem_space.data(), std::forward<Args>(args)...);
}

static void open_screen_if_not_opened(ScreenFactory::Creator c) {
    auto scrns = Screens::Access();
    if (!scrns->IsScreenOpened(c)) {
        scrns->Open(c);
    }
}

template <ClientFSM fsm_, typename Screen>
struct FSMScreenDef {
    static constexpr ClientFSM fsm = fsm_;

    static void open([[maybe_unused]] fsm::BaseData data) {
        open_screen_if_not_opened(ScreenFactory::Screen<Screen>);
    }

    static void close() {
        assert(Screens::Access()->IsScreenOnStack<Screen>());
        Screens::Access()->Close<Screen>();
    }

    static void change(fsm::BaseData data) {
        if (auto s = Screens::Access()->get<Screen>()) {
            s->Change(data);
        }
    }
};

template <ClientFSM fsm_, typename Dialog>
struct FSMDialogDef {
    static constexpr ClientFSM fsm = fsm_;

    static void open(fsm::BaseData data) {
        DialogHandler::Access().ptr = make_dialog_ptr<Dialog>(data);
    }

    static void close() {
        // Do nothing, is handled elsewhere
    }

    static void change(fsm::BaseData data) {
        if (auto &ptr = DialogHandler::Access().ptr) {
            ptr->Change(data);
        }
    }
};

template <ClientFSM fsm_>
struct FSMPrintDef {
    static constexpr ClientFSM fsm = fsm_;

    static void open([[maybe_unused]] fsm::BaseData data) {
        if (IScreenPrinting::GetInstance()) {
            IScreenPrinting::NotifyMarlinStart();
            return;
        }

        if constexpr (fsm == ClientFSM::Serial_printing) {
            Screens::Access()->ClosePrinting();
            Screens::Access()->Open(ScreenFactory::Screen<SerialPrint>);

        } else if constexpr (fsm == ClientFSM::Printing) {
            Screens::Access()->CloseAll();
            Screens::Access()->Open(ScreenFactory::Screen<screen_printing_data_t>);

        } else {
            static_assert(0);
        }
    }

    static void close() {
        Screens::Access()->CloseAll();
    }

    static void change([[maybe_unused]] fsm::BaseData data) {
        // Do nothing
    }
};

// Just so that we have something at the end of the list and don't have to care about commas
struct FSMEndDef {
    static constexpr ClientFSM fsm = ClientFSM::_count;

    static void open(fsm::BaseData) {}
    static void close() {}
    static void change(fsm::BaseData) {}
};

template <class... T>
struct FSMDisplayConfigDef {
};

using FSMDisplayConfig = FSMDisplayConfigDef<
    FSMPrintDef<ClientFSM::Serial_printing>,
    FSMDialogDef<ClientFSM::Load_unload, DialogLoadUnload>,
    FSMScreenDef<ClientFSM::Preheat, ScreenPreheat>,
#if HAS_SELFTEST()
    FSMScreenDef<ClientFSM::Selftest, ScreenSelftest>,
#endif
    FSMScreenDef<ClientFSM::NetworkSetup, ScreenNetworkSetup>,
    FSMPrintDef<ClientFSM::Printing>,
#if ENABLED(CRASH_RECOVERY)
    FSMScreenDef<ClientFSM::CrashRecovery, ScreenCrashRecovery>,
#endif
    FSMDialogDef<ClientFSM::QuickPause, DialogQuickPause>,
    FSMDialogDef<ClientFSM::Warning, DialogWarning>,
    FSMScreenDef<ClientFSM::PrintPreview, ScreenPrintPreview>,
#if HAS_COLDPULL()
    FSMScreenDef<ClientFSM::ColdPull, ScreenColdPull>,
#endif
#if HAS_PHASE_STEPPING()
    FSMScreenDef<ClientFSM::PhaseStepping, ScreenPhaseStepping>,
#endif
#if HAS_INPUT_SHAPER_CALIBRATION()
    FSMScreenDef<ClientFSM::InputShaperCalibration, ScreenInputShaperCalibration>,
#endif
#if HAS_BELT_TUNING()
    FSMScreenDef<ClientFSM::BeltTuning, ScreenBeltTuningWizard>,
#endif

    // This is here so that we can worry-free write commas at the end of each argument
    FSMEndDef>;

void visit_display_config(ClientFSM fsm, auto f) {
    [&]<class... T>(FSMDisplayConfigDef<T...>) {
        ((fsm == T::fsm ? f(T()) : void()), ...);
    }(FSMDisplayConfig());
};

static constexpr size_t fsm_display_config_size = []<class... T>(FSMDisplayConfigDef<T...>) { return sizeof...(T); }(FSMDisplayConfig());
static_assert(fsm_display_config_size == ftrstd::to_underlying(ClientFSM::_count) + 1);

//*****************************************************************************
// method definitions
void DialogHandler::open(ClientFSM fsm_type, fsm::BaseData data) {
    if (ptr) {
        if (dialog_cache.has_value()) {
            // TODO: Make all dialogs screens and use Screens state stack
            bsod("Can't open more then 2 dialogs at a time.");
        }

        dialog_cache = last_fsm_change;
        ptr = nullptr;
    }

    last_fsm_change = std::make_pair(fsm_type, data);

    visit_display_config(fsm_type, [&]<typename Config>(Config) {
        Config::open(data);
    });
}

void DialogHandler::close(ClientFSM fsm_type) {
    visit_display_config(fsm_type, []<typename Config>(Config) {
        Config::close();
    });

    // Attempt to restore underlying screen state
    if (ptr != nullptr) {
        if (dialog_cache.has_value()) {
            ptr = nullptr;
            const auto cache = *dialog_cache;
            dialog_cache = std::nullopt;
            open(cache.first, cache.second);
        } else {
            ptr = nullptr; // destroy current dialog
        }
    }
}

void DialogHandler::change(ClientFSM fsm_type, fsm::BaseData data) {
    last_fsm_change = std::make_pair(fsm_type, data);

    visit_display_config(fsm_type, [&]<typename Config>(Config) {
        Config::change(data);
    });
}

bool DialogHandler::IsOpen() const {
    return ptr != nullptr;
}

DialogHandler &DialogHandler::Access() {
    static DialogHandler instance;
    return instance;
}

void DialogHandler::Loop() {
    const auto old_top = current_fsm_top;
    const auto new_top = marlin_vars().get_fsm_states().get_top();

    if (new_top == old_top) {
        return;
    }

    // TODO Investigate whether Screens::Access()->Loop() is really needed.
    // TODO Update open() so that we won't need to call change() afterwards.
    if (new_top && old_top) {
        if (new_top->fsm_type == old_top->fsm_type) {
            if (new_top->data != old_top->data) {
                change(new_top->fsm_type, new_top->data);
            }
        } else {
            if (new_top->fsm_type == ClientFSM::Load_unload && (old_top->fsm_type == ClientFSM::PrintPreview
#if HAS_COLDPULL()
                    || old_top->fsm_type == ClientFSM::ColdPull
#endif
                    )) {
                // TODO Remove this shitcode/prasohack as soon as possible.
                //      As a special exception we do not close PrintPreview screen when the LoadUnload dialog
                //      is requested. It would destroy the ToolsMappingBody while one of its methods is still
                //      executing, leading to calling refresh_physical_tool_filament_labels() which in turn
                //      jumped to undefined memory.
            } else {
                close(old_top->fsm_type);
                Screens::Access()->Loop();
            }
            open(new_top->fsm_type, new_top->data);
            Screens::Access()->Loop();
            change(new_top->fsm_type, new_top->data);
        }
    } else if (new_top && !old_top) {
        open(new_top->fsm_type, new_top->data);
        Screens::Access()->Loop();
        change(new_top->fsm_type, new_top->data);
    } else if (!new_top && old_top) {
        close(old_top->fsm_type);
        Screens::Access()->Loop();
    } else {
        std::abort();
    }

    current_fsm_top = new_top;
}

bool DialogHandler::IsAnyOpen() const {
    return current_fsm_top.has_value();
}
