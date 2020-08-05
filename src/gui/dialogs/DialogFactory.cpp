#include "DialogFactory.hpp"
#include "i18n.h"

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
    string_view_utf8 name;
    switch (static_cast<LoadUnloadMode>(data)) {
    case LoadUnloadMode::Change:
        name = string_view_utf8::MakeCPUFLASH((const uint8_t *)change);
        break;
    case LoadUnloadMode::Load:
        name = string_view_utf8::MakeCPUFLASH((const uint8_t *)load);
        break;
    case LoadUnloadMode::Unload:
        name = string_view_utf8::MakeCPUFLASH((const uint8_t *)unload);
        break;
    case LoadUnloadMode::Purge:
        name = string_view_utf8::MakeCPUFLASH((const uint8_t *)purge);
        break;
    default:
        name = string_view_utf8::MakeCPUFLASH((const uint8_t *)def);
    }
    return make_static_unique_ptr<DialogLoadUnload>(&all_dialogs, name);
}

static_unique_ptr<IDialogStateful> DialogFactory::G162(uint8_t data) {
    static const char *nm = N_("HOME TO MAX");
    string_view_utf8 name = string_view_utf8::MakeCPUFLASH((const uint8_t *)nm);
    return make_static_unique_ptr<DialogG162>(&all_dialogs, name);
}

DialogFactory::Ctors DialogFactory::GetAll() {
    //todo check if all fields are set
    std::array<fnc, size_t(ClientFSM::_count)> ret = { nullptr };
    ret[size_t(ClientFSM::Serial_printing)] = serial_printing;
    ret[size_t(ClientFSM::Load_unload)] = load_unload;
    ret[size_t(ClientFSM::G162)] = G162;
    return ret;
}
