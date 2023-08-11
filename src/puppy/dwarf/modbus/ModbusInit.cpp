#include "ModbusInit.hpp"
#include "hal/HAL_RS485.hpp"
#include "ModbusRegisters.hpp"
#include "ModbusProtocol.hpp"
#include "ModbusTask.hpp"
#include "PuppyConfig.hpp"
#include "ModbusControl.hpp"
#include "puppies/fifo_coder.hpp"
#include "startup/ApplicationStartupArguments.hpp"

// Make release build happy about assert being no-op.
// Those no-op can trigger unused variable warning.
#ifdef NDEBUG
    #undef assert
    #define assert(x) ((void)(x))
#else
    #include <assert.h>
#endif

namespace dwarf {

void modbus_init() {
    bool ret = hal::RS485Driver::Init(get_assigned_modbus_address());
    assert(ret);

    ModbusRegisters::Init();

    modbus::ModbusProtocol::Init(get_assigned_modbus_address());

    ret = ModbusControl::Init();
    assert(ret);

    ret = modbus::ModbusTask::Init();
    assert(ret);
}

} // namespace dwarf
