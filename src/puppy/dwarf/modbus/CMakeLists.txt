target_sources(
  firmware PRIVATE ModbusControl.cpp ModbusFIFOHandlers.cpp ModbusInit.cpp ModbusRegisters.cpp
  )

target_include_directories(firmware PUBLIC .)
