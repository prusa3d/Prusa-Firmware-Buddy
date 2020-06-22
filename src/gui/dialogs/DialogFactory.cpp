#include "DialogFactory.hpp"
#include "../lang/i18n.h"

DialogFactory::mem_space DialogFactory::all_dialogs;

static_unique_ptr<IDialogStateful> DialogFactory::serial_printing(uint8_t /*data*/) {
    return nullptr; //ClientFSM::Serial_printing hack it is a screen
}

static_unique_ptr<IDialogStateful> DialogFactory::load_unload(uint8_t data) {
    static const char *change = N_("CHANGE FILAMENT");
    static const char *load = N_("LOAD FILAMENT");
    static const char *unload = N_("UNLOAD FILAMENT");
    static const char *purge = N_("PURGE FILAMENT");
    static const char *def = "INDEX ERROR"; // intentionally not translated
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
    case LoadUnloadMode::Purge:
        name = purge;
        break;
    default:
        name = def;
    }
    return make_static_unique_ptr<DialogLoadUnload>(&all_dialogs, name);
}

DialogFactory::Ctors DialogFactory::GetAll() {
    std::array<fnc, size_t(ClientFSM::_count)> ret = {
        serial_printing,
        load_unload
    };
    return ret;
}
