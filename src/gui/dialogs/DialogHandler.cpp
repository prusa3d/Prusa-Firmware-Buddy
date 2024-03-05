#include "DialogHandler.hpp"

#include "DialogLoadUnload.hpp"
#include "IScreenPrinting.hpp"
#include "ScreenHandler.hpp"
#include "ScreenESP.hpp"
#include "screen_printing.hpp"
#include "config_features.h"
#include "screen_print_preview.hpp"
#include "log.h"
#include "window_dlg_preheat.hpp"
#include "window_dlg_quickpause.hpp"
#include "window_dlg_warning.hpp"

#if HAS_COLDPULL()
    #include "screen_cold_pull.hpp"
#endif

LOG_COMPONENT_REF(GUI);

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

using mem_space = std::aligned_union_t<0, DialogQuickPause, DialogLoadUnload, DialogMenuPreheat, DialogWarning
#if HAS_COLDPULL()
    ,
    ScreenColdPull
#endif
    >;
static mem_space all_dialogs;

// safer than make_static_unique_ptr, checks storage size
template <class T, class... Args>
static static_unique_ptr<IDialogMarlin> make_dialog_ptr(Args &&...args) {
    static_assert(sizeof(T) <= sizeof(all_dialogs), "Error dialog does not fit");
    return make_static_unique_ptr<T>(&all_dialogs, std::forward<Args>(args)...);
}

static void OpenPrintScreen(ClientFSM dialog) {
    switch (dialog) {
    case ClientFSM::Serial_printing:
        Screens::Access()->ClosePrinting();
        Screens::Access()->Open(ScreenFactory::Screen<SerialPrint>);
        return;
    case ClientFSM::Printing:
        Screens::Access()->CloseAll();
        Screens::Access()->Open(ScreenFactory::Screen<screen_printing_data_t>);
        return;
    default:
        return;
    }
}

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

    // todo get_scr_printing_serial() is no dialog but screen ... change to dialog?
    //  only ptr = dialog_creators[dialog](data); should remain
    switch (fsm_type) {
    case ClientFSM::Serial_printing:
    case ClientFSM::Printing:
        if (IScreenPrinting::GetInstance() == nullptr) {
            OpenPrintScreen(fsm_type);
        } else {
            // opened, notify it
            IScreenPrinting::NotifyMarlinStart();
        }
        break;
    case ClientFSM::PrintPreview:
        if (!ScreenPrintPreview::GetInstance()) {
            Screens::Access()->Open(ScreenFactory::Screen<ScreenPrintPreview>);
        }
        break;
    case ClientFSM::CrashRecovery:
        if (!CrashRecovery::GetInstance()) {
            Screens::Access()->Open(ScreenFactory::Screen<CrashRecovery>);
        }
        break;
    case ClientFSM::Selftest:
#if HAS_SELFTEST()
        if (!ScreenSelftest::GetInstance()) {
            // data contain screen caption type
            // ScreenSelftest::SetHeaderMode(...);
            Screens::Access()->Open(ScreenFactory::Screen<ScreenSelftest>);
        }
#endif // HAS_SELFTEST
        break;
    case ClientFSM::ESP:
        if (!ScreenESP::GetInstance()) {
            Screens::Access()->Open(ScreenFactory::Screen<ScreenESP>);
        }
        break;
    case ClientFSM::ColdPull:
#if HAS_COLDPULL()
        if (!ScreenColdPull::GetInstance()) {
            Screens::Access()->Open(ScreenFactory::Screen<ScreenColdPull>);
        }
#endif
        break;
    case ClientFSM::QuickPause:
        ptr = make_dialog_ptr<DialogQuickPause>(data);
        break;
    case ClientFSM::Warning:
        ptr = make_dialog_ptr<DialogWarning>(data);
        break;
    case ClientFSM::Load_unload:
        ptr = make_dialog_ptr<DialogLoadUnload>(data);
        break;
    case ClientFSM::Preheat:
        ptr = make_dialog_ptr<DialogMenuPreheat>(data);
        break;
    case ClientFSM::_none:
        break;
    }
}

