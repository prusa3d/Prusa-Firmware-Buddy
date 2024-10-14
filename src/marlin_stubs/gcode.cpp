#include "Marlin/src/gcode/gcode.h"
#include "Marlin/src/gcode/queue.h"

#include "PrusaGcodeSuite.hpp"

#include "M330.h"
#include "M340.h"
#include "M919-M920.h"
#include <stdint.h>
#include <device/board.h>
#include "printers.h"
#include "MarlinPin.h"
#include "metric.h"
#include <option/has_loadcell.h>
#include <option/has_toolchanger.h>
#include <option/has_side_leds.h>
#include <option/has_leds.h>
#include <option/has_phase_stepping.h>
#include <option/has_input_shaper_calibration.h>
#include <option/has_belt_tuning.h>

#if HAS_LOADCELL()
    #include "loadcell.hpp"
#endif

#if HAS_PHASE_STEPPING()
    #include "M1977.hpp"
#endif

#if HAS_INPUT_SHAPER_CALIBRATION()
    #include "M1959.hpp"
#endif

#include <logging/log.hpp>

LOG_COMPONENT_DEF(PRUSA_GCODE, logging::Severity::info);

static void record_pre_gcode_metrics();

FilamentType PrusaGcodeSuite::get_filament_type_from_command(char parameter, const char **string_begin_ptr) {
    if (!parser.seen(parameter)) {
        return FilamentType::none;
    }

    const char *text_begin = strchr(parser.string_arg, '"');
    if (!text_begin) {
        return FilamentType::none;
    }

    text_begin++; // move pointer from '"' to first letter

    const char *text_end = strchr(text_begin, '"');
    if (!text_end) {
        return FilamentType::none;
    }

    if (string_begin_ptr) {
        *string_begin_ptr = text_begin;
    }

    return FilamentType::from_name(std::string_view(text_begin, text_end));
}

int8_t PrusaGcodeSuite::get_target_extruder_from_command(const GCodeParser2 &p) {
    return GcodeSuite::get_target_extruder_from_option_value(p.option<uint8_t>('T'));
}

