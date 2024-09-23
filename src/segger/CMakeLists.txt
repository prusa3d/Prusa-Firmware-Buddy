if(MCU MATCHES "STM32G0")
  target_sources(firmware PRIVATE SEGGER_SYSVIEW_Config_FreeRTOS_G0.cpp)
elseif(MCU MATCHES "STM32F4")
  target_sources(firmware PRIVATE SEGGER_SYSVIEW_Config_FreeRTOS_F4.cpp)
else()
  message(FATAL_ERROR "Unknown subdirectory for ${MCU}")
endif()

target_sources(firmware PRIVATE SEGGER_SYSVIEW_FreeRTOS.cpp)
