#include "DialogFactory.hpp"
#include "i18n.h"
#include "bsod.h"
#include "DialogLoadUnload.hpp"
#include "window_dlg_preheat.hpp"
DialogFactory::mem_space DialogFactory::all_dialogs;

constexpr static const char change[] = N_("CHANGE FILAMENT");
constexpr static const char load[] = N_("LOAD FILAMENT");
constexpr static const char unload[] = N_("UNLOAD FILAMENT");
constexpr static const char purge[] = N_("PURGE FILAMENT");
constexpr static const char load_preheat[] = N_("PREHEAT for LOAD");
constexpr static const char unload_preheat[] = N_("PREHEAT for UNLOAD");
constexpr static const char purge_preheat[] = N_("PREHEAT for PURGE");
constexpr static const char index_error[] = "INDEX ERROR"; // intentionally not to be translated

//screens .. not used, return nullptr (to pass check in GetAll)
static_unique_ptr<IDialogMarlin> DialogFactory::screen_not_dialog(uint8_t /*data*/) {
    return nullptr;
}

static_unique_ptr<IDialogMarlin> DialogFactory::load_unload(uint8_t data) {

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
        name = string_view_utf8::MakeCPUFLASH((const uint8_t *)index_error); //not translated
    }
    return makePtr<DialogLoadUnload>(name);
}

static_unique_ptr<IDialogMarlin> DialogFactory::G162(uint8_t data) {
    static const char *nm = N_("HOME TO MAX");
    string_view_utf8 name = _(nm);
    return makePtr<DialogG162>(name);
}

static_unique_ptr<IDialogMarlin> DialogFactory::Preheat(uint8_t data) {
    PreheatData type(data);
    string_view_utf8 name;
    switch (type.Mode()) {
    case PreheatMode::None:
        name = string_view_utf8::MakeNULLSTR();
        break;
    case PreheatMode::Load:
    case PreheatMode::Autoload:
        name = _(load_preheat);
        break;
    case PreheatMode::Unload:
        name = _(unload_preheat);
        break;
    case PreheatMode::Purge:
        name = _(purge_preheat);
        break;
    case PreheatMode::Change_phase1:
        name = _(unload_preheat); //use unload caption, not a bug
        break;
    case PreheatMode::Change_phase2:
        name = _(load_preheat); //use load caption, not a bug
        break;
    default:
        name = string_view_utf8::MakeCPUFLASH((const uint8_t *)index_error); //not translated
    }

    return makePtr<DialogMenuPreheat>(name, type);
}

DialogFactory::Ctors DialogFactory::GetAll() {
    //todo check if all fields are set
    std::array<fnc, size_t(ClientFSM::_count)> ret = { nullptr };
    ret[size_t(ClientFSM::Serial_printing)] = screen_not_dialog;
    ret[size_t(ClientFSM::Printing)] = screen_not_dialog;
    ret[size_t(ClientFSM::FirstLayer)] = screen_not_dialog;
    ret[size_t(ClientFSM::Load_unload)] = load_unload;
    ret[size_t(ClientFSM::Selftest)] = screen_not_dialog;
    ret[size_t(ClientFSM::G162)] = G162;
    ret[size_t(ClientFSM::Preheat)] = Preheat;
    ret[size_t(ClientFSM::SelftestAxis)] = [](uint8_t) { return static_unique_ptr<IDialogMarlin>(makePtr<DialogSelftestAxis>()); };
    ret[size_t(ClientFSM::SelftestFans)] = [](uint8_t) { return static_unique_ptr<IDialogMarlin>(makePtr<DialogSelftestFans>()); };
    ret[size_t(ClientFSM::SelftestHeat)] = [](uint8_t) { return static_unique_ptr<IDialogMarlin>(makePtr<DialogSelftestTemp>()); };

    if (std::find(std::begin(ret), std::end(ret), nullptr) != std::end(ret))
        bsod("Error missing dialog Ctor"); // GUI init will throw this

    return ret;
}
