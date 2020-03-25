#include "DialogFactory.hpp"

DialogFactory::mem_space DialogFactory::all_dialogs;

static_unique_ptr<IDialogStateful> DialogFactory::load_unload(uint8_t data) {
    static const char *change = "CHANGE FILAMENT";
    static const char *load = "LOAD FILAMENT";
    static const char *unload = "UNLOAD FILAMENT";
    static const char *def = "INDEX ERROR";
    const char *name;
    switch (static_cast<load_unload_type_t>(data)) {
    case DLG_type_change:
        name = change;
        break;
    case DLG_type_load:
        name = load;
        break;
    case DLG_type_unload:
        name = unload;
        break;
    default:
        name = def;
    }
    return make_static_unique_ptr<DialogLoadUnload>(&all_dialogs, name);
}
