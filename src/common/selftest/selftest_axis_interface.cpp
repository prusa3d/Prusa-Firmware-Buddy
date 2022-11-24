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
#include "eeprom.h"

namespace selftest {
SelftestSingleAxis_t staticResults[axis_count]; // automatically initialized by PartHandler

bool phaseAxis(IPartHandler *&m_pAxis, const AxisConfig_t &config_axis) {
    //validity check
    if (config_axis.axis >= axis_count)
        return false;

    // clang-format off
    m_pAxis = m_pAxis ? m_pAxis : selftest::Factory::CreateDynamical<CSelftestPart_Axis>(config_axis,
        staticResults[config_axis.axis],
        &CSelftestPart_Axis::stateWaitHome, &CSelftestPart_Axis::stateInitProgressTimeCalculation, &CSelftestPart_Axis::stateCycleMark,
        &CSelftestPart_Axis::stateMove, &CSelftestPart_Axis::stateMoveWaitFinish);
    // clang-format on

    SelftestResultEEprom_t eeres;
    eeres.ui32 = variant8_get_ui32(eeprom_get_var(EEVAR_SELFTEST_RESULT));

    bool in_progress = m_pAxis->Loop();
    SelftestAxis_t result = SelftestAxis_t(staticResults[0], staticResults[1], staticResults[2]);
    FSM_CHANGE_WITH_DATA__LOGGING(Selftest, IPartHandler::GetFsmPhase(), result.Serialize());

    if (in_progress) {
        return true;
    }

    switch (config_axis.axis) {
    case X_AXIS:
        eeres.xaxis = uint8_t(m_pAxis->GetResult());
        break;
    case Y_AXIS:
        eeres.yaxis = uint8_t(m_pAxis->GetResult());
        break;
    case Z_AXIS:
        eeres.zaxis = uint8_t(m_pAxis->GetResult());
        break;
    }
    eeprom_set_var(EEVAR_SELFTEST_RESULT, variant8_ui32(eeres.ui32));

    delete m_pAxis;
    m_pAxis = nullptr;
    return false;
}
} // namespace selftest
