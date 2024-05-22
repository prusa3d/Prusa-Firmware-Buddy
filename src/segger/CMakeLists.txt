if(MCU MATCHES "STM32G0")
  target_sources(firmware PRIVATE SEGGER_SYSVIEW_Config_FreeRTOS_G0.cpp)
else()
  target_sources(firmware PRIVATE SEGGER_SYSVIEW_Config_FreeRTOS_F4.cpp)
endif()

target_sources(firmware PRIVATE SEGGER_SYSVIEW_FreeRTOS.cpp)
