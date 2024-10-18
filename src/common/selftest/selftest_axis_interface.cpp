/**
 * @file selftest_axis_interface.cpp
 * @author Radek Vana
 * @date 2021-09-24
 */
#include "selftest_axis_interface.hpp"
#include "selftest_axis.h"
#include "selftest_axis_type.hpp"
#include "selftest_sub_state.hpp"
#include "marlin_server.hpp"
#include "../../Marlin/src/module/stepper.h"
#include "selftest_part.hpp"
#include <config_store/store_instance.hpp>

namespace selftest {

bool phaseAxis(IPartHandler *&m_pAxis, const AxisConfig_t &config_axis, Separate separate) {
    static SelftestSingleAxis_t staticResults[axis_count];

    // validity check
    if (config_axis.axis >= axis_count) {
        return false;
    }

    if (m_pAxis == nullptr) {
        // Convert from EEPROM test state to GUI subtest state
        auto to_SubtestState = [](TestResult state) {
            switch (state) {
            case TestResult_Failed:
                return SelftestSubtestState_t::not_good;
            case TestResult_Passed:
                return SelftestSubtestState_t::ok;
            default:
                return SelftestSubtestState_t::undef;
            }
        };

        // Init staticResults with current state from EEPROM
        SelftestResult eeres = config_store().selftest_result.get();
        staticResults[0].state = to_SubtestState(eeres.xaxis);
        staticResults[1].state = to_SubtestState(eeres.yaxis);
        staticResults[2].state = to_SubtestState(eeres.zaxis);

        // Allocate selftest part
        switch (config_axis.axis) {
        case Z_AXIS:
            // clang-format off
        m_pAxis = selftest::Factory::CreateDynamical<CSelftestPart_Axis>(config_axis, staticResults[config_axis.axis],
            &CSelftestPart_Axis::stateHomeZ,
            &CSelftestPart_Axis::stateWaitHome,
            &CSelftestPart_Axis::stateEnableZProbe,
            &CSelftestPart_Axis::stateInitProgressTimeCalculation,

            &CSelftestPart_Axis::stateCycleMark2,
            &CSelftestPart_Axis::stateMove,
            &CSelftestPart_Axis::stateMoveFinishCycle,
            &CSelftestPart_Axis::stateParkAxis,
            &CSelftestPart_Axis::state_verify_coils);
            // clang-format on
            break;

        case X_AXIS:
        case Y_AXIS:
            m_pAxis = selftest::Factory::CreateDynamical<CSelftestPart_Axis>(config_axis, staticResults[config_axis.axis],
                &CSelftestPart_Axis::stateActivateHomingReporter,
                &CSelftestPart_Axis::stateHomeXY,
                &CSelftestPart_Axis::stateWaitHomingReporter,
                &CSelftestPart_Axis::stateEvaluateHomingXY,
                &CSelftestPart_Axis::stateInitProgressTimeCalculation,

                &CSelftestPart_Axis::stateCycleMark2,
                &CSelftestPart_Axis::stateMove,
                &CSelftestPart_Axis::stateMoveFinishCycle,
                &CSelftestPart_Axis::stateParkAxis,
                &CSelftestPart_Axis::state_verify_coils //
            );
            break;

        default:
            break;
        }
    }

    bool in_progress = m_pAxis->Loop();
    SelftestAxis_t result = SelftestAxis_t(staticResults[0], staticResults[1], staticResults[2], (separate == Separate::yes) ? config_axis.axis : (Z_AXIS + 1)); // If separate, use >Z_AXIS
    marlin_server::fsm_change(IPartHandler::GetFsmPhase(), result.Serialize());

    if (in_progress) {
        return true;
    }

    SelftestResult eeres = config_store().selftest_result.get();
    switch (config_axis.axis) {
    case X_AXIS:
        eeres.xaxis = m_pAxis->GetResult();
        break;
    case Y_AXIS:
        eeres.yaxis = m_pAxis->GetResult();
        break;
    case Z_AXIS:
        eeres.zaxis = m_pAxis->GetResult();
        break;

    default:
        break;
    }
    config_store().selftest_result.set(eeres);

    delete m_pAxis;
    m_pAxis = nullptr;
    return false;
}
} // namespace selftest
