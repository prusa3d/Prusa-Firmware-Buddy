#include "window_dlg_preheat.hpp"

#include "img_resources.hpp"
#include "marlin_client.hpp"
#include "stdlib.h"
#include "i18n.h"
#include <limits>
#include <filament_gui.hpp>
#include <str_utils.hpp>

using namespace preheat_menu;

// * MI_FILAMENT
MI_FILAMENT::MI_FILAMENT(FilamentType filament_type, uint8_t target_extruder)
    : WiInfo({}, nullptr, is_enabled_t::yes, is_hidden_t::no)
    , filament_type(filament_type)
    , filament_params(filament_type.parameters())
    , target_extruder(target_extruder) //
{
    FilamentTypeGUI::setup_menu_item(filament_type, filament_params, *this);

    ArrayStringBuilder<GetInfoLen()> sb;
    sb.append_printf("%3u/%-3u", filament_params.nozzle_temperature, filament_params.heatbed_temperature);
    ChangeInformation(sb.str());
}

void MI_FILAMENT::click(IWindowMenu &) {
    if (filament_params.is_abrasive && !config_store().nozzle_is_hardened.get().test(target_extruder)) {
        StringViewUtf8Parameters<filament_name_buffer_size + 1> params;
        if (MsgBoxWarning(_("Filament '%s' is abrasive, but you don't have a hardened nozzle installed. Do you really want to continue?").formatted(params, filament_params.name), Responses_YesNo) != Response::Yes) {
            return;
        }
    }

    marlin_client::FSM_response_variant(PhasesPreheat::UserTempSelection, FSMResponseVariant::make<FilamentType>(filament_type));
}

// * MI_RETURN_PREHEAT
MI_RETURN_PREHEAT::MI_RETURN_PREHEAT()
    : IWindowMenuItem(_("Return"), &img::folder_up_16x16, is_enabled_t::yes, is_hidden_t::no) {
    has_return_behavior_ = true;
}

void MI_RETURN_PREHEAT::click(IWindowMenu &window_menu) {
    window_menu.Validate(); /// don't redraw since we leave the menu
    marlin_client::FSM_response(PhasesPreheat::UserTempSelection, Response::Abort);
}

// * MI_SHOW_ALL
MI_SHOW_ALL::MI_SHOW_ALL(WindowMenuPreheat &menu)
    : IWindowMenuItem(_("Show All"))
    , menu(menu) {
}

void MI_SHOW_ALL::click(IWindowMenu &) {
    menu.set_show_all_filaments(true);
}

// * MI_COOLDOWN
MI_COOLDOWN::MI_COOLDOWN()
    : IWindowMenuItem(_(get_response_text(Response::Cooldown)), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_COOLDOWN::click([[maybe_unused]] IWindowMenu &window_menu) {
    marlin_client::FSM_response(PhasesPreheat::UserTempSelection, Response::Cooldown);
}

// * WindowMenuPreheat
WindowMenuPreheat::WindowMenuPreheat(window_t *parent, const Rect16 &rect, const PreheatData &data)
    : WindowMenuVirtual(parent, rect, CloseScreenReturnBehavior::no)
    , preheat_data(data) //
{
    index_mapping.set_item_enabled<Item::return_>(data.has_return_option);
    index_mapping.set_item_enabled<Item::cooldown>(data.has_cooldown_option);
    update_list();
}

void WindowMenuPreheat::set_show_all_filaments(bool set) {
    if (show_all_filaments_ == set) {
        return;
    }

    const auto prev_focused_index = focused_item_index();
    show_all_filaments_ = set;
    update_list();
    move_focus_to_index(prev_focused_index);
}

void WindowMenuPreheat::update_list() {
    const GenerateFilamentListConfig config {
        .visible_only = !show_all_filaments_,
        .visible_first = true,
    };
    const auto count = generate_filament_list(filament_list_storage, config);
    index_mapping.set_section_size<Item::filament_section>(count);
    index_mapping.set_item_enabled<Item::show_all>(!show_all_filaments_);
    setup_items();
}

void WindowMenuPreheat::setup_item(ItemVariant &variant, int index) {
    const auto mapping = index_mapping.from_index(index);
    switch (mapping.item) {

    case Item::return_:
        variant.emplace<MI_RETURN_PREHEAT>();
        break;

    case Item::cooldown:
        variant.emplace<MI_COOLDOWN>();
        break;

    case Item::show_all:
        variant.emplace<MI_SHOW_ALL>(*this);
        break;

    case Item::filament_section:
        variant.emplace<MI_FILAMENT>(filament_list_storage[mapping.pos_in_section], preheat_data.extruder);
        break;
    }
}

void WindowMenuPreheat::screenEvent(window_t *sender, GUI_event_t event, void *param) {
    switch (event) {

    case GUI_event_t::TOUCH_SWIPE_LEFT:
    case GUI_event_t::TOUCH_SWIPE_RIGHT:
        if (index_mapping.is_item_enabled<Item::return_>()) {
            marlin_client::FSM_response(PhasesPreheat::UserTempSelection, Response::Abort);
            return;
        }
        break;

    default:
        break;
    }

    WindowMenuVirtual::screenEvent(sender, event, param);
}

// * DialogMenuPreheat
DialogMenuPreheat::DialogMenuPreheat(fsm::BaseData data)
    : IDialogMarlin(GuiDefaults::RectScreen)
    , menu(this, GuiDefaults::RectScreenNoHeader, PreheatData::deserialize(data.GetData()))
    , header(this) //
{
    if (string_view_utf8 title = get_title(data); !title.isNULLSTR()) {
        header.SetText(title);
    } else {
        SetRect(GuiDefaults::RectScreenNoHeader);
        header.Hide();
    }

    CaptureNormalWindow(menu);
}

string_view_utf8 DialogMenuPreheat::get_title(fsm::BaseData data) {
    switch (PreheatData::deserialize(data.GetData()).mode) {
    case PreheatMode::None:
        return string_view_utf8::MakeNULLSTR();

    case PreheatMode::Load:
    case PreheatMode::Autoload:
    case PreheatMode::Change_phase2: // use load caption, not a bug
        return _("Preheating for load");

    case PreheatMode::Unload:
    case PreheatMode::Change_phase1: // use unload caption, not a bug
        return _("Preheating for unload");

    case PreheatMode::Purge:
        return _("Preheating for purge");

    default:
        return string_view_utf8::MakeCPUFLASH("Index error");
    }
}
