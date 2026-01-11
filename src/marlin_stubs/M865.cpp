#include "PrusaGcodeSuite.hpp"

#include <option/has_filament_heatbreak_param.h>

#include <filament.hpp>
#include <temperature.hpp>
#include <utils/string_builder.hpp>

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M865: Manage filament parameters
 *
 * Utility G-code that allows managing filament types and their parameters.
 * Allows changing filament parameters and force-setting the currently loaded filament.
 * Also print selected filament parameters to the serial.
 *
 *#### Parameters
 * - `S"<name>"` - Select filament with the specified name
 * - `I<ix>` - Select filament currently loaded to the specified tool (indexed from 0)
 * - `U<ix>` - Select User filament (indexed from 0)
 * - `X` - Select (pending) Custom filament type that will be loaded using `M600 F"#"` (or similar filament change gcode)
 *
 * - `L<ix>` - Set currently loaded filament for the given tool to the selected filament
 *
 * - `R` - Reset parameters not specified in this gcode to defaults
 *
 * - `T<val>` - Set nozzle temperature
 * - `P<val>` - Set nozzle preheat temperature
 * - `B<val>` - Set bed temperature
 * - `H<val>` - Set heatbreak temperature
 * - `A<val>` - Set is abrasive
 * - `G<val>` - Set disable auto retract (1 = no retraction)
 *
 * - `C<val>` - Set target chamber temperature
 * - `D<val>` - Set minimum chamber temperature
 * - `E<val>` - Set maximum chamber temperature
 * - `F<val>` - Set requires filtration
 *
 * - `N"<name>"` - Set name
 *
 */
void PrusaGcodeSuite::M865() {
    GCodeParser2 p;
    if (!p.parse_marlin_command()) {
        return;
    }

    FilamentType filament_type;

    if (const auto slot = p.option<uint8_t>('I', static_cast<uint8_t>(0), static_cast<uint8_t>(EXTRUDERS - 1))) {
        filament_type = config_store().get_filament_type(*slot);

    } else if (p.option<bool>('X').value_or(false)) {
        filament_type = PendingAdHocFilamentType {};

    } else if (const auto slot = p.option<uint8_t>('U', static_cast<uint8_t>(0), static_cast<uint8_t>(user_filament_type_count - 1))) {
        filament_type = UserFilamentType { .index = *slot };

    } else if (const auto ft = p.option<FilamentType>('S')) {
        filament_type = *ft;

    } else {
        SERIAL_ERROR_MSG("Filament type invalid or not specified.");
        return;
    }

    FilamentTypeParameters params = filament_type.parameters();

    if (p.option<bool>('R').value_or(false)) {
        params = {};
    }

    p.store_option('T', params.nozzle_temperature);
    p.store_option('P', params.nozzle_preheat_temperature);
    p.store_option('B', params.heatbed_temperature);

    p.store_option('A', params.is_abrasive);
    p.store_option('G', params.do_not_auto_retract);

#if HAS_FILAMENT_HEATBREAK_PARAM()
    p.store_option('H', params.heatbreak_temperature);
#endif

#if HAS_CHAMBER_API()
    p.store_option('C', params.chamber_target_temperature);
    p.store_option('D', params.chamber_min_temperature);
    p.store_option('E', params.chamber_max_temperature);
    p.store_option('F', params.requires_filtration);
#endif

    std::array<char, filament_name_buffer_size - 1> name_buf;
    if (const auto opt = p.option<std::string_view>('N', name_buf)) {
        StringBuilder b(params.name);
        b.append_std_string_view(*opt);

        if (const auto r = filament_type.can_be_renamed_to(params.name.data()); !r) {
            SERIAL_ERROR_START();
            SERIAL_ECHOLN(r.error());
            return;
        }
    }

    if (filament_type.is_customizable()) {
        filament_type.set_parameters(params);
    }

    if (auto load = p.option<uint8_t>('L', static_cast<uint8_t>(0), static_cast<uint8_t>(EXTRUDERS - 1))) {
        config_store().set_filament_type(*load, filament_type);
    }

    if (filament_type != FilamentType::none) {
        SERIAL_ECHOLNPAIR("name:", params.name.data());
        SERIAL_ECHOLNPAIR("nozzle_temperature:", params.nozzle_temperature);
        SERIAL_ECHOLNPAIR("heatbed_temperature:", params.heatbed_temperature);
        SERIAL_ECHOLNPAIR("is_abrasive:", params.is_abrasive);
#if HAS_CHAMBER_API()
        SERIAL_ECHOLNPAIR("requires_filtration:", params.requires_filtration);
#endif
    }
}

/** @}*/
