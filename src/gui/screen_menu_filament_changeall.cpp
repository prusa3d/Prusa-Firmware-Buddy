
#include "screen_menu_filament_changeall.hpp"

#include <str_utils.hpp>
#include <algorithm_extensions.hpp>

#include <ScreenHandler.hpp>
#include <img_resources.hpp>
#include <marlin_client.hpp>
#include "DialogHandler.hpp"
#include <option/has_mmu2.h>
#include <config_store/store_instance.hpp>
#include "mmu2_toolchanger_common.hpp"
#include <window_dlg_wait.hpp>
#include "Marlin/src/gcode/queue.h"
#include "Marlin/src/module/planner.h"
#include <str_utils.hpp>
#include <algorithm_extensions.hpp>
#include <filament_list.hpp>

using namespace multi_filament_change;

MI_ActionSelect::MI_ActionSelect(uint8_t tool_ix)
    : WI_LAMBDA_SPIN({}, 1, nullptr, is_enabled_t::yes, is_hidden_t::no, 0, [this](const std::span<char> &buffer) { get_item_text(GetIndex(), buffer); })
    , tool_ix(tool_ix) //
{
    has_filament_loaded = (config_store().get_filament_type(tool_ix) != FilamentType::none);
    set_is_hidden(!is_tool_enabled(tool_ix));
    SetLabel(_(HAS_MMU2() ? N_("Tool %u Filament") : N_("Filament %u")).formatted(label_params, static_cast<unsigned>(tool_ix) + 1));
    index_mapping.set_item_enabled<Action::unload>(has_filament_loaded);
}

void MI_ActionSelect::set_config(const ConfigItem &set) {
    // By using enforce_first_item, we make sure the target filament is in the list (it might be hidden otherwise) and that it's on the first place (which is a welcome bonus)
    const size_t filament_list_size = generate_filament_list(filament_list, { .enforce_first_item = set.new_filament });
    index_mapping.set_section_size<Action::change>(filament_list_size);
    item_count = index_mapping.total_item_count();

    color = set.color;
    SetIndex([&] -> size_t {
        switch (set.action) {
        case Action::keep:
            return index_mapping.to_index<Action::keep>();

        case Action::unload:
            return index_mapping.to_index<Action::unload>();

        case Action::change:
            return index_mapping.to_index<Action::change>(std::find(filament_list.begin(), filament_list.begin() + filament_list_size, set.new_filament) - filament_list.begin());
        }

        std::abort();
    }());
}

ConfigItem MI_ActionSelect::config() const {
    const auto mapping = index_mapping.from_index(GetIndex());
    return ConfigItem {
        .action = mapping.item,
        .new_filament = (mapping.item == Action::change) ? filament_list[mapping.pos_in_section] : FilamentType::none,
        .color = color
    };
}

void MI_ActionSelect::get_item_text(size_t index, std::span<char> buffer) const {
    const auto mapping = index_mapping.from_index(index);
    switch (mapping.item) {

    case Action::keep:
        _("Don't change").copyToRAM(buffer);
        break;

    case Action::unload:
        _("Unload").copyToRAM(buffer);
        break;

    case Action::change: {
        StringBuilder sb(buffer);
        sb.append_string_view(_(has_filament_loaded ? N_("Change to") : N_("Load")));
        sb.append_char(' ');
        sb.append_string(filament_list[mapping.pos_in_section].parameters().name);
        break;
    }
    }
}

MI_ApplyChanges::MI_ApplyChanges()
    : IWindowMenuItem(_("Carry Out the Changes"), &img::arrow_right_10x16, is_enabled_t::yes, is_hidden_t::no) {}

void MI_ApplyChanges::click(IWindowMenu &menu) {
    menu.WindowEvent(&menu, GUI_event_t::CHILD_CLICK, nullptr);
}

MenuMultiFilamentChange::MenuMultiFilamentChange(window_t *parent, const Rect16 &rect)
    : WindowMenu(parent, rect) {
    BindContainer(container);
}

void MenuMultiFilamentChange::set_configuration(const MultiFilamentChangeConfig &set) {
    // Set the correct indexes for the actions
    stdext::visit_sequence<tool_count>([&]<size_t ix>() {
        container.Item<WithConstructorArgs<MI_ActionSelect, ix>>().set_config(set[ix]);
    });
}

void MenuMultiFilamentChange::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::CHILD_CLICK) {
        carry_out_changes();
        Screens::Access()->Close();
        return;
    }

    WindowMenu::windowEvent(sender, event, param);
}

