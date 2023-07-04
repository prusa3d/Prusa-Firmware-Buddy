#pragma once

#include <eeprom.h>
#include <module/temperature.h>

// TODO: Find a better home for this
inline bool operator==(DockPosition lhs, DockPosition rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

// TODO: Find a better home for this
inline bool operator==(ToolOffset lhs, ToolOffset rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

// TODO: Find a better home for this
inline bool operator==(Sheet lhs, Sheet rhs) {
    for (size_t i = 0; i < std::size(lhs.name); ++i) {
        if (lhs.name[i] != rhs.name[i]) {
            return false;
        }
        if (lhs.name[i] == '\0') {
            break;
        }
    }
    return lhs.z_offset == rhs.z_offset;
}

// TODO: Find a better home for this
inline bool operator==(PID_t lhs, PID_t rhs) {
    return lhs.Kd == rhs.Kd && lhs.Ki == rhs.Ki && lhs.Kp == rhs.Kp;
}

// TODO: Find a better home for this
enum class HWCheckSeverity : uint8_t {
    Ignore = 0,
    Warning = 1,
    Abort = 2
};

namespace eeprom_journal {
// place for constants relevant to eeprom_journal
namespace constants {

} // namespace constants
} // namespace eeprom_journal