void DialogHandler::close(ClientFSM fsm_type) {
    // following are screens (not dialogs)
    switch (fsm_type) {
    case ClientFSM::Serial_printing:
    case ClientFSM::Printing:
        Screens::Access()->CloseAll();
        break;
    case ClientFSM::PrintPreview:
    case ClientFSM::CrashRecovery:
    case ClientFSM::Selftest:
    case ClientFSM::ESP:
    case ClientFSM::ColdPull:
        Screens::Access()->Close();
        break;
    default:
        break;
    }

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

    switch (fsm_type) {
    case ClientFSM::PrintPreview:
        if (ScreenPrintPreview::GetInstance()) {
            ScreenPrintPreview::GetInstance()->Change(data);
        }
        break;
    case ClientFSM::CrashRecovery:
        if (CrashRecovery::GetInstance()) {
            CrashRecovery::GetInstance()->Change(data);
        }
        break;
    case ClientFSM::Selftest:
#if HAS_SELFTEST()
        if (ScreenSelftest::GetInstance()) {
            ScreenSelftest::GetInstance()->Change(data);
        }
#endif // HAS_SELFTEST
        break;
    case ClientFSM::ESP:
        if (ScreenESP::GetInstance()) {
            ScreenESP::GetInstance()->Change(data);
        }
        break;
    case ClientFSM::ColdPull:
#if HAS_COLDPULL()
        if (ScreenColdPull::GetInstance()) {
            ScreenColdPull::GetInstance()->Change(data);
        }
#endif
        break;
    default:
        if (ptr) {
            ptr->Change(data);
        }
    }
}

bool DialogHandler::IsOpen() const {
    return ptr != nullptr;
}

DialogHandler &DialogHandler::Access() {
    static DialogHandler instance;
    return instance;
}

void DialogHandler::Command(std::pair<uint32_t, uint16_t> serialized) {

    fsm::Change change(serialized);
    Access().command_queue.force_push(change);
}

/**
 * @brief determine correct operation with data
 * 3 possibilities: create(open), change(modify), destroy(destroy)
 * can contain close screen + open dialog
 *
 * @param change data containing description of a change, can be even open + close
 */
void DialogHandler::command(fsm::DequeStates changes) {
    log_debug(GUI, "fsm changes from %d, to %d", static_cast<int>(changes.last_sent.get_fsm_type()), static_cast<int>(changes.current.get_fsm_type()));

    // destroy
    // new fsm is ClientFSM::_none, old fsm being ClientFSM::_none should not happen
    if (changes.current.get_fsm_type() == ClientFSM::_none) {
        close(changes.last_sent.get_fsm_type());
        return;
    }

    // create
    // last command was no fsm, new command has fsm
    // or last command was different fsm
    if (changes.current.get_fsm_type() != changes.last_sent.get_fsm_type()) {
        if (changes.last_sent.get_fsm_type() == ClientFSM::_none) {
            // regular open
            open(changes.current.get_fsm_type(), changes.current.get_data());
            Screens::Access()->Loop(); // ensure screen is opened before call of Draw
            // now continue to change
            // open currently does not support change directly
            // TODO make it so .. than Screens::Access()->Loop(); could be removed
        } else {
            // close + open
            close(changes.last_sent.get_fsm_type());
            Screens::Access()->Loop(); // currently it is the simplest way to ensure screen is closed in case close called it
            open(changes.current.get_fsm_type(), changes.current.get_data());
            Screens::Access()->Loop(); // ensure screen is opened before call of Draw
            return;
        }
    }

    // change
    // last and new command fsms are the same
    // no need to check if data changed, queue handles it
    change(changes.current.get_fsm_type(), changes.current.get_data());
}

void DialogHandler::Loop() {
    std::optional<fsm::DequeStates> change = command_queue.dequeue();
    if (!change) {
        return;
    }

    command(*change);
}

bool DialogHandler::IsOpen(ClientFSM fsm) const {
    const ClientFSM q0 = command_queue.GetOpenFsmQ0();
    const ClientFSM q1 = command_queue.GetOpenFsmQ1();
    const ClientFSM q2 = command_queue.GetOpenFsmQ2();

    return fsm == q0 || fsm == q1 || fsm == q2;
}

bool DialogHandler::IsAnyOpen() const {
    const ClientFSM q0 = command_queue.GetOpenFsmQ0();
    return q0 != ClientFSM::_none; // cannot have q1 without q0
}
