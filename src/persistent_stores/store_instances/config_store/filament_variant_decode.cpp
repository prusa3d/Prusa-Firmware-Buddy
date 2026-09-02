#include "filament_variant_decode.hpp"

namespace config_store_ns::migrations {

FilamentType filament_type_from_variant_bytes(uint8_t discriminant, uint8_t value) {
    switch (discriminant) {
    case 0: // NoFilamentType
        return NoFilamentType {};
    case 1: // PresetFilamentType
        return static_cast<PresetFilamentType>(value);
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
