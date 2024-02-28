#pragma once

/// Shared for all printers.
/// !!! Never change order, never remove items - this is used in config store
enum class NozzleType : uint8_t {
    Normal = 0,
    HighFlow = 1,
    _cnt,
};
