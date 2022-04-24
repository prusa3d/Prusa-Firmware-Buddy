// DialogHandler.cpp
#include "DialogHandler.hpp"
#include "DialogLoadUnload.hpp"
#include "DialogFactory.hpp"
#include "IScreenPrinting.hpp"
#include "ScreenHandler.hpp"
#include "screen_printing_serial.hpp"
#include "screen_printing.hpp"
#include "ScreenFirstLayer.hpp"

#if HAS_SELFTEST
    #include "ScreenSelftest.hpp"
#endif

static void OpenPrintScreen(ClientFSM dialog) {
    switch (dialog) {
    case ClientFSM::Serial_printing:
        Screens::Access()->CloseSerial();
        Screens::Access()->Open(ScreenFactory::Screen<screen_printing_serial_data_t>);
        return;
    case ClientFSM::Printing:
        Screens::Access()->CloseAll();
        Screens::Access()->Open(ScreenFactory::Screen<screen_printing_data_t>);
        return;
    case ClientFSM::FirstLayer: //do not close screens
        Screens::Access()->Open(ScreenFactory::Screen<ScreenFirstLayer>);
        return;
    default:
        return;
    }
}

//*****************************************************************************
//method definitions
void DialogHandler::open(fsm::create_t o) {
    if (ptr)
        return; //the dialog is already openned

    const ClientFSM dialog = o.type.GetType();

    ++opened_times[size_t(dialog)];

    //todo get_scr_printing_serial() is no dialog but screen ... change to dialog?
    // only ptr = dialog_creators[dialog](data); should remain
    switch (dialog) {
    case ClientFSM::Serial_printing:
    case ClientFSM::Printing:
    case ClientFSM::FirstLayer:
        if (IScreenPrinting::GetInstance() == nullptr) {
            OpenPrintScreen(dialog);
        } else {
            //openned, notify it
            IScreenPrinting::NotifyMarlinStart();
        }
        break;
    case ClientFSM::Selftest:
#if HAS_SELFTEST
        if (!ScreenSelftest::GetInstance()) {
            //data contain screen caption type
            //ScreenSelftest::SetHeaderMode(...);
            Screens::Access()->Open(ScreenFactory::Screen<ScreenSelftest>);
        }
#endif // HAS_SELFTEST
        break;
    default:
        ptr = dialog_ctors[size_t(dialog)](o.data);
    }
}

void DialogHandler::close(fsm::destroy_t o) {
    const ClientFSM dialog = o.type.GetType();

    ++closed_times[size_t(dialog)];

    if (waiting_closed == dialog) {
        waiting_closed = ClientFSM::_none;
    } else {
        //hack get_scr_printing_serial() is no dialog but screen ... todo change to dialog?

        switch (dialog) {
        case ClientFSM::Serial_printing:
            Screens::Access()->CloseAll();
            break;
        case ClientFSM::FirstLayer:
        case ClientFSM::Selftest:
            Screens::Access()->Close();
            break;
        case ClientFSM::Printing: //closed on button, todo marlin thread should close it
            break;
        default:
            break;
        }
    }

    ptr = nullptr; //destroy current dialog
}

void DialogHandler::change(fsm::change_t o) {
    const ClientFSM dialogType = o.type.GetType();

    switch (dialogType) {
    case ClientFSM::Selftest:
#if HAS_SELFTEST
        if (ScreenSelftest::GetInstance()) {
            ScreenSelftest::GetInstance()->Change(o.data);
        }
#endif // HAS_SELFTEST
        break;
    default:
        if (ptr)
            ptr->Change(o.data);
    }
}

bool DialogHandler::IsOpen() const {
    return ptr != nullptr;
}
//*****************************************************************************
//Meyers singleton
DialogHandler &DialogHandler::Access() {
    static DialogHandler ret(DialogFactory::GetAll());
    return ret;
}

void DialogHandler::Command(uint32_t u32, uint16_t u16) {
    fsm::variant_t variant(u32, u16);

    // not sure about buffering ClientFSM_Command::change, it could work without it
    // but it is buffered too to be simpler
    Access().command_queue.Push(variant);
}

void DialogHandler::command(fsm::variant_t variant) {
    switch (variant.GetCommand()) {
    case ClientFSM_Command::create:
        open(variant.create);
        break;
    case ClientFSM_Command::destroy:
        close(variant.destroy);
        break;
    case ClientFSM_Command::change:
        change(variant.change);
        break;
    default:
        break;
    }
}

void DialogHandler::WaitUntilClosed(ClientFSM dialog, uint8_t data) {
    PreOpen(dialog, data);
    waiting_closed = dialog;
    while (waiting_closed == dialog) {
        gui::TickLoop();
        Loop();
        gui_loop();
    }
}

void DialogHandler::PreOpen(ClientFSM dialog, uint8_t data) {
    const fsm::variant_t var(fsm::create_t(dialog, data));
    Command(var.u32, var.u16);
}

redraw_cmd_t DialogHandler::Loop() {
    fsm::variant_t variant = command_queue.Front();
    // execute 1 command (don't use "while") because
    // screen open only pushes factory method on top of the stack - in this case we would loose folowing change !!!
    // queue merges states, longest possible sequence for not nested fsm is destroy -> craate -> change
    if (variant.GetCommand() == ClientFSM_Command::none)
        return redraw_cmd_t::none;

    command(variant);
    command_queue.Pop(); //erase item from queue

    variant = command_queue.Front();
    if (variant.GetCommand() == ClientFSM_Command::none)
        return redraw_cmd_t::redraw;
    return redraw_cmd_t::skip;
}
