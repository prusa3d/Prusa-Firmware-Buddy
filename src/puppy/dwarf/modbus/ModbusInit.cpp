#include "ModbusInit.hpp"
#include "hal/HAL_RS485.hpp"
#include "ModbusRegisters.hpp"
#include "ModbusProtocol.hpp"
#include "ModbusTask.hpp"
#include "PuppyConfig.hpp"
#include "ModbusControl.hpp"
#include <fifo_coder/fifo_encoder.hpp>
#include "startup/ApplicationStartupArguments.hpp"
#include <bsod/bsod.h>

namespace dwarf {

void modbus_init() {
    bool ret = hal::RS485Driver::Init(get_assigned_modbus_address());
    debug_assert(ret);

    ModbusRegisters::Init();

    modbus::ModbusProtocol::Init(get_assigned_modbus_address());

    ret = ModbusControl::Init();
    debug_assert(ret);

    ret = modbus::ModbusTask::Init();
    debug_assert(ret);
}

} // namespace dwarf
