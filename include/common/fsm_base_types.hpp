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
#pragma pack(push, 1)

inline constexpr size_t BaseDataSZ = 5;
using PhaseData = std::array<uint8_t, BaseDataSZ - 1>;

template <typename T>
PhaseData serialize_data(const T &t) {
    static_assert(std::is_trivially_copyable_v<T>);
    static_assert(sizeof(T) <= sizeof(PhaseData));

    PhaseData result;
    memcpy(result.data(), &t, sizeof(T));
    return result;
}

template <typename T>
T deserialize_data(PhaseData data) {
    static_assert(std::is_trivially_copyable_v<T>);
    static_assert(sizeof(T) <= sizeof(PhaseData));

    T result;
    memcpy(&result, data.data(), sizeof(T));
    return result;
}

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

    constexpr auto operator<=>(const BaseData &) const = default;
};
static_assert(sizeof(BaseData) == BaseDataSZ, "Wrong size of BaseData");

#pragma pack(pop)

}; // namespace fsm
