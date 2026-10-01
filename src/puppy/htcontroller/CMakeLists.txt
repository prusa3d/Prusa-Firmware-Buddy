if(NOT BOARD STREQUAL "HTCONTROLLER")
  message(FATAL_ERROR "Unsupported HT controller board: ${BOARD}")
endif()

target_link_options(firmware PUBLIC "-L" "${CMAKE_CURRENT_SOURCE_DIR}/link")
if(BOOTLOADER STREQUAL "EMPTY" OR BOOTLOADER)
  target_set_linker_script(firmware "${CMAKE_CURRENT_SOURCE_DIR}/link/link_boot.ld")
  set(BOOTLOADER_SIZE 8192)
else()
  target_set_linker_script(firmware "${CMAKE_CURRENT_SOURCE_DIR}/link/link_noboot.ld")
  set(BOOTLOADER_SIZE 0)
endif()

target_compile_definitions(
  firmware PRIVATE BOOTLOADER_SIZE=${BOOTLOADER_SIZE} STM32GENERIC STM32G0 STM32G0xx
  )

add_library(HAL_Config INTERFACE)
target_compile_definitions(HAL_Config INTERFACE STM32G070xx USE_HAL_DRIVER)
target_include_directories(HAL_Config INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/config")
add_library(STM32G0_HAL_Config ALIAS HAL_Config)

target_sources(
  firmware
  PRIVATE "${CMAKE_SOURCE_DIR}/src/logging/log.cpp"
          "${CMAKE_SOURCE_DIR}/src/logging/log_dest_rtt.cpp"
          "${CMAKE_SOURCE_DIR}/src/logging/log_dest_shared.cpp"
          "${CMAKE_SOURCE_DIR}/src/logging/log_puppy.cpp"
          src/hal.cpp
          src/logging.cpp
          src/main.cpp
          src/startup.s
          src/stdlib.cpp
          src/system.cpp
          src/timing.cpp
  )

# The shared logging sources include <printf/printf.h> relative to lib
target_include_directories(
  firmware PRIVATE . ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/lib
                   ${CMAKE_SOURCE_DIR}/src/logging/include ${CMAKE_CURRENT_SOURCE_DIR}/src
  )

target_link_libraries(
  firmware
  PRIVATE STM32G0::HAL
          HAL_Config
          bsod
          printf::printf
          buddy_utils
          freertos
          BuddyHeaders
          cortex-m0-atomic
          Segger
  )

# Keep the RTT control block a global symbol so the debugger can locate it LTO would otherwise
# remove it
target_link_options(firmware PRIVATE -Wl,--undefined=_SEGGER_RTT)

# TODO For now, we need to also explicitly depend on `FreeRTOS::FreeRTOS` because we are using
# functions that are not wrapped by `freertos`.
target_link_libraries(firmware PRIVATE FreeRTOS::FreeRTOS)

add_library(FreeRTOS_Config INTERFACE)
target_link_libraries(FreeRTOS_Config INTERFACE STM32G0xx::CMSIS)
target_include_directories(FreeRTOS_Config INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/config")