bool GcodeSuite::process_parsed_command_custom(bool no_ok) {
    record_pre_gcode_metrics();
    bool processed = true;

    switch (parser.command_letter) {
    case 'M':
        switch (parser.codenum) {
        case 0:
            PrusaGcodeSuite::M0();
            break;

        case 104:
            switch (parser.subcode) {

            case 1:
                PrusaGcodeSuite::M104_1();
                break;

            default:
                processed = false;
                break;
            }
            break;

        case 123:
            PrusaGcodeSuite::M123();
            break;

#if HAS_CHAMBER_API()
        case 141:
            PrusaGcodeSuite::M141();
            break;
#endif

#if HAS_LEDS()
        case 150:
            PrusaGcodeSuite::M150();
            break;
#endif
#if HAS_SIDE_LEDS()
        case 151:
            PrusaGcodeSuite::M151();
            break;
#endif

#if HAS_CHAMBER_API()
        case 191:
            PrusaGcodeSuite::M191();
            break;
#endif

#if HAS_I2C_EXPANDER()
        case 262:
            PrusaGcodeSuite::M262();
            break;
        case 263:
            PrusaGcodeSuite::M263();
            break;
        case 264:
            PrusaGcodeSuite::M264();
            break;
        case 265:
            PrusaGcodeSuite::M265();
            break;
        case 267:
            PrusaGcodeSuite::M267();
            break;
        case 268:
            PrusaGcodeSuite::M268();
            break;
#endif // HAS_I2C_EXPANDER()
        case 300:
            PrusaGcodeSuite::M300();
            break;
        case 330:
            // Metrics handler selection deprecated. We only really have one handler. Let's not pretend otherwise.
            // BFW-5464
            break;
        case 331:
            PrusaGcodeSuite::M331();
            break;
        case 332:
            PrusaGcodeSuite::M332();
            break;
        case 333:
            PrusaGcodeSuite::M333();
            break;
        case 334:
            PrusaGcodeSuite::M334();
            break;
        case 340:
            PrusaGcodeSuite::M340();
            break;
        // case 505: // deprecated
        //     PrusaGcodeSuite::M505();
        //     break;
        case 591:
            PrusaGcodeSuite::M591();
            break;
        case 704:
            PrusaGcodeSuite::M704();
            break;
        case 1704:
            PrusaGcodeSuite::M1704();
            break;
        case 705:
            PrusaGcodeSuite::M705();
            break;
        case 706:
            PrusaGcodeSuite::M706();
            break;
        case 707:
            PrusaGcodeSuite::M707();
            break;
        case 708:
            PrusaGcodeSuite::M708();
            break;
        case 709:
            PrusaGcodeSuite::M709();
            break;
#ifdef PRINT_CHECKING_Q_CMDS
        case 862:
            switch (parser.subcode) {
            case 1:
                PrusaGcodeSuite::M862_1();
                break;
            case 2:
                PrusaGcodeSuite::M862_2();
                break;
            case 3:
                PrusaGcodeSuite::M862_3();
                break;
            case 4:
                PrusaGcodeSuite::M862_4();
                break;
            case 5:
                PrusaGcodeSuite::M862_5();
                break;
            case 6:
                PrusaGcodeSuite::M862_6();
                break;
            default:
                processed = false;
            }
            break;
#endif

#if ENABLED(PRUSA_TOOL_MAPPING)
        case 863:
            PrusaGcodeSuite::M863();
            break;
#endif
#if ENABLED(PRUSA_SPOOL_JOIN)
        case 864:
            PrusaGcodeSuite::M864();
            break;
#endif
        case 919:
            PrusaGcodeSuite::M919();
            break;
        case 920:
            PrusaGcodeSuite::M920();
            break;
#if HAS_BELT_TUNING()
        case 960:
            PrusaGcodeSuite::M960();
            break;
#endif
        case 997:
            PrusaGcodeSuite::M997();
            break;
        case 999:
            if (parser.seen('R')) {
                PrusaGcodeSuite::M999();
                break;
            } else {
                processed = false;
                break;
            }
#if BUDDY_ENABLE_CONNECT()
        case 1200:
            PrusaGcodeSuite::M1200();
            break;
#endif // BUDDY_ENABLE_CONNECT()
        case 1600:
            PrusaGcodeSuite::M1600();
            break;
        case 1601:
            PrusaGcodeSuite::M1601();
            break;

        case 1700:
            PrusaGcodeSuite::M1700();
            break;
        case 1701:
            PrusaGcodeSuite::M1701();
            break;
        case 1702:
            PrusaGcodeSuite::M1702();
            break;
        case 1703:
            PrusaGcodeSuite::M1703();
            break;

#if HAS_INPUT_SHAPER_CALIBRATION()
        case 1959:
            PrusaGcodeSuite::M1959();
            break;
#endif

#if HAS_PHASE_STEPPING()
        case 1977:
            PrusaGcodeSuite::M1977();
            break;
#endif

        case 9140:
            PrusaGcodeSuite::M9140();
            break;
        case 9150:
            PrusaGcodeSuite::M9150();
            break;

        case 9200:
            PrusaGcodeSuite::M9200();
            break;

        case 9201:
            PrusaGcodeSuite::M9201();
            break;

        default:
            processed = false;
            break;
        }
        break;
    case 'G':
        switch (parser.codenum) {
#if HAS_NOZZLE_CLEANER()
        case 12:
            PrusaGcodeSuite::G12();
            break;
#endif
        case 26:
            PrusaGcodeSuite::G26();
            break;
        case 64:
            PrusaGcodeSuite::G64();
            break;
        case 162:
            PrusaGcodeSuite::G162();
            break;
        case 163:
            PrusaGcodeSuite::G163();
            break;
        default:
            processed = false;
            break;
        }
        break;
#if HAS_TOOLCHANGER()
    case 'P':
        switch (parser.codenum) {
        case 0:
            PrusaGcodeSuite::P0();
            break;
        default:
            processed = false;
            break;
        }
        break;
#endif
    default:
        processed = false;
        break;
    }

    if (processed && !no_ok) {
        queue.ok_to_send();
    }

    return processed;
}

static void record_pre_gcode_metrics() {
    METRIC_DEF(gcode, "gcode", METRIC_VALUE_STRING, 0, METRIC_DISABLED);
    metric_record_string(&gcode, "%s", parser.command_ptr);

#if HAS_LOADCELL()
    METRIC_DEF(loadcell_scale_m, "loadcell_scale", METRIC_VALUE_FLOAT, 5000, METRIC_ENABLED);
    METRIC_DEF(loadcell_threshold_static_m, "loadcell_threshold", METRIC_VALUE_FLOAT, 5005, METRIC_ENABLED);
    METRIC_DEF(loadcell_threshold_continuous_m, "loadcell_threshold_cont", METRIC_VALUE_FLOAT, 5010, METRIC_ENABLED);
    METRIC_DEF(loadcell_hysteresis_m, "loadcell_hysteresis", METRIC_VALUE_FLOAT, 5015, METRIC_ENABLED);

    metric_record_float(&loadcell_scale_m, loadcell.GetScale());
    metric_record_float(&loadcell_threshold_static_m, loadcell.GetThreshold(Loadcell::TareMode::Static));
    metric_record_float(&loadcell_threshold_continuous_m, loadcell.GetThreshold(Loadcell::TareMode::Continuous));
    metric_record_float(&loadcell_hysteresis_m, loadcell.GetHysteresis());

#endif
}
