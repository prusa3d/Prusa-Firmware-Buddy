if(NOT TARGET FreeRTOS_Config)
  message(FATAL_ERROR "Target FreeRTOS_Config does not exist.")
endif()

add_library(
  FreeRTOS
  Source/croutine.c
  Source/event_groups.c
  Source/list.c
  Source/queue.c
  Source/stream_buffer.c
  Source/tasks.c
  Source/timers.c
  )

target_include_directories(FreeRTOS PUBLIC Source/include)

if(MCU MATCHES "STM32F4")
  target_sources(FreeRTOS PRIVATE Source/portable/GCC/ARM_CM4F/port.c)
  target_include_directories(FreeRTOS PUBLIC Source/portable/GCC/ARM_CM4F)
elseif(MCU MATCHES "STM32G0")
  target_sources(FreeRTOS PRIVATE Source/portable/GCC/ARM_CM0/port.c)
  target_include_directories(FreeRTOS PUBLIC Source/portable/GCC/ARM_CM0)
elseif(MCU MATCHES "STM32H5")
  target_include_directories(FreeRTOS PUBLIC Source/portable/GCC/ARM_CM33_NTZ/non_secure)
  target_sources(FreeRTOS PRIVATE
    Source/portable/GCC/ARM_CM33_NTZ/non_secure/port.c
    Source/portable/GCC/ARM_CM33_NTZ/non_secure/portasm.c
    Source/portable/GCC/ARM_CM33_NTZ/non_secure/mpu_wrappers_v2_asm.c
    Source/portable/Common/mpu_wrappers_v2.c
    )
else()
  message(FATAL_ERROR "Unknown HW platform of mcu \"${MCU}\"")
endif()

# TODO start disabling for some builds where CMSIS RTOS is not needed.
set(HAS_CMSIS_RTOS ON)

if(HAS_CMSIS_RTOS)
  target_sources(FreeRTOS PRIVATE Source/CMSIS_RTOS/cmsis_os.c)
  target_include_directories(FreeRTOS PUBLIC Source/CMSIS_RTOS)
endif()

target_link_libraries(FreeRTOS PUBLIC FreeRTOS_Config)

add_library(FreeRTOS::FreeRTOS ALIAS FreeRTOS)
