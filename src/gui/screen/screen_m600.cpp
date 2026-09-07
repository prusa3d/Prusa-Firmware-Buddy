#include "screen_m600.hpp"
#include <general_response.hpp>
#include <window_msgbox.hpp>
#include <ScreenHandler.hpp>
#include <WindowMenuItems.hpp>
#include <screen_menu.hpp>
#include <img_resources.hpp>
#include <timing.h>
#include <config_store/store_instance.hpp>
#include <utils/string_builder.hpp>
#include <gui/menu_item/menu_item_utils.hpp>

#include <option/has_toolchanger.h>
#include <bsod/bsod.h>
#if HAS_TOOLCHANGER()
    #include <Marlin/src/module/prusa/toolchanger.h>
    #include <gui/dialogs/dialog_tool_select.hpp>
#endif

namespace {

bool inject(VirtualToolIndex tool) {
    StringViewUtf8Parameters<8> params;
    const auto msg_answer = MsgBoxQuestion(
        _("Change filament now?\n"
          "Use same filament type as currently loaded.\n"
          "Current filament type: %s")
            .formatted(params, config_store().get_filament_type(tool).parameters().name.data()),
        Responses_YesNo);

    if (msg_answer != Response::Yes) {
        return false;
    }
    marlin_client::inject(GCodeLiteral("M600 P T%.0f", static_cast<float>(tool.to_raw())));
    return true;
}

} // namespace

MI_M600::MI_M600()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no,
#if HAS_TOOLCHANGER()
        (prusa_toolchanger.is_toolchanger_enabled()) ? expands_t::yes :
#endif
                                                     expands_t::no) {
}

void MI_M600::click([[maybe_unused]] IWindowMenu &window_menu) {
#if HAS_TOOLCHANGER()
    if (prusa_toolchanger.is_toolchanger_enabled()) {
        const auto tool = select_tool_dialog({
            .allow_return = true,
        });
        if (tool.has_value() && inject(*tool)) {
            Screens::Access()->Close();
        }
        return;
    }
#endif
    match(
        marlin_vars().active_extruder.get(),
        [](VirtualToolIndex virtual_tool) { inject(virtual_tool); },
        [](NoTool) { debug_assert(false); /* theoretically reachable edge case - do nothing */ });
}

void MI_M600::Loop() {
    loop_gcode_inject_menu_item(*this,
        {
            .update_enabled = true,
            .update_icon = true,
            .enabled = marlin_vars().max_printed_z > 0,
        });
}
