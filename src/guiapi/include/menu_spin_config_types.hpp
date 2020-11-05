/**
 * @file menu_spin_config_types.hpp
 * @author Radek Vana
 * @brief
 * @version 0.1
 * @date 2020-11-04
 *
 * @copyright Copyright (c) 2020
 * do not include this file outside menu_spin_config_type.hpp
 * include menu_spin_config_type.hpp instead
 */
#pragma once
#include <array>

// const char* Unit() is not virtual, because only one of SpinConfig SpinConfigWithUnit is used

union SpinType {
    float flt;
    uint32_t u32;
    int32_t i32;
    uint16_t u16;
    int16_t i16;
    uint8_t u08;
    int8_t i08;

    constexpr operator float() const { return flt; }
    constexpr operator uint32_t() const { return u32; }
    constexpr operator int32_t() const { return i32; }
    constexpr operator uint16_t() const { return u16; }
    constexpr operator int16_t() const { return i16; }
    constexpr operator uint8_t() const { return u08; }
    constexpr operator int8_t() const { return i08; }

    constexpr SpinType(float x)
        : flt(x) {}
    constexpr SpinType(uint32_t x)
        : u32(x) {}
    constexpr SpinType(int32_t x)
        : i32(x) {}
    constexpr SpinType(uint16_t x)
        : u32(x) {} //meant to be 32 bit not a bug
    constexpr SpinType(int16_t x)
        : i32(x) {} //meant to be 32 bit not a bug
    constexpr SpinType(uint8_t x)
        : u32(x) {} //meant to be 32 bit not a bug
    constexpr SpinType(int8_t x)
        : i32(x) {} //meant to be 32 bit not a bug
};

template <class T>
struct SpinConfig {
    static constexpr const char *nullstr = "";
    std::array<T, 3> range; // todo change array to struct containing min, max, step
    static const char *const prt_format;

    constexpr SpinConfig(const std::array<T, 3> &arr)
        : range(arr) {}
    constexpr T Min() const { return range[0]; }
    constexpr T Max() const { return range[1]; }
    constexpr T Step() const { return range[2]; }
    constexpr const char *Unit() { return nullstr; } // not virtual
    //char *sn_prt(std::array<char, 10>& temp_buff, SpinType value) const;
    // bool Change(SpinType value, int dif) const;
};

template <class T>
struct SpinConfigWithUnit : public SpinConfig<T> {
    const char *const unit;

    constexpr SpinConfigWithUnit(const std::array<T, 3> &arr, const char *unit_)
        : SpinConfig<T>(arr)
        , unit(unit_) {}
    constexpr const char *Unit() { return unit; } // not virtual
};
/*
template <class T>
char *SpinConfig<T>::sn_prt(std::array<char, 10>& temp_buff, SpinType value) const {
    snprintf(temp_buff.data(), temp_buff.size(), prt_format, (T)(value));
    return temp_buff.data();
}

template <>
inline char *SpinConfig<float>::sn_prt(std::array<char, 10>& temp_buff, SpinType value) const {
    snprintf(temp_buff.data(), temp_buff.size(), prt_format, static_cast<double>(value.flt));
    return temp_buff.data();
}

*/
