/**
 * @file selftest_firstlayer_type.hpp
 * @brief selftest first layer data to be passed between threads
 */

#pragma once

#include "fsm_base_types.hpp"
#include "selftest_sub_state.hpp"
#include "general_response.hpp"

struct SelftestFirstLayer_t {
    union {
        float current_offset;
        Response preselect_response;
        fsm::PhaseData converter;
    };

    constexpr SelftestFirstLayer_t(float current_offset)
        : current_offset(current_offset) {}

    constexpr SelftestFirstLayer_t() {
        converter.fill(0);
    }

    constexpr SelftestFirstLayer_t(Response preselect_response)
        : preselect_response(preselect_response) {}

    constexpr SelftestFirstLayer_t(fsm::PhaseData new_data)
        : SelftestFirstLayer_t() {
        Deserialize(new_data);
    }

    constexpr fsm::PhaseData Serialize() const {
        fsm::PhaseData ret = converter; // { { inserted } };
        return ret;
    }

    constexpr void Deserialize(fsm::PhaseData new_data) {
        converter = new_data;
    }

    bool operator==(const SelftestFirstLayer_t &other) const {
        return ((converter == other.converter));
    }

    bool operator!=(const SelftestFirstLayer_t &other) const {
        return !((*this) == other);
    }

    void Pass() {}
    void Fail() {}
    void Abort() {} // currently not needed
};

static_assert(sizeof(SelftestFirstLayer_t) == sizeof(fsm::PhaseData), "Data fits into phase data");
