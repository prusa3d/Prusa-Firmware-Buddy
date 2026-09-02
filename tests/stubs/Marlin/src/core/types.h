#pragma once

#include <array>
#include <cstdint>

enum AxisEnum : uint8_t {
    X_AXIS,
    Y_AXIS,
    Z_AXIS,
    E_AXIS,

    A_AXIS = X_AXIS,
    B_AXIS = Y_AXIS,
    C_AXIS = Z_AXIS,
};

struct xyze_float_t {
    union {
        struct {
            float x, y, z, e;
        };
        float pos[4];
    };

    // Default constructor (zero-initialize)
    constexpr xyze_float_t()
        : x(0)
        , y(0)
        , z(0)
        , e(0) {}

    // Constructor from individual values
    constexpr xyze_float_t(float x_in, float y_in, float z_in, float e_in)
        : x(x_in)
        , y(y_in)
        , z(z_in)
        , e(e_in) {}

    // Constructor from array (for double-brace initialization syntax)
    constexpr xyze_float_t(const float (&arr)[4])
        : x(arr[0])
        , y(arr[1])
        , z(arr[2])
        , e(arr[3]) {}

    // Array access operator
    constexpr float &operator[](int n) { return pos[n]; }
    constexpr const float &operator[](int n) const { return pos[n]; }

    // Arithmetic operators (commonly used in Marlin)
    constexpr xyze_float_t operator+(const xyze_float_t &rs) const {
        return { x + rs.x, y + rs.y, z + rs.z, e + rs.e };
    }

    constexpr xyze_float_t operator-(const xyze_float_t &rs) const {
        return { x - rs.x, y - rs.y, z - rs.z, e - rs.e };
    }

    constexpr xyze_float_t operator*(float v) const {
        return { x * v, y * v, z * v, e * v };
    }

    constexpr xyze_float_t operator/(float v) const {
        return { x / v, y / v, z / v, e / v };
    }

    // Compound assignment operators
    xyze_float_t &operator+=(const xyze_float_t &rs) {
        x += rs.x;
        y += rs.y;
        z += rs.z;
        e += rs.e;
        return *this;
    }

    xyze_float_t &operator-=(const xyze_float_t &rs) {
        x -= rs.x;
        y -= rs.y;
        z -= rs.z;
        e -= rs.e;
        return *this;
    }

    // Comparison operators
    constexpr bool operator==(const xyze_float_t &rs) const {
        return x == rs.x && y == rs.y && z == rs.z && e == rs.e;
    }

    constexpr bool operator!=(const xyze_float_t &rs) const {
        return !(*this == rs);
    }
};

using xyze_pos_t = xyze_float_t;

// Machine and native positions are the same type in the firmware for now too.
using MachinePosXYZE = xyze_float_t;
