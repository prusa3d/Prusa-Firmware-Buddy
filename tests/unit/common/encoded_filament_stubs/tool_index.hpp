#pragma once

#include <cstdint>

struct PhysicalToolIndex {
    constexpr static int count = 6;
    uint8_t val;
    constexpr static PhysicalToolIndex from_raw(uint8_t val) { return PhysicalToolIndex { val }; }
    constexpr auto to_raw() const { return val; }
    constexpr inline bool operator==(const PhysicalToolIndex &) const = default;
};

struct VirtualToolIndex {
    constexpr static int count = 6;
    uint8_t val;
    constexpr static VirtualToolIndex from_raw(uint8_t val) { return VirtualToolIndex { val }; }
    constexpr auto to_raw() const { return val; }
    constexpr PhysicalToolIndex to_physical() const { return PhysicalToolIndex { val }; }
    constexpr inline bool operator==(const VirtualToolIndex &) const = default;
};

struct NoTool {
    constexpr inline bool operator==(const NoTool &) const = default;
};
