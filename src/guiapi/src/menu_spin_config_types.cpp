/**
 * @file menu_spin_config_types.cpp
 * @author Radek Vana
 * @brief
 * @version 0.1
 * @date 2020-11-05
 *
 * @copyright Copyright (c) 2020
 */
#include "menu_spin_config_types.hpp"

template <>
const char *const SpinConfig<int8_t>::prt_format = "%d";
template <>
const char *const SpinConfig<int16_t>::prt_format = "%d";
template <>
const char *const SpinConfig<int32_t>::prt_format = "%d";

template <>
const char *const SpinConfig<uint8_t>::prt_format = "%u";
template <>
const char *const SpinConfig<uint16_t>::prt_format = "%u";
template <>
const char *const SpinConfig<uint32_t>::prt_format = "%u";

template <>
const char *const SpinConfig<float>::prt_format = "%f";
