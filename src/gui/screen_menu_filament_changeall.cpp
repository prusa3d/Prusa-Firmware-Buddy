
#include "screen_menu_filament_changeall.hpp"
#include <filament.hpp>
#include <ScreenHandler.hpp>
#include <img_resources.hpp>
#include <marlin_client.hpp>
#include "DialogHandler.hpp"
#include <config_store/store_instance.hpp>
#include "mmu2_toolchanger_common.hpp"

I_MI_FilamentSelect::I_MI_FilamentSelect(const char *const label, int tool_n)
    : WI_LAMBDA_SPIN(_(label),
        (ftrstd::to_underlying(filament::Type::_last) + (config_store().get_filament_type(tool_n) != filament::Type::NONE ? 2 : 1)), // Allow unload only if something is loaded now
        nullptr, is_enabled_t::yes, is_tool_enabled(tool_n) ? is_hidden_t::no : is_hidden_t::yes,
        0, // Default "Don't change"
        [&](char *buffer) {
            const size_t index = GetIndex();
            if (index == nochange_index) {
                // Don't change
                strncpy(buffer, label_nochange, GuiDefaults::infoDefaultLen);
            } else if (index == unload_index) {
                // Unload
                strncpy(buffer, label_unload, GuiDefaults::infoDefaultLen);
            } else {
                // Print "Change to"/"Load" and filament name to buffer
                snprintf(buffer, GuiDefaults::infoDefaultLen, "%s %s", loaded ? label_change_fil : label_load_fil, filament::get_name(filament::Type(index)));
            }
        })
    , loaded(config_store().get_filament_type(tool_n) != filament::Type::NONE) {
}

MI_FilamentApplyChanges::MI_FilamentApplyChanges()
    : IWindowMenuItem(_(label), &img::arrow_right_10x16, is_enabled_t::yes, is_hidden_t::no) {}

void MI_FilamentApplyChanges::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->WindowEvent(GUI_event_t::CHILD_CLICK, (void *)this);
}

ScreenChangeAllFilaments::ScreenChangeAllFilaments()
    : detail::ScreenChangeAllFilaments(_(label)) {
    EnableLongHoldScreenAction();
    Screens::Access()->DisableMenuTimeout();
}

