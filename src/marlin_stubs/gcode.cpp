#include "Marlin/src/gcode/gcode.h"
#include "Marlin/src/gcode/queue.h"

#include "PrusaGcodeSuite.hpp"

#include "M330.h"
#include "M340.h"
#include "M919-M920.h"
#include "config_buddy_2209_02.h"
#include <device/board.h>
#include "metric.h"
#include <option/has_loadcell.h>
#include <option/has_toolchanger.h>
#include <option/has_side_leds.h>
#include <option/has_leds.h>

#if HAS_LOADCELL()
    #include "loadcell.hpp"
#endif

#include "log.h"

LOG_COMPONENT_DEF(PRUSA_GCODE, LOG_SEVERITY_INFO);

static void record_pre_gcode_metrics();

bool GcodeSuite::process_parsed_command_custom(bool no_ok) {
    record_pre_gcode_metrics();
    bool processed = true;

    switch (parser.command_letter) {
    case 'M':
        switch (parser.codenum) {
        case 0:
            PrusaGcodeSuite::M0();
            break;
        case 50:
            PrusaGcodeSuite::M50(); // selftest
            break;
        case 123:
            PrusaGcodeSuite::M123();
            break;
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
        case 300:
            PrusaGcodeSuite::M300();
            break;
        case 330:
            PrusaGcodeSuite::M330();
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
        case 930:
            PrusaGcodeSuite::M930();
            break;
        case 931:
            PrusaGcodeSuite::M931();
            break;
        case 932:
            PrusaGcodeSuite::M932();
            break;
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
        case 1587:
            PrusaGcodeSuite::M1587();
            break;
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
        default:
            processed = false;
            break;
        }
        break;
    case 'G':
        switch (parser.codenum) {
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

// weak g-codes to prevent ugly preprocessor
void __attribute__((weak)) PrusaGcodeSuite::M0() { log_error(PRUSA_GCODE, "M0 unsupported"); }
void __attribute__((weak)) PrusaGcodeSuite::M50() { log_error(PRUSA_GCODE, "M50 unsupported"); }
void __attribute__((weak)) PrusaGcodeSuite::M704() { log_error(PRUSA_GCODE, "M704 unsupported"); }
void __attribute__((weak)) PrusaGcodeSuite::M1704() { log_error(PRUSA_GCODE, "M1704 unsupported"); }
void __attribute__((weak)) PrusaGcodeSuite::M705() { log_error(PRUSA_GCODE, "M705 unsupported"); }
void __attribute__((weak)) PrusaGcodeSuite::M706() { log_error(PRUSA_GCODE, "M706 unsupported"); }
void __attribute__((weak)) PrusaGcodeSuite::M707() { log_error(PRUSA_GCODE, "M707 unsupported"); }
void __attribute__((weak)) PrusaGcodeSuite::M708() { log_error(PRUSA_GCODE, "M708 unsupported"); }
void __attribute__((weak)) PrusaGcodeSuite::M709() { log_error(PRUSA_GCODE, "M709 unsupported"); }
void __attribute__((weak)) PrusaGcodeSuite::M930() { log_error(PRUSA_GCODE, "M930 unsupported"); }
void __attribute__((weak)) PrusaGcodeSuite::M931() { log_error(PRUSA_GCODE, "M931 unsupported"); }
void __attribute__((weak)) PrusaGcodeSuite::M932() { log_error(PRUSA_GCODE, "M932 unsupported"); }
void __attribute__((weak)) PrusaGcodeSuite::M999() { log_error(PRUSA_GCODE, "M999 unsupported"); }
void __attribute__((weak)) PrusaGcodeSuite::M1587() { log_error(PRUSA_GCODE, "M1587 unsupported"); }
void __attribute__((weak)) PrusaGcodeSuite::M1600() { log_error(PRUSA_GCODE, "M1600 unsupported"); }
void __attribute__((weak)) PrusaGcodeSuite::M1601() { log_error(PRUSA_GCODE, "M1601 unsupported"); }
void __attribute__((weak)) PrusaGcodeSuite::M1700() { log_error(PRUSA_GCODE, "M1700 unsupported"); }
void __attribute__((weak)) PrusaGcodeSuite::M1701() { log_error(PRUSA_GCODE, "M1701 unsupported"); }
void __attribute__((weak)) PrusaGcodeSuite::G64() { log_error(PRUSA_GCODE, "G64 unsupported"); }
void __attribute__((weak)) PrusaGcodeSuite::G162() { log_error(PRUSA_GCODE, "G162 unsupported"); }
void __attribute__((weak)) PrusaGcodeSuite::G163() { log_error(PRUSA_GCODE, "G163 unsupported"); }

static void record_pre_gcode_metrics() {
    static metric_t gcode = METRIC("gcode", METRIC_VALUE_STRING, 0, METRIC_HANDLER_DISABLE_ALL);
    metric_record_string(&gcode, "%s", parser.command_ptr);

#if HAS_LOADCELL()
    static metric_t loadcell_scale_m = METRIC("loadcell_scale", METRIC_VALUE_FLOAT, 5000, METRIC_HANDLER_ENABLE_ALL);
    static metric_t loadcell_threshold_static_m = METRIC("loadcell_threshold", METRIC_VALUE_FLOAT, 5005, METRIC_HANDLER_ENABLE_ALL);
    static metric_t loadcell_threshold_continuous_m = METRIC("loadcell_threshold_cont", METRIC_VALUE_FLOAT, 5010, METRIC_HANDLER_ENABLE_ALL);
    static metric_t loadcell_hysteresis_m = METRIC("loadcell_hysteresis", METRIC_VALUE_FLOAT, 5015, METRIC_HANDLER_ENABLE_ALL);
    metric_register(&loadcell_scale_m);
    metric_register(&loadcell_threshold_static_m);
    metric_register(&loadcell_threshold_continuous_m);
    metric_register(&loadcell_hysteresis_m);

    metric_record_float(&loadcell_scale_m, loadcell.GetScale());
    metric_record_float(&loadcell_threshold_static_m, loadcell.GetThreshold(Loadcell::TareMode::Static));
    metric_record_float(&loadcell_threshold_continuous_m, loadcell.GetThreshold(Loadcell::TareMode::Continuous));
    metric_record_float(&loadcell_hysteresis_m, loadcell.GetHysteresis());

#endif
}
