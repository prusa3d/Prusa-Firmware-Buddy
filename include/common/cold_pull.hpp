#pragma once

#include "fsm_base_types.hpp"

namespace cold_pull {

union TemperatureProgressData {
    fsm::PhaseData fsm_data;
    struct __attribute__((packed)) {
        uint8_t percent;
        uint8_t _pad;
        uint16_t time_sec;
    };
};
static_assert(sizeof(TemperatureProgressData) == sizeof(fsm::PhaseData));

}; // namespace cold_pull
