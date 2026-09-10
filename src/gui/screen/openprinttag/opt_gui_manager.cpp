/// @file

#include "opt_gui_manager.hpp"
#include "screen_opt_detected.hpp"
#include "screen_opt_lost.hpp"
#include "screen_opt_flashing.hpp"

#include <ScreenFactory.hpp>
#include <ScreenHandler.hpp>

#include <puppies/cyphal_node_table_host.hpp>
#include <xbuddy_extension/shared_enums.hpp>

#include <option/has_side_leds.h>
#if HAS_SIDE_LEDS()
    #include <leds/side_strip_handler.hpp>
#endif

namespace buddy::openprinttag {

namespace {

    bool any_opt_popup_on_stack() {
        // We cannot use ScreenHandler::IsScreenOnStack because it compares both constructor function and args
        // So ScreenOPTPopup for a different ToolTag would have a different tag and wouldn't match
        return Screens::Access()->any_screen_on_stack([&](const auto &node) {
            return //
                node.creator.func == ScreenFactory::screen_with_arg_creator_f<ScreenOPTDetected, ToolTag>
                || node.creator.func == ScreenFactory::screen_with_arg_creator_f<ScreenOPTLost, VirtualToolIndex>;
        });
    }

} // namespace

void gui_loop_opt() {
    static std::bitset<VirtualToolIndex::count> prev_tags_present;
    static std::bitset<VirtualToolIndex::count> prev_assigned_tags_present;

    for (VirtualToolIndex tool : VirtualToolIndex::all()) {
        const auto ephemeral_tag = ToolTag::for_tool_ephemeral(tool);
        const auto assigned_tag = ToolTag::for_tool_assigned(tool);

        const auto tag_present = ephemeral_tag.has_value();
        const bool prev_tag_present = prev_tags_present[tool.to_raw()];
        prev_tags_present.set(tool.to_raw(), tag_present);

        const bool assigned_tag_present = assigned_tag.has_value() && (assigned_tag == ephemeral_tag);
        const bool prev_assigned_tag_present = prev_assigned_tags_present[tool.to_raw()];
        prev_assigned_tags_present.set(tool.to_raw(), assigned_tag_present);

        if (!tool.is_enabled()) {
            // Keep the bitsets up-to-date even with disabled tools
            continue;
        }

        // Only pop up for a tag that is not assigned to a tool
        if (tag_present && !assigned_tag_present && !prev_tag_present) {
            if (ScreenOPTPrompt::can_popup() && !any_opt_popup_on_stack()) {
                Screens::Access()->Open(ScreenFactory::ScreenWithArg<ScreenOPTDetected, ToolTag>(*ephemeral_tag));
            }

#if HAS_SIDE_LEDS()
            // Consider this an activity
            leds::SideStripHandler::instance().activity_ping();
#endif
        }

        if (!assigned_tag_present && prev_assigned_tag_present) {
            if (ScreenOPTPrompt::can_popup() && !any_opt_popup_on_stack()) {
                Screens::Access()->Open(ScreenFactory::ScreenWithArg<ScreenOPTLost, VirtualToolIndex>(tool));
            }
        }
    }

    // Reader firmware update: pop up once per update
    {
        static bool prev_flashing = false;

        const bool flashing = puppies::cyphal_node_table().get_node_state(xbuddy_extension::NodeName::cz_prusa3d_honeybee_nfc) == xbuddy_extension::NodeState::flash;
        if (flashing && !prev_flashing && ScreenOPTFlashing::can_popup() && !Screens::Access()->IsScreenOnStack<ScreenOPTFlashing>()) {
            Screens::Access()->Open<ScreenOPTFlashing>();
        }
        prev_flashing = flashing;
    }
}

} // namespace buddy::openprinttag