namespace {
void handle_change_all(const std::array<size_t, ScreenChangeAllFilaments::tool_count> &selection, [[maybe_unused]] const std::array<std::optional<filament::Colour>, ScreenChangeAllFilaments::tool_count> &colors = {}) {
    static constexpr auto tool_count = ScreenChangeAllFilaments::tool_count;

    filament::Type new_filament[tool_count] = {}; // Filled with NONE
    filament::Type old_filament[tool_count] = {};
    bool valid[tool_count] = {}; // Nothing valid
    for (size_t tool = 0; tool < tool_count; tool++) {
        if (!is_tool_enabled(tool)) {
            continue; // Tool not enabled
        }

        if (selection[tool] == I_MI_FilamentSelect::nochange_index) {
            continue; // Ignore "Don't change"
        }

        if (selection[tool] == I_MI_FilamentSelect::unload_index
            && config_store().get_filament_type(tool) == filament::Type::NONE) {
            continue; // Ignore unload when no filament is inserted
        }

        if (selection[tool] == I_MI_FilamentSelect::unload_index) {
            new_filament[tool] = filament::Type::NONE; // Unload
        } else {
            new_filament[tool] = static_cast<filament::Type>(selection[tool]);
        }
        old_filament[tool] = config_store().get_filament_type(tool);
        valid[tool] = true;
    }

#if PRINTER_IS_PRUSA_XL
    // Set all temperatures
    for (size_t tool = 0; tool < tool_count; tool++) {
        if (!valid[tool]) {
            continue;
        }

        uint16_t temperature = max(filament::get_description(new_filament[tool]).nozzle, filament::get_description(old_filament[tool]).nozzle);
        marlin_client::set_target_nozzle(temperature, tool);
        marlin_client::set_display_nozzle(temperature, tool);
    }

    if (std::ranges::any_of(valid, [](auto &elem) { return elem; })) { // if any valid
        // Lift Z to prevent unparking and parking of each tool
        marlin_client::gcode("G27 P0 Z40");

        // Wait for Z to finish
        gui_dlg_wait([] {
            if (!(queue.has_commands_queued() || planner.processing())) {
                Screens::Access()->Close();
            }
        });
    }

    // Do all changes
    for (size_t tool = 0; tool < tool_count; tool++) {
        if (!valid[tool]) {
            continue;
        }

        if (new_filament[tool] == filament::Type::NONE) {
            marlin_client::gcode_printf("M702 T%d W2", tool); // Unload
        } else if (old_filament[tool] == filament::Type::NONE) { // Load
            if (colors[tool].has_value()) {
                marlin_client::gcode_printf("M701 S\"%s\" T%d W2 O%d", filament::get_name(new_filament[tool]), tool,
                    colors[tool].value().to_int());
            } else {
                marlin_client::gcode_printf("M701 S\"%s\" T%d W2", filament::get_name(new_filament[tool]), tool);
            }
        } else { // Change, don't ask for unload
            if (colors[tool].has_value()) {
                marlin_client::gcode_printf("M1600 S\"%s\" T%d R O%d", filament::get_name(new_filament[tool]), tool, colors[tool].value().to_int());
            } else {
                marlin_client::gcode_printf("M1600 S\"%s\" T%d R", filament::get_name(new_filament[tool]), tool);
            }
        }

        /// @note Wait while there are more than one command in queue.
        ///  Keep one G-code in queue at all times to prevent race.
        ///  If the queue is empty for a short moment, the printer seems to be idle and other stuff can happen.
        while (queue.length > 1) {
            gui::TickLoop();
            DialogHandler::Access().Loop();
            gui_loop();
        }
    }

#elif PRINTER_IS_PRUSA_MK4 || PRINTER_IS_PRUSA_MK3_5

    /* MMU2 Reimplementation

        MMU should be in unloaded state right now (as every start of print)
        For all selected tools (in this case tool == MMU's filament slot) change filament
        This means MMU will go through selected tools and load to the slot (NOT load to extruder)
        M704 for filament pre-load does not support color specification
    */

    UNUSED(old_filament);

    for (size_t tool = 0; tool < tool_count; tool++) {
        if (!valid[tool]) { // Unloading is manual on MMU
            continue;
        }

        if (selection[tool] == I_MI_FilamentSelect::unload_index) {
            config_store().set_filament_type(tool, filament::Type::NONE);
            continue;
        }

        marlin_client::gcode_printf("M704 P%d", tool);
        // change filament type in EEPROM;
        config_store().set_filament_type(tool, new_filament[tool]);

        /// @note Wait while there are more than one command in queue.
        ///  Keep one G-code in queue at all times to prevent race.
        ///  If the queue is empty for a short moment, the printer seems to be idle and other stuff can happen.
        while (queue.length > 1) {
            gui::TickLoop();
            DialogHandler::Access().Loop();
            gui_loop();
        }
    }

#endif // PRINTER_IS_PRUSA_XXX

    // Wait for all changes to finish
    while (queue.has_commands_queued() || planner.processing()) {
        gui::TickLoop();
        DialogHandler::Access().Loop();
        gui_loop();
    }
}
} // namespace

void ScreenChangeAllFilaments::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::CHILD_CLICK) {
        // Get all selected filaments
        const std::array<size_t, tool_count> selection { {
            Item<MI_FilamentSelect<0>>().GetIndex(),
            Item<MI_FilamentSelect<1>>().GetIndex(),
            Item<MI_FilamentSelect<2>>().GetIndex(),
            Item<MI_FilamentSelect<3>>().GetIndex(),
            Item<MI_FilamentSelect<4>>().GetIndex(),
        } };
        handle_change_all(selection);
        // Close this menu
        Screens::Access()->Close();
        Screens::Access()->Get()->Validate();
        return;
    }

    SuperWindowEvent(sender, event, param);
}

