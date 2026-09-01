/// @file
#include "screen_opt_lost.hpp"
#include "screen_opt_detected.hpp"

#include <ScreenHandler.hpp>

namespace buddy::openprinttag {

ScreenOPTLost::ScreenOPTLost(VirtualToolIndex tool)
    : ScreenOPTPrompt { N_("OPENPRINTTAG REMOVED") }
    , frame_ { this, GuiDefaults::RectScreenNoHeader }
    , prompt_ { &frame_ }
    , tool_(tool) {

    {
        const auto filament_name = FilamentType::for_tool(tool).parameters().name;

        prompt_.set_texts(_("OpenPrintTag removed"),
            _("Unload filament \"%.*s\" from tool %i?")
                .formatted(text_params_, std::string_view(filament_name), tool.display_index()));
    }

    prompt_.radio.Change(PhaseResponses { Response::Unload, Response::Ignore });
    prompt_.radio.set_callback([this](Response response) {
        switch (response) {

        case Response::Unload:
            // W2 - return option
            marlin_client::inject(GCodeLiteral { "M702 W2 T%.0f", (float)tool_.to_raw() });
            break;

        case Response::Ignore:
            break;

        default:
            bsod_unreachable();
        }

        // Close screen after any response
        close_screen();
    });
}

void ScreenOPTLost::screenEvent(window_t *sender, GUI_event_t event, void *param) {
    switch (event) {

    case GUI_event_t::LOOP: {
        const auto ephemeral_tag = ToolTag::for_tool_ephemeral(tool_);
        if (ephemeral_tag.has_value()) {
            // Got a tag back -> stop reporting lost
            close_screen();

            if (ephemeral_tag == ToolTag::for_tool_assigned(tool_)) {
                // Tag is already assigned to the tool, be silent

            } else {
                // Turn "tag lost" into "new tag detected"
                Screens::Access()->Open(ScreenFactory::ScreenWithArg<ScreenOPTDetected, ToolTag>(*ephemeral_tag));
            }
        }

        return;
    }

    default:
        break;
    }

    ScreenOPTPrompt::screenEvent(sender, event, param);
}

} // namespace buddy::openprinttag
