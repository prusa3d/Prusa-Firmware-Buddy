#include "filament_variant_decode.hpp"

#include <encoded_filament.hpp>

namespace config_store_ns::migrations {

FilamentType filament_type_from_variant_bytes(uint8_t discriminant, uint8_t value) {
    switch (discriminant) {
    case 0: // NoFilamentType
        return NoFilamentType {};
    case 1: // PresetFilamentType
        // Employ the bounds and validity checking from EncodedFilamentType
        return FilamentType::from_optional(EncodedFilamentType::preset_filament_type_from_enum_value(value));
    case 2: // UserFilamentType
        return UserFilamentType { value };
    case 3: // AdHocFilamentType
        if (value >= VirtualToolIndex::count) {
            return NoFilamentType {};
        }
        return AdHocFilamentType { VirtualToolIndex::from_raw(value) };
    default: // PendingAdHocFilamentType or unknown
        return NoFilamentType {};
    }
}

} // namespace config_store_ns::migrations