void MenuMultiFilamentChange::carry_out_changes() {
    struct ToolConfig : public ConfigItem {
        FilamentType old_filament = FilamentType::none;
    };
    std::array<ToolConfig, tool_count> tool_config = [&]<size_t... ix>(std::index_sequence<ix...>) {
        return std::array<ToolConfig, tool_count> {
            ToolConfig { container.Item<WithConstructorArgs<MI_ActionSelect, ix>>().config(), config_store().get_filament_type(ix) }...
        };
    }(std::make_index_sequence<tool_count>());

    // Validate the inputs
    for (size_t tool = 0; tool < tool_count; tool++) {
        auto &config = tool_config[tool];

        if (!is_tool_enabled(tool)) {
            config.action = Action::keep;
            continue;
        }

        if (config.action == Action::unload && config_store().get_filament_type(tool) == FilamentType::none) {
            config.action = Action::keep;
            continue;
        }
    }

    // If we have nothing to do, we can exit right now
    if (std::all_of(tool_config.begin(), tool_config.end(), [](const auto &i) { return i.action == Action::keep; })) {
        return;
    }

#if HAS_TOOLCHANGER()
    // Set all temperatures
    for (size_t tool = 0; tool < tool_count; tool++) {
        const auto &config = tool_config[tool];
        if (config.action == Action::keep) {
            continue;
        }

        const uint16_t temperature = max(config.new_filament.parameters().nozzle_temperature, config.old_filament.parameters().nozzle_temperature);
        marlin_client::set_target_nozzle(temperature, tool);
        marlin_client::set_display_nozzle(temperature, tool);
    }

    // Lift Z to prevent unparking and parking of each tool
    marlin_client::gcode("G27 P0 Z40");

    // Wait for Z to finish
    gui_dlg_wait([] {
        if (!(queue.has_commands_queued() || planner.processing())) {
            Screens::Access()->Close();
        }
    });
#endif

    /* MMU2 Reimplementation

        MMU should be in unloaded state right now (as every start of print)
        For all selected tools (in this case tool == MMU's filament slot) change filament
        This means MMU will go through selected tools and load to the slot (NOT load to extruder)
        M704 for filament pre-load does not support color specification
    */

    // Carry out the changes
    for (size_t tool = 0; tool < tool_count; tool++) {
        const auto &config = tool_config[tool];
        switch (config.action) {

        case Action::keep:
            break;

        case Action::unload:
#if HAS_MMU2()
            config_store().set_filament_type(tool, FilamentType::none);
#else
            marlin_client::gcode_printf("M702 T%d W2", tool);
#endif
            break;

        case Action::change: {
#if HAS_MMU2()
            marlin_client::gcode_printf("M704 P%d", tool);
            config_store().set_filament_type(tool, config.new_filament);
#else
            ArrayStringBuilder<MAX_CMD_SIZE> command_builder;

            // M1600 - filament change (doesn't ask for unload)
            // M701 - filament load
            command_builder.append_printf((config.old_filament != FilamentType::none) ? "M1600 S\"%s\" T%d R" : "M701 S\"%s\" T%d W2", config.new_filament.parameters().name, tool);

            if (config.color.has_value()) {
                command_builder.append_printf(" O%" PRIu32, config.color->raw);
            }

            assert(command_builder.is_ok());
            marlin_client::gcode(command_builder.str());
#endif
            break;
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

    // Wait for all changes to finish
    while (queue.has_commands_queued() || planner.processing()) {
        gui::TickLoop();
        DialogHandler::Access().Loop();
        gui_loop();
    }
}

static constexpr const char *header_text = HAS_MMU2() ? N_("FILAMENT CHANGE") : N_("MULTITOOL FILAMENT CHANGE");

ScreenChangeAllFilaments::ScreenChangeAllFilaments()
    : ScreenMenuBase(nullptr, _(header_text), EFooter::On) //
{
    EnableLongHoldScreenAction();
    Screens::Access()->DisableMenuTimeout();
    menu.menu.set_configuration({});
}

bool DialogChangeAllFilaments::exec(const MultiFilamentChangeConfig &initial_config, bool exit_on_media) {
    DialogChangeAllFilaments dlg(initial_config);
    dlg.exit_on_media = exit_on_media;
    Screens::Access()->gui_loop_until_dialog_closed();
    return dlg.exited_by_media;
}

DialogChangeAllFilaments::DialogChangeAllFilaments(const MultiFilamentChangeConfig &initial_configuration)
    : IDialog(GuiDefaults::RectScreenNoHeader)
    , header(this, _(header_text))
    , menu(this, GuiDefaults::RectScreenNoHeader) //
{
    CaptureNormalWindow(menu);
    menu.menu.set_configuration(initial_configuration);
}

void DialogChangeAllFilaments::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::MEDIA) {
        const MediaState_t media_state = MediaState_t(reinterpret_cast<int>(param));
        if (media_state == MediaState_t::removed || media_state == MediaState_t::error) {
            // USB was removed
            if (exit_on_media && !menu.menu.is_carrying_out_changes()) { // Blocked if filament change screens are open
                Screens::Access()->Close();
            }
            exited_by_media = true; // Mark exit by USB for return of the dialog box
        }
    }

    IDialog::windowEvent(sender, event, param);
}
