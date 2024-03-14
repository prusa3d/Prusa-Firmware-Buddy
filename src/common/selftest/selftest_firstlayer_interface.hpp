/**
 * @file selftest_firstlayer_interface.hpp
 * @brief interface between selftest and first layer calibration
 */
#pragma once
#include <cstdint>
#include "selftest_firstlayer_config.hpp"

class IPartHandler;

namespace selftest {

/**
 * @brief this function is cyclically called in selftest
 * 1. handles dynamical allocation of selftest part state machine
 * 2. calls loop and notifies other threads its state was changed
 * 3. stores result to eeprom
 * 4. handles deallocation
 *
 * @param pFirstLayer reference to pointer of state machine
 * @param config      configuration structure
 * @retval true       not finished
 * @retval false      finished
 */
bool phaseFirstLayer(IPartHandler *&pFirstLayer, const FirstLayerConfig_t &config);

}; // namespace selftest
