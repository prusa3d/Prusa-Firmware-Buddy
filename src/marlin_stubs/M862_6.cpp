/**
 * @file
 */
#include "../common/sound.hpp"
#include "PrusaGcodeSuite.hpp"
#include "../../lib/Marlin/Marlin/src/gcode/parser.h"
#include "gcode_info.hpp"
#include <option/has_indx.h>
#include <option/has_mmu2.h>
#if HAS_MMU2()
    #include <feature/prusa/MMU2/mmu2_mk4.h>
#endif
#include <option/has_ht_hotend.h>
#if HAS_HT_HOTEND()
    #include <config_store/store_instance.hpp>
    #include <hotend_type.hpp>
    #include <tool_index.hpp>
#endif

#ifdef PRINT_CHECKING_Q_CMDS

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M862.6: Check supported features <a href="https://reprap.org/wiki/G-code#M862.6:_Firmware_features">M862.6: Firmware features</a>
 *
 *#### Usage
 *
 *    M [ Q | P "<string>" ]
 *
 *#### Parameters
 *
 * - `Q` - Print currently supported features. Features tied to hardware that can come and
 *         go (the MMU, the HT hotend) are reported only while that hardware is present.
 * - `P "<string>"` - Check for feature
 */
void PrusaGcodeSuite::M862_6() {
    // Handle only Q
    // P is ignored when printing (it is handled before printing by GCodeInfo.*)
    if (!parser.boolval('Q')) {
        return;
    }

    // Pure string composition, nothing to format: emitting the pieces straight to the serial
    // keeps the feature names where they already live and costs no stack buffer.
    const auto report = [](const char *feature) {
        SERIAL_ECHO_START();
        SERIAL_ECHOPGM("  M862.6 P\"");
        SERIAL_ECHO(feature);
        SERIAL_CHAR('"');
        SERIAL_EOL();
    };

    report(gcode_info::m862_6_input_shaper);

    #if HAS_MMU2()
    if (MMU2::mmu2.Enabled()) {
        report(gcode_info::m862_6_mmu);
    }
    #endif

    #if HAS_INDX()
    report(gcode_info::m862_6_indx_lock);
    #endif

    #if HAS_HT_HOTEND()
    static_assert(PhysicalToolIndex::count == 1, "HT hotend is a single-tool printer feature");
    if (config_store().hotend_type.get(0) == HotendType::high_temp) {
        report(gcode_info::m862_6_ht_hotend);
    }
    #endif
}

/** @}*/

#endif
