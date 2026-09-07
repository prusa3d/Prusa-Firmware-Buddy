/// @file
#pragma once

#include <option/has_xbuddy_extension.h>
#include <option/has_xl_can.h>

#if HAS_XBUDDY_EXTENSION()
    #include <puppies/xbuddy_extension.hpp>
#elif HAS_XL_CAN()
    #include <puppies/xl_can.hpp>
#else
    #error "Cyphal bridge host required for HAS_TOOL_OFFSET_SENSOR builds (need xBuddy Extension or XL-CAN bridge)"
#endif

namespace buddy::puppies {

/// Puppy driver that controls the cyphal bridge
inline auto &cyphal_bridge_host() {
#if HAS_XBUDDY_EXTENSION()
    return xbuddy_extension;
#elif HAS_XL_CAN()
    return xl_can;
#else
    #error
#endif
}

} // namespace buddy::puppies
