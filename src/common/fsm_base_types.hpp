/**
 * @file fsm_base_types.hpp
 * @author Radek Vana
 * @brief selftest data to be passed between threads
 * @date 2021-03-01
 */

#pragma once

#include <array>
#include <stdint.h>
#include <cstddef> //size_t

namespace fsm {
#pragma pack(push, 1) // must be packed to fit in variant8

static const size_t BaseDataSZ = 5;
using PhaseData = std::array<uint8_t, BaseDataSZ - 1>;

class BaseData {

    PhaseData data;
    uint8_t phase;

public:
    constexpr uint8_t GetPhase() const { return phase; }
    constexpr PhaseData GetData() const { return data; }
    constexpr void SetPhase(uint8_t ph) { phase = ph; }
    constexpr void SetData(PhaseData dt) { data = dt; }

    constexpr BaseData()
        : data({ {} })
        , phase(0) {}
    constexpr BaseData(uint8_t phase, PhaseData data)
        : BaseData() {
        SetPhase(phase);
        SetData(data);
    }
    constexpr bool operator==(const BaseData &other) const {
        return GetPhase() == other.GetPhase() && GetData() == other.GetData();
    }
    constexpr bool operator!=(const BaseData &other) const {
        return !((*this) == other);
    }
};
static_assert(sizeof(BaseData) == BaseDataSZ, "Wrong size of BaseData");

#pragma pack(pop)
};
