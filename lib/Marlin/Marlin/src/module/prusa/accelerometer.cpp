#include "accelerometer.h"

#include <enum_array.hpp>

const char *PrusaAccelerometer::error_str() const {
    static constexpr EnumArray<Error, const char *, Error::_cnt> error_strs {
        { Error::none, nullptr },
            { Error::communication, "accelerometer_communication" },

#if HAS_REMOTE_ACCELEROMETER()
            { Error::no_active_tool, "no active tool" },
            { Error::busy, "busy" },
#endif

            { Error::overflow_sensor, "sample overrun on accelerometer sensor" },

#if HAS_REMOTE_ACCELEROMETER()
            { Error::overflow_buddy, "sample missed on buddy" },
            { Error::overflow_dwarf, "sample missed on dwarf" },
            { Error::overflow_possible, "sample possibly lost in transfer" },
#endif
    };
    return error_strs[get_error()];
}
