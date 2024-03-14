add_library(
  LUFA
  Descriptors.c
  lufa/LUFA/Drivers/USB/Class/Device/CDCClassDevice.c
  lufa/LUFA/Drivers/USB/Core/AVR8/Device_AVR8.c
  lufa/LUFA/Drivers/USB/Core/AVR8/Endpoint_AVR8.c
  lufa/LUFA/Drivers/USB/Core/AVR8/EndpointStream_AVR8.c
  lufa/LUFA/Drivers/USB/Core/AVR8/USBController_AVR8.c
  lufa/LUFA/Drivers/USB/Core/AVR8/USBInterrupt_AVR8.c
  lufa/LUFA/Drivers/USB/Core/ConfigDescriptors.c
  lufa/LUFA/Drivers/USB/Core/DeviceStandardReq.c
  lufa/LUFA/Drivers/USB/Core/Events.c
  lufa/LUFA/Drivers/USB/Core/USBTask.c
  )

target_include_directories(LUFA PRIVATE . lufa)
target_compile_options(LUFA PRIVATE -include lufa_config.h)

target_compile_features(LUFA PUBLIC c_std_99)
