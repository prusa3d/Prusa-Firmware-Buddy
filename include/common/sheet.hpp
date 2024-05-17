#pragma once
#include <array>

inline constexpr size_t MAX_SHEET_NAME_LENGTH { 8 };

/// Buffer size to fit null-terminated sheet name
inline constexpr size_t SHEET_NAME_BUFFER_SIZE = MAX_SHEET_NAME_LENGTH + 1;

// #define Z_OFFSET_STEP     0.0025F//calculated
#define Z_OFFSET_MIN -2.0F
#define Z_OFFSET_MAX 2.0F

struct Sheet {
    char name[MAX_SHEET_NAME_LENGTH]; //!< Can be null terminated, doesn't need to be null terminated
    float z_offset; //!< Z_BABYSTEP_MIN .. Z_BABYSTEP_MAX = Z_BABYSTEP_MIN*2/1000 [mm] .. Z_BABYSTEP_MAX*2/1000 [mm]
};

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
