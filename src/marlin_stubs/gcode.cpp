#include "Marlin/src/gcode/gcode.h"
#include "Marlin/src/gcode/queue.h"

#include "PrusaGcodeSuite.hpp"

#include "M330.h"
#include "M340.h"
#include <stdint.h>
#include <device/board.h>
#include "printers.h"
#include "MarlinPin.h"
#include "metric.h"
#include <option/has_gearbox_alignment.h>
#include <option/has_loadcell.h>
#include <option/has_toolchanger.h>
#include <option/has_side_leds.h>
#include <option/has_leds.h>
#include <option/has_manual_belt_tuning.h>
#include <option/has_door_sensor_calibration.h>
#include <option/has_chamber_vents.h>
#include <option/has_spool_join.h>
#include <option/has_indx.h>
#include <option/has_heaters_selftest_gcode.h>
#include <option/has_wastebin_fill_tracking.h>

#if HAS_LOADCELL()
    #include "loadcell.hpp"
#endif

#if HAS_TRINAMIC
    #include "M919-M920.h"
#endif

#include <option/has_phase_stepping_calibration.h>
#if HAS_PHASE_STEPPING_CALIBRATION()
    #include "M1977.hpp"
#endif

#include <option/has_input_shaper_calibration.h>
#if HAS_INPUT_SHAPER_CALIBRATION()
    #include "M1959.hpp"
#endif

#if HAS_GEARBOX_ALIGNMENT()
    #include <feature/gearbox_alignment/gcode_gearbox_alignment.hpp>
#endif

#include <logging/log.hpp>

LOG_COMPONENT_DEF(PRUSA_GCODE, logging::Severity::info);

static void record_pre_gcode_metrics();

GcodeSuite::VirtualToolFromCommand PrusaGcodeSuite::get_target_virtual_from_command(const GCodeParser2 &p) {
    return GcodeSuite::get_target_virtual_from_optional(p.option<uint8_t>('T'), true);
}
GcodeSuite::VirtualToolFromCommand PrusaGcodeSuite::get_target_virtual_from_command_p(const GCodeParser2 &p) {
    return GcodeSuite::get_target_virtual_from_optional(p.option<uint8_t>('T'), !p.option<bool>('P').value_or(false));
}

GcodeSuite::PhysicalToolFromCommand PrusaGcodeSuite::get_target_physical_from_command(const GCodeParser2 &p) {
    return GcodeSuite::get_target_physical_from_optional(p.option<uint8_t>('T'), true);
}
GcodeSuite::PhysicalToolFromCommand PrusaGcodeSuite::get_target_physical_from_command_p(const GCodeParser2 &p) {
    return GcodeSuite::get_target_physical_from_optional(p.option<uint8_t>('T'), !p.option<bool>('P').value_or(false));
}

[[gnu::noinline]]
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

#if HAS_CHAMBER_FILTRATION_API()
        case 147:
        case 148:
            PrusaGcodeSuite::M147_148();
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
#if PRINTER_IS_PRUSA_iX()
        case 853:
            PrusaGcodeSuite::M853();
            break;
#endif
#if HAS_TOOL_MAPPING()
        case 863:
            PrusaGcodeSuite::M863();
            break;
#endif
#if HAS_SPOOL_JOIN()
        case 864:
            PrusaGcodeSuite::M864();
            break;
#endif
        case 865:
            PrusaGcodeSuite::M865();
            break;

#if HAS_CHAMBER_VENTS()
        case 870:
            PrusaGcodeSuite::M870();
            break;
#endif

#if HAS_TRINAMIC
        case 919:
            PrusaGcodeSuite::M919();
            break;
        case 920:
            PrusaGcodeSuite::M920();
            break;
#endif
#if HAS_MANUAL_BELT_TUNING()
        case 961:
            PrusaGcodeSuite::M961();
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
#if HAS_ESP()
        case 1703:
            PrusaGcodeSuite::M1703();
            break;
#endif

#if HAS_AUTO_RETRACT()
        case 1705:
            PrusaGcodeSuite::M1705();
            break;
#endif

#if HAS_INPUT_SHAPER_CALIBRATION()
        case 1959:
            PrusaGcodeSuite::M1959();
            break;
#endif
#if HAS_PHASE_STEPPING_CALIBRATION()
        case 1977:
            PrusaGcodeSuite::M1977();
            break;
#endif
#if HAS_SELFTEST()
        case 1978:
            PrusaGcodeSuite::M1978();
            break;
#endif
#if HAS_GEARBOX_ALIGNMENT()
        case 1979:
            PrusaGcodeSuite::M1979();
            break;
#endif
#if HAS_DOOR_SENSOR_CALIBRATION()
        case 1980:
            PrusaGcodeSuite::M1980();
            break;
#endif
#if HAS_SELFTEST()
        case 1981:
            PrusaGcodeSuite::M1981();
            break;
#endif
#if HAS_INDX()
        case 1982:
            PrusaGcodeSuite::M1982();
            break;
        case 1983:
            PrusaGcodeSuite::M1983();
            break;
        case 1984:
            PrusaGcodeSuite::M1984();
            break;
#endif
#if HAS_TOOL_OFFSET_SENSOR()
        case 1985:
            PrusaGcodeSuite::M1985();
            break;
#endif
#if HAS_WASTEBIN_FILL_TRACKING()
        case 1986:
            PrusaGcodeSuite::M1986();
            break;
#endif
#if HAS_HEATERS_SELFTEST_GCODE()
        case 1987:
            PrusaGcodeSuite::M1987();
            break;
#endif
        case 9140:
            PrusaGcodeSuite::M9140();
            break;
        case 9141:
            PrusaGcodeSuite::M9141();
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

#if HAS_PRECISE_HOMING_COREXY()
        case 9202:
            PrusaGcodeSuite::M9202();
            break;
#endif

        case 9933:
            PrusaGcodeSuite::M9933();
            break;

#if HAS_CRASH_DETECTION()
        case 9944:
            PrusaGcodeSuite::M9944();
            break;
#endif

#if EXTRUDERS > 1
        case 9934:
            PrusaGcodeSuite::M9934();
            break;
#endif

        default:
            processed = false;
            break;
        }
        break;
    case 'G':
        switch (parser.codenum) {
#if HAS_NOZZLE_CLEANER() || HAS_NOZZLE_CLEANER_LITE()
        case 12:
            PrusaGcodeSuite::G12();
            break;
#endif
        case 26:
            PrusaGcodeSuite::G26();
            break;
        case 123:
            PrusaGcodeSuite::G123();
            break;
#if HAS_SELFTEST()
        case 162:
            PrusaGcodeSuite::G162();
            break;
        case 163:
            PrusaGcodeSuite::G163();
            break;
#endif
#if HAS_TOOL_OFFSET_SENSOR()
        case 427:
            PrusaGcodeSuite::G427();
            break;
#endif
#if HAS_INDX()
        case 750:
            PrusaGcodeSuite::G750();
            break;
#endif
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
