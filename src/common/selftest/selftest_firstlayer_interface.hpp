/**
 * @file selftest_firstlayer_interface.hpp
 * @brief Set of functions adding functionality to selftest classes
 * Could be solved with multiple inheritance (did not want that)
 * or Interface (C++ does not have)
 */
#pragma once
#include <cstdint>
#include "selftest_firstlayer_config.hpp"

class IPartHandler;

namespace selftest {

bool phaseFirstLayer(IPartHandler *&pFirstLayer, const FirstLayerConfig_t &config);

};
