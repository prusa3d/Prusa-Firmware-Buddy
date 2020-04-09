#include "DialogFactory.hpp"

DialogFactory::mem_space DialogFactory::all_dialogs;

static_unique_ptr<IDialogStateful> DialogFactory::serial_printing(uint8_t data) {
    return nullptr; //ClinetFSM::Serial_printing hack it is a screen
}

static_unique_ptr<IDialogStateful> DialogFactory::load_unload(uint8_t data) {
    static const char *change = "CHANGE FILAMENT";
    static const char *load = "LOAD FILAMENT";
    static const char *unload = "UNLOAD FILAMENT";
    static const char *def = "INDEX ERROR";
    const char *name;
    switch (static_cast<LoadUnloadMode>(data)) {
    case LoadUnloadMode::Change:
        name = change;
        break;
    case LoadUnloadMode::Load:
        name = load;
        break;
    case LoadUnloadMode::Unload:
        name = unload;
        break;
    default:
        name = def;
    }
    return make_static_unique_ptr<DialogLoadUnload>(&all_dialogs, name);
}

DialogFactory::Ctors DialogFactory::GetAll() {
    std::array<fnc, size_t(ClinetFSM::_count)> ret = {
        serial_printing,
        load_unload
    };
    return ret;
}
