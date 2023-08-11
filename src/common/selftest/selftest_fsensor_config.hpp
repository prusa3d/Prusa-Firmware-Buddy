/**
 * @file selftest_fsensor_config.hpp
 * @author Radek Vana
 * @brief config filament sensor selftest part
 * currently not needed, but used anyway to behave same as other parts of selftest
 * @date 2021-11-22
 */

#pragma once
#include <cstdint>
#include "selftest_fsensor_type.hpp"
#include "client_response.hpp"
#include "filament_sensor.hpp"

namespace selftest {
// using 32bit variables, because it is stored in flash and access to 32bit variables is more efficient
struct FSensorConfig_t {
    using type_evaluation = SelftestFSensor_t;
    static constexpr SelftestParts part_type = SelftestParts::FSensor;
    const char *partname { "" };
    uint8_t extruder_id = 0;
    bool mmu_mode = false;
};

}; // namespace selftest