namespace dialog_change_all_filaments {

detail::DialogEnabledMI::DialogEnabledMI() {
}

void detail::DialogEnabledMI::set_parent(IDialog *new_parent) {
    parent = new_parent;
}

DMI_RETURN::DMI_RETURN()
    : detail::DialogEnabledMI()
    , IWindowMenuItem(_(label), &img::folder_up_16x16, is_enabled_t::yes, is_hidden_t::no) {
    has_return_behavior_ = true;
}

void DMI_RETURN::click(IWindowMenu &) {
    if (parent) {
        parent->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, nullptr);
    }
}

DMI_FilamentApplyChanges::DMI_FilamentApplyChanges()
    : detail::DialogEnabledMI()
    , IWindowMenuItem(_(label), &img::arrow_right_10x16, is_enabled_t::yes, is_hidden_t::no) {}

void DMI_FilamentApplyChanges::click(IWindowMenu & /*window_menu*/) {
    if (parent) {
        parent->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, nullptr);
    }
}

DialogChangeAllFilaments::DialogChangeAllFilaments(const std::array<size_t, I_MI_FilamentSelect::max_I_MI_FilamentSelect_idx + 1> &default_selections, bool exit_on_media_, const std::array<std::optional<filament::Colour>, ScreenChangeAllFilaments::tool_count> &colors_)
    : AddSuperWindow<IDialog>(GuiDefaults::RectScreenNoHeader)
    , exit_on_media(exit_on_media_)
    , colors(colors_)
    , menu(this, GuiDefaults::RectScreenNoHeader, &container)
    , header(this) {

    header.SetText(_(label));
    CaptureNormalWindow(menu);

    // assign this to all menu items  as a parent, so that we can be notified about results
    std::apply([&](auto &...menu_items) {
        (..., [&](auto &menu_item) {
            // this just loops over all menu items, passing each item to this function
            if constexpr (is_any_of<std::remove_cvref_t<decltype(menu_item)>, DMI_RETURN, DMI_FilamentApplyChanges>) {
                menu_item.set_parent(this);
            } else { // MI_FilamentSelect instances only
                menu_item.SetIndex(default_selections[menu_item.filament_select_idx]);
            }
        }(menu_items));
    },
        container.menu_items);
}

void DialogChangeAllFilaments::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    switch (event) {
    case GUI_event_t::MEDIA: {
        const MediaState_t media_state = MediaState_t(reinterpret_cast<int>(param));
        if (media_state == MediaState_t::removed || media_state == MediaState_t::error) {
            // USB was removed
            if (exit_on_media && !exit_on_media_blocked.load()) { // Blocked if filament change screens are open
                Screens::Access()->Close();
            }
            exited_by_media = true; // Mark exit by USB for return of the dialog box
        }
        break;
    }
    case GUI_event_t::CHILD_CLICK: {
        int result = menu.GetIndex() ? *menu.GetIndex() : -1;
        assert(result >= 0); // should never happen
        // expecting return to be 0 and do changes at 6, but for simplicity anything >0 means execute the all change.
        if (result > 0) {
            const std::array<size_t, tool_count> selection { {
                std::get<MI_FilamentSelect<0>>(container.menu_items).GetIndex(),
                std::get<MI_FilamentSelect<1>>(container.menu_items).GetIndex(),
                std::get<MI_FilamentSelect<2>>(container.menu_items).GetIndex(),
                std::get<MI_FilamentSelect<3>>(container.menu_items).GetIndex(),
                std::get<MI_FilamentSelect<4>>(container.menu_items).GetIndex(),
            } };
            exit_on_media_blocked = true; // Block while processing changes
            handle_change_all(selection, colors);
            exit_on_media_blocked = false;
        }
        Screens::Access()->Close();
        break;
    }
    default:
        SuperWindowEvent(sender, event, param);
    }
}
} // namespace dialog_change_all_filaments

bool ChangeAllFilamentsBox(const std::array<size_t, I_MI_FilamentSelect::max_I_MI_FilamentSelect_idx + 1> &default_selections, bool exit_on_media, const std::array<std::optional<filament::Colour>, ScreenChangeAllFilaments::tool_count> &colors) {
    dialog_change_all_filaments::DialogChangeAllFilaments d { default_selections, exit_on_media, colors };
    Screens::Access()->gui_loop_until_dialog_closed();
    return d.was_exited_by_media();
}
