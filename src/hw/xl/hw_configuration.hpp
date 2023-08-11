/**
 * @file hw_configuration.hpp
 * @brief configuration of XL
 */

#pragma once
#include "hw_configuration_common.hpp"

namespace buddy::hw {
class Configuration : public ConfigurationCommon {
    Configuration() = default;
    Configuration(const Configuration &) = delete;

public:
    /**
     * @brief Meyers singleton
     * @return Configuration&
     */
    static Configuration &Instance();
};

} // namespace buddy::hw
