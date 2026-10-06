#include "PrusaGcodeSuite.hpp"

#include <option/has_filament_heatbreak_param.h>

#include <common/aggregate_arity.hpp>
#include <filament.hpp>
#include <temperature.hpp>
#include <tool_index.hpp>

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M865/M865.1: Manage filament parameters
 *
 * Utility G-Code that allows managing filament types and their parameters.
 * Allows changing filament parameters and force-setting the currently loaded filament.
 * Also print selected filament parameters to the serial.
 *
 *#### Parameters (Common)
 * - `S"<short_name>"` - Select filament with the specified name
 * - `I<ix>` - Select filament currently loaded to the specified tool (indexed from 0)
 * - `U<ix>` - Select User filament (indexed from 0)
 * - `X` - Select (pending) Custom filament type that will be loaded using `M600 F"#"` (or similar filament change gcode)
 *
 * - `L<ix>` - Set currently loaded filament for the given tool to the selected filament
 *
 * - `R` - Reset ALL parameters not specified in this gcode to defaults
 *
 *#### Parameters (M865)
 *
 *
 * - `T<val>` - Set nozzle temperature
 * - `P<val>` - Set nozzle preheat temperature
 * - `B<val>` - Set bed temperature
 * - `H<val>` - Set heatbreak temperature
 * - `A<val>` - Set is abrasive
 * - `G<val>` - Set is flexible
 *
 * - `C<val>` - Set target chamber temperature
 * - `D<val>` - Set minimum chamber temperature
 * - `E<val>` - Set maximum chamber temperature
 * - `F<val>` - Set requires filtration
 *
 * - `J"<preset>"` - Set base preset (empty "" = no base preset)
 *
 * - `N"<short_name>"` - Set short name
 * - `O"#<hex>"` - Set color
 *
 *#### Parameters (M865.1)
 * - `A"<uuid>"` - Set material UUID
 * - `B"<uuid>"` - Set package UUID
 * - `C"<uuid>"` - Set instance UUID
 */
void PrusaGcodeSuite::M865() {
    GCodeParser2 p;
    if (!p.parse_marlin_command()) {
        return;
    }

    FilamentType filament_type;

    if (const auto slot = p.option<uint8_t, uint8_t, uint8_t>('I', 0, VirtualToolIndex::count - 1)) {
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

    switch (p.command().subcode.value_or(0)) {

    case 0: {

        p.store_option_if_present('T', params.nozzle_temperature);
        p.store_option_if_present('P', params.nozzle_preheat_temperature);
        p.store_option_if_present('B', params.heatbed_temperature);

        p.store_option_if_present('A', params.is_abrasive);
        p.store_option_if_present('G', params.is_flexible);

#if HAS_FILAMENT_HEATBREAK_PARAM()
        p.store_option_if_present('H', params.heatbreak_temperature);
#endif

#if HAS_FILAMENT_BASE_PRESET_PARAM()
        FilamentTypeParameters::Name base_preset_name;
        if (auto str = p.option<std::string_view>('J', base_preset_name)) {
            const auto base_filament_type = FilamentType::from_name(*str);

            if (str->empty()) {
                params.base_preset = std::nullopt;

            } else if (const auto preset = std::get_if<PresetFilamentType>(&base_filament_type)) {
                params.base_preset = *preset;

            } else {
                p.report_option_error('J', "Needs to be a preset filament type");
            }
        }
#endif

        if (auto c = p.option<Color>('O')) {
            params.color = *c;
        }

#if HAS_CHAMBER_API()
        p.store_option_if_present('C', params.chamber_target_temperature);
        p.store_option_if_present('D', params.chamber_min_temperature);
        p.store_option_if_present('E', params.chamber_max_temperature);
#endif
#if HAS_CHAMBER_FILTRATION_API()
        p.store_option_if_present('F', params.requires_filtration);
#endif

        std::array<char, filament_name_buffer_size - 1> name_buf;
        if (const auto opt = p.option<std::string_view>('N', name_buf)) {
            if (const auto r = filament_type.can_be_renamed_to(*opt); !r) {
                SERIAL_ERROR_START();
                SERIAL_ECHOLN(r.error());
                return;
            }
            params.name = *opt;
        }

        break;
    }

    case 1: {
#if HAS_ANFC()
        constexpr auto parse_uuid = [](GCodeParser2 &p2, char key, FilamentTypeParameters::OptionalUUID &target) {
            std::array<char, UUID::string_length> buf;
            std::string_view str;
            if (!p2.store_option_if_present(key, str, buf)) {
                return;
            }

            if (str.empty()) {
                target = std::nullopt;
                return;
            }

            const auto uuid = UUID::from_string(str);
            if (!uuid.has_value()) {
                p2.report_option_error(key, "Invalid UUID");
                return;
            }

            // Silently interpret UUID::nil as nullopt
            target = FilamentTypeParameters::OptionalUUID { *uuid, FilamentTypeParameters::OptionalUUID::NulloptCheck::no };
        };

        parse_uuid(p, 'A', params.material_uuid);
        parse_uuid(p, 'B', params.package_uuid);
        parse_uuid(p, 'C', params.instance_uuid);
#endif
    } break;
    }

    static_assert(
        aggregate_arity<FilamentTypeParameters>()
            == 7
                + HAS_FILAMENT_HEATBREAK_PARAM() * 1
                + HAS_CHAMBER_API() * 3
                + HAS_CHAMBER_FILTRATION_API() * 1
                + HAS_FILAMENT_BASE_PRESET_PARAM() * 1
                + HAS_ANFC() * 4 // OpenPrintTag Hash ID not configurable from the gcode, it's an internal thing
                + HAS_HT_HOTEND() * 1 // requires_ht_idler_door is preset-only, not a gcode parameter
        //
        ,
        "Revise M865 parameters");

    if (filament_type.is_customizable()) {
        filament_type.set_parameters(params);
    }

    /// MUST be after set_parameters. In case of pendingadhoc -> adhoc loading, the data is copied
    if (auto load = p.option<uint8_t, uint8_t, uint8_t>('L', 0, VirtualToolIndex::count - 1)) {
        config_store().set_filament_type(VirtualToolIndex::from_raw(*load), filament_type);
    }

    if (filament_type != FilamentType::none) {
        SERIAL_ECHOLNPAIR("name:", params.name.data());
        SERIAL_ECHOLNPAIR("nozzle_temperature:", params.nozzle_temperature);
        SERIAL_ECHOLNPAIR("heatbed_temperature:", params.heatbed_temperature);
        SERIAL_ECHOLNPAIR("is_abrasive:", params.is_abrasive);
#if HAS_CHAMBER_FILTRATION_API()
        SERIAL_ECHOLNPAIR("requires_filtration:", params.requires_filtration);
#endif
    }
}

/** @}*/
