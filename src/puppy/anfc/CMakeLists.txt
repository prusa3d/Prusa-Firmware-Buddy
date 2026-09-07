include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/options.cmake")

if(NOT BOARD STREQUAL "ANFC")
  message(FATAL_ERROR "Unsupported nfc board: ${BOARD}")
endif()

target_link_options(firmware PUBLIC "-L" "${CMAKE_CURRENT_SOURCE_DIR}/link")
if(BOOTLOADER STREQUAL "EMPTY" OR BOOTLOADER)
  target_set_linker_script(firmware "${CMAKE_CURRENT_SOURCE_DIR}/link/link_boot.ld")
  set(BOOTLOADER_SIZE 49152)
else()
  target_set_linker_script(firmware "${CMAKE_CURRENT_SOURCE_DIR}/link/link_noboot.ld")
  set(BOOTLOADER_SIZE 0)
endif()

target_compile_definitions(
  firmware PRIVATE BOOTLOADER_SIZE=${BOOTLOADER_SIZE} STM32GENERIC STM32C0 STM32C0xx
  )

add_library(HAL_Config INTERFACE)
target_compile_definitions(HAL_Config INTERFACE STM32C092xx USE_HAL_DRIVER)
target_include_directories(HAL_Config INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/config")
add_library(STM32C0_HAL_Config ALIAS HAL_Config)

target_sources(
  firmware
  PRIVATE "${CMAKE_SOURCE_DIR}/src/device/multi_watchdog.cpp"
          src/adc.cpp
          src/anfc_event_publisher.cpp
          src/cyphal_anfc_node.cpp
          src/enum_mapping_check.cpp
          src/hal.cpp
          src/main.cpp
          src/nfc.cpp
          src/otp.cpp
          src/precise_timer.cpp
          src/startup.s
          src/stdlib.cpp
          src/system.c
  )

target_include_directories(
  firmware PRIVATE . ${CMAKE_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/src ../shared/hal
                   ../honeybee_shared
  )

target_link_libraries(
  firmware
  PRIVATE STM32C0::HAL
          HAL_Config
          bsod
          printf::printf
          SG14
          buddy_utils
          freertos
          dsdl_uavcan
          dsdl_prusa3d
          canard
          BuddyHeaders
          o1heap
          openprinttag
          openprinttag_nfcv
          prusa_nfc_task
          st25r39xxb
          raii
          crc
          mbedTLS
          log_dev_null
          cortex-m0-atomic
  )

# TODO For now, we need to also explicitly depend on `FreeRTOS::FreeRTOS` because we are using
# functions that are not wrapped by `freertos`.
target_link_libraries(firmware PRIVATE FreeRTOS::FreeRTOS)

target_link_libraries(firmware PUBLIC magic_enum)

add_library(FreeRTOS_Config INTERFACE)
target_link_libraries(FreeRTOS_Config INTERFACE STM32C0xx::CMSIS)
target_include_directories(FreeRTOS_Config INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/config")
