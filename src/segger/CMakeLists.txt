if(MCU MATCHES "STM32G0")
  target_sources(firmware PRIVATE SEGGER_SYSVIEW_Config_FreeRTOS_G0.c)
else()
  target_sources(firmware PRIVATE SEGGER_SYSVIEW_Config_FreeRTOS_F4.c)
endif()

target_sources(firmware PRIVATE SEGGER_SYSVIEW_FreeRTOS.c)
