#include "selftest_tool_helper.hpp"
#include <option/has_toolchanger.h>
#include "selftest_types.hpp"
#if HAS_TOOLCHANGER()
    #include "module/prusa/toolchanger.h"
#endif

bool is_tool_selftest_enabled(const uint8_t tool, const ToolMask mask) {
#if HAS_TOOLCHANGER()
    if (!prusa_toolchanger.is_tool_enabled(tool)) {
        return false;
    }
#endif

    if (!(static_cast<uint8_t>(mask) & 1 << tool)) {
        return false;
    }

    return true;
}
