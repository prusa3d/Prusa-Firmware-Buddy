#include "motor_current_profile.hpp"

#include <bsod/bsod.h>
#include <inc/MarlinConfig.h>
#include <module/stepper/indirection.h>
#include <option/has_indx.h>
#include <persistent_stores/store_instances/config_store/store_c_api.h>

static_assert(HAS_INDX(), "This file should only be compiled for INDX printers");

using namespace buddy;

static StandardMotorCurrentProfile current_profile_state = StandardMotorCurrentProfile::fw_default;

static MotorCurrentProfile get_fw_default_profile() {
    return {
        .x = get_rms_current_ma_x(),
        .y = get_rms_current_ma_y(),
        .z = get_rms_current_ma_z(),
        .e = get_rms_current_ma_e(),
    };
}

const MotorCurrentProfile &buddy::standard_motor_current_profile(StandardMotorCurrentProfile profile) {
    switch (profile) {

    case StandardMotorCurrentProfile::fw_default: {
        static const MotorCurrentProfile p = get_fw_default_profile();
        return p;
    }

    case StandardMotorCurrentProfile::increased_e: {
        static const MotorCurrentProfile p = [] {
            auto p = get_fw_default_profile();
            p.e = std::max<uint16_t>(p.e, 650);
            return p;
        }();
        return p;
    }

    case StandardMotorCurrentProfile::decreased_e_flex: {
        static const MotorCurrentProfile p = [] {
            auto p = get_fw_default_profile();
            p.e = std::min<uint16_t>(p.e, 450);
            return p;
        }();
        return p;
    }

    case StandardMotorCurrentProfile::_count:
        break;
    }

    bsod_unreachable();
}

StandardMotorCurrentProfile buddy::active_motor_current_profile() {
    return current_profile_state;
}

void buddy::set_active_motor_current_profile(StandardMotorCurrentProfile profile) {
    current_profile_state = profile;
    const auto &current = standard_motor_current_profile(profile);

#if AXIS_IS_TMC(X)
    stepperX.rms_current(current.x);
#endif
#if AXIS_IS_TMC(Y)
    stepperY.rms_current(current.y);
#endif
#if AXIS_IS_TMC(Z)
    stepperZ.rms_current(current.z);
#endif
#if AXIS_IS_TMC(E0)
    stepperE0.rms_current(current.e);
#endif
}
