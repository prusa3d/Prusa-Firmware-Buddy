#include "DialogFactory.hpp"
#include "i18n.h"
#include "bsod.h"
#include "DialogLoadUnload.hpp"

DialogFactory::mem_space DialogFactory::all_dialogs;

//screens .. not used, return nullptr (to pass check in GetAll)
static_unique_ptr<IDialogMarlin> DialogFactory::serial_printing(uint8_t /*data*/) {
    return nullptr;
}
static_unique_ptr<IDialogMarlin> DialogFactory::printing(uint8_t /*data*/) {
    return nullptr;
}
static_unique_ptr<IDialogMarlin> DialogFactory::first_layer(uint8_t /*data*/) {
    return nullptr;
}

static_unique_ptr<IDialogMarlin> DialogFactory::load_unload(uint8_t data) {
    static const char change[] = N_("CHANGE FILAMENT");
    static const char load[] = N_("LOAD FILAMENT");
    static const char unload[] = N_("UNLOAD FILAMENT");
    static const char purge[] = N_("PURGE FILAMENT");
    static const char def[] = "INDEX ERROR"; // intentionally not to be translated
    string_view_utf8 name;
    switch (static_cast<LoadUnloadMode>(data)) {
    case LoadUnloadMode::Change:
        DialogLoadUnload::is_M600_phase = true;
        name = _(change);
        break;
    case LoadUnloadMode::Load:
        name = _(load);
        break;
    case LoadUnloadMode::Unload:
        DialogLoadUnload::is_M600_phase = false;
        name = _(unload);
        break;
    case LoadUnloadMode::Purge:
        name = _(purge);
        break;
    default:
        name = string_view_utf8::MakeCPUFLASH((const uint8_t *)def); //not translated
    }
    return makePtr<DialogLoadUnload>(name);
}

static_unique_ptr<IDialogMarlin> DialogFactory::G162(uint8_t data) {
    static const char *nm = N_("HOME TO MAX");
    string_view_utf8 name = _(nm);
    return makePtr<DialogG162>(name);
}

DialogFactory::Ctors DialogFactory::GetAll() {
    //todo check if all fields are set
    std::array<fnc, size_t(ClientFSM::_count)> ret = { nullptr };
    ret[size_t(ClientFSM::Serial_printing)] = serial_printing;
    ret[size_t(ClientFSM::Printing)] = printing;
    ret[size_t(ClientFSM::FirstLayer)] = first_layer;
    ret[size_t(ClientFSM::Load_unload)] = load_unload;
    ret[size_t(ClientFSM::G162)] = G162;
    ret[size_t(ClientFSM::SelftestAxis)] = [](uint8_t) { return static_unique_ptr<IDialogMarlin>(makePtr<DialogSelftestAxis>()); };
    ret[size_t(ClientFSM::SelftestFans)] = [](uint8_t) { return static_unique_ptr<IDialogMarlin>(makePtr<DialogSelftestFans>()); };
    ret[size_t(ClientFSM::SelftestHeat)] = [](uint8_t) { return static_unique_ptr<IDialogMarlin>(makePtr<DialogSelftestTemp>()); };

    if (std::find(std::begin(ret), std::end(ret), nullptr) != std::end(ret))
        bsod("Error missing dialog Ctor"); // GUI init will throw this

    return ret;
}
