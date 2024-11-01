#include "PrusaGcodeSuite.hpp"
#include <filament.hpp>
#include <temperature.hpp>

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M865: Configure ad-hoc filament
 *
 *#### Parameters
 * - `I` - Ad-hoc filament slot (indexed from 0)
 * - `T` - Nozzle temperature
 * - `P` - Nozzle preheat temperature
 * - `B` - Bed temperature
 * - `A` - Is abrasive
 * - `F` - Requries filtration
 * - `N"<string>"` - Filament name
 *
 * Ad-hoc/custom filaments can the be referenced in other gcodes using adhoc_filament_gcode_prefix.
 * For example `M600 S"#0"` will load ad-hoc filament previously set with `M865 I0`.
 */
void PrusaGcodeSuite::M865() {
    GCodeParser2 p;
    if (!p.parse_marlin_command()) {
        return;
    }

    const auto slot = p.option<uint8_t>('I', static_cast<uint8_t>(0), static_cast<uint8_t>(adhoc_filament_type_count - 1));
    if (!slot) {
        return;
    }

    const FilamentType filament_type = AdHocFilamentType { .tool = *slot };

    FilamentTypeParameters params = filament_type.parameters();

    // We cannot use store_option here because FilamentTypeParameters is packed :(
    if (const auto opt = p.option<decltype(FilamentTypeParameters::nozzle_temperature)>('T')) {
        params.nozzle_temperature = *opt;
    }
    if (const auto opt = p.option<decltype(FilamentTypeParameters::nozzle_preheat_temperature)>('P')) {
        params.nozzle_preheat_temperature = *opt;
    }
    if (const auto opt = p.option<decltype(FilamentTypeParameters::heatbed_temperature)>('B')) {
        params.heatbed_temperature = *opt;
    }

    if (const auto opt = p.option<bool>('A')) {
        params.is_abrasive = *opt;
    }
    if (const auto opt = p.option<bool>('F')) {
        params.requires_filtration = *opt;
    }

    std::array<char, filament_name_buffer_size - 1> name_buf;
    if (const auto opt = p.option<std::string_view>('N', name_buf)) {
        StringBuilder b = StringBuilder::from_ptr(params.name, filament_name_buffer_size);
        b.append_std_string_view(*opt);
    }

    filament_type.modify_parameters([&params](auto &target) {
        target = params;
    });
}

/** @}*/
