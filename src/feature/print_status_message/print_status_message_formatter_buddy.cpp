#include "print_status_message_formatter_buddy.hpp"

#include <option/has_print_sheet_detection.h>
#include <option/has_spool_join.h>

#include <option/has_toolchanger.h>
#include <option/has_indx.h>
#if HAS_TOOLCHANGER()
    #include <Marlin/src/module/prusa/toolchanger.h>
#endif

#include <option/has_translations.h>

#include <bsod/bsod.h>
#include <utils/string_builder.hpp>

using Message = PrintStatusMessage;

static constexpr EnumArray<Message::Type, const char *, Message::Type::_cnt> messages {
    { Message::Type::none, nullptr },
        { Message::Type::custom, nullptr },
        { Message::Type::homing, N_("Homing") },
        { Message::Type::homing_retrying, ("Homing failed, retrying") },
        { Message::Type::homing_refining, ("Updating precise home point") },
        { Message::Type::recalibrating_home, N_("Recalibrating home. This may take some time.") },
        { Message::Type::calibrating_axis, N_("Calibrating axis") },
        { Message::Type::probing_bed, N_("Probing bed") },
        { Message::Type::additional_probing, N_("Additional probing") },
        { Message::Type::dwelling, N_("Dwelling") },
        { Message::Type::absorbing_heat, N_("Absorbing heat") },
        { Message::Type::waiting_for_hotend_temp, N_("Waiting for hotend") },
        { Message::Type::waiting_for_bed_temp, N_("Waiting for bed") },

#if ENABLED(PROBE_CLEANUP_SUPPORT)
        { Message::Type::nozzle_cleaning, N_("Nozzle cleaning") },
#endif
#if HAS_PRINT_SHEET_DETECTION()
        { Message::Type::detecting_steel_sheet, N_("Detecting steel sheet") },
#endif
#if HAS_SPOOL_JOIN()
        { Message::Type::spool_joined, N_("Spool joined") },
        { Message::Type::joining_spool, N_("Joining spool") },
#endif
#if HAS_CHAMBER_API()
        { Message::Type::waiting_for_chamber_temp, N_("Waiting for chamber") },
#endif
#if HAS_AUTO_RETRACT()
        { Message::Type::auto_retracting, N_("Auto-retracting") },
#endif
#if HAS_CHAMBER_VENTS()
        { Message::Type::opening_chamber_vents, N_("Opening chamber vents") },
        { Message::Type::closing_chamber_vents, N_("Closing chamber vents") },
#endif
#if HAS_TOOL_OFFSET_SENSOR()
        { Message::Type::tool_offset_calibrating, N_("Calibrating tool offsets") },
#endif
#if HAS_NOZZLE_CLEANER()
        { Message::Type::nozzle_cleaner, N_("Nozzle cleaning") },
#endif
#if HAS_NOZZLE_CLEANER_LITE()
        { Message::Type::nozzle_cleaner_lite, N_("Nozzle cleaning") },
#endif
};

void PrintStatusMessageFormatterBuddy::format(StringBuilder &target, const Message &msg) {
    if (auto txt = messages.get_or(msg.type, nullptr)) {
#if HAS_TRANSLATIONS()
        target.append_string_view(_(txt));
#else
        target.append_string(txt);
#endif
    }

    switch (msg.type) {

    case Message::Type::homing:
    case Message::Type::recalibrating_home:
#if HAS_SPOOL_JOIN()
    case Message::Type::spool_joined:
    case Message::Type::joining_spool:
#endif
#if ENABLED(PROBE_CLEANUP_SUPPORT)
    case Message::Type::nozzle_cleaning:
#endif
#if HAS_PRINT_SHEET_DETECTION()
    case Message::Type::detecting_steel_sheet:
#endif
#if HAS_CHAMBER_VENTS()
    case Message::Type::opening_chamber_vents:
    case Message::Type::closing_chamber_vents:
#endif
#if HAS_NOZZLE_CLEANER()
    case Message::Type::nozzle_cleaner:
#endif
#if HAS_NOZZLE_CLEANER_LITE()
    case Message::Type::nozzle_cleaner_lite:
#endif

        // No extra data to show
        break;

    case Message::Type::custom: {
        const auto d = std::get<PrintStatusMessageDataCustom>(msg.data);
        target.append_string(d.message.get());
        break;
    }

    case Message::Type::homing_retrying:
    case Message::Type::homing_refining:
    case Message::Type::calibrating_axis: {
        const auto d = std::get<PrintStatusMessageDataAxisProgress>(msg.data);
        target.append_printf(" (%c)", axis_codes[d.axis]);
        break;
    }

    case Message::Type::probing_bed:
    case Message::Type::additional_probing: {
        const auto d = std::get<PrintStatusMessageDataProgress>(msg.data);
        target.append_printf("\n%i/%i", (int)d.current, (int)d.target);
        break;
    }

    case Message::Type::dwelling: {
        const auto d = std::get<PrintStatusMessageDataProgress>(msg.data);
        const int val = (int)d.current;
        target.append_printf("\n%i:%02i", val / 60, val % 60);
        break;
    }

    case Message::Type::waiting_for_hotend_temp: {
        const auto d = std::get<PrintStatusMessageDataToolProgress>(msg.data);

        target.append_char('\n');
#if HAS_TOOLCHANGER()
        if (prusa_toolchanger.is_toolchanger_enabled()) {
            target.append_printf("T%i ", (int)(d.tool + 1));
        }
#endif
        target.append_printf("%i/%i °C", (int)std::round(d.progress.current), (int)std::round(d.progress.target));
        break;
    }

    case Message::Type::waiting_for_bed_temp:
#if HAS_CHAMBER_API()
    case Message::Type::waiting_for_chamber_temp:
#endif
    {
        const auto d = std::get<PrintStatusMessageDataProgress>(msg.data);
        target.append_printf("\n%i/%i °C", (int)std::round(d.current), (int)std::round(d.target));
        break;
    }

#if HAS_AUTO_RETRACT()
    case Message::Type::auto_retracting:
#endif
    case Message::Type::absorbing_heat: {
        const auto d = std::get<PrintStatusMessageDataProgress>(msg.data);
        target.append_printf("\n%i %%", (int)std::round(d.current));
        break;
    }

#if HAS_TOOL_OFFSET_SENSOR()
    case Message::Type::tool_offset_calibrating: {
        const auto d = std::get<PrintStatusMessageDataToolProgress>(msg.data);
        target.append_printf("\nT%i (%i/%i)", (int)(d.tool + 1), (int)d.progress.current, (int)d.progress.target);
        break;
    }
#endif

    case Message::Type::none:
    case Message::Type::_cnt:
        break;
    }
}
