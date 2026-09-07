add_library(STM32H5_HAL_Config INTERFACE)
target_compile_definitions(STM32H5_HAL_Config INTERFACE STM32H503xx USE_HAL_DRIVER)
target_include_directories(STM32H5_HAL_Config INTERFACE config)

target_compile_definitions(firmware PRIVATE STM32GENERIC STM32H5 STM32H5xx)

target_link_options(firmware PUBLIC "-L" "${CMAKE_CURRENT_SOURCE_DIR}")
if(BOOTLOADER STREQUAL "EMPTY" OR BOOTLOADER)
  target_set_linker_script(firmware "${CMAKE_CURRENT_SOURCE_DIR}/stm32h503_boot.ld")
  target_compile_definitions(firmware PRIVATE BOOTLOADER_SIZE=8192)
else()
  target_set_linker_script(firmware "${CMAKE_CURRENT_SOURCE_DIR}/stm32h503_noboot.ld")
  target_compile_definitions(firmware PRIVATE BOOTLOADER_SIZE=0)
endif()

target_sources(
  firmware
  PRIVATE app.cpp
          cmsis.cpp
          cyphal_application.cpp
          cyphal_nfc_node.cpp
          cyphal_presentation.cpp
          cyphal_transport.cpp
          hal.cpp
          hal_clock.cpp
          hal_ext_fs.cpp
          hal_memory.cpp
          hal_mmu_port.cpp
          hal_pub.cpp
          hal_rng.cpp
          hal_rs485.cpp
          hal_usb.cpp
          main.cpp
          master_activity.cpp
          stdlib.c
          stm32h503.s
          temperature.cpp
  )
target_link_libraries(
  firmware
  PRIVATE printf::printf
          bsod
          dsdl_prusa3d
          dsdl_uavcan
          STM32H5::HAL
          STM32H5_HAL_Config
          freertos
          xbuddy_extension
          buddy_utils
          options
          canard
          o1heap
          ac_controller
          anfc
          modbus
          tool_offset_sensor
          onewire_master
          raii
  )

if(HAS_MMU2)
  # TODO It would be better if Prusa-Firmware-MMU itself declared this library, but ok.
  set(PRUSA_FIRMWARE_MMU_PATH ${CMAKE_SOURCE_DIR}/lib/Prusa-Firmware-MMU)
  add_library(mmu_protocol OBJECT ${PRUSA_FIRMWARE_MMU_PATH}/src/modules/protocol.cpp)
  target_include_directories(mmu_protocol PUBLIC ${PRUSA_FIRMWARE_MMU_PATH}/src)
  target_compile_options(mmu_protocol PRIVATE -Wno-implicit-fallthrough)

  target_sources(firmware PRIVATE mmu.cpp)
  target_link_libraries(firmware PRIVATE mmu_protocol)
endif()

# TODO For now, we need to also explicitly depend on `FreeRTOS::FreeRTOS` because we are using
# functions that are not wrapped by `freertos`.
target_link_libraries(firmware PRIVATE FreeRTOS::FreeRTOS)

target_link_libraries(firmware PUBLIC magic_enum)

add_library(FreeRTOS_Config INTERFACE)
target_include_directories(FreeRTOS_Config INTERFACE config)

set(EXTENSION_STANDARD 1)
set(EXTENSION_IX 2)
set(EXTENSION_XL_CAN 3)

if(${PRINTER} STREQUAL "iX")
  set(EXTENSION_VARIANT ${EXTENSION_IX})

  target_sources(firmware PRIVATE hal_gpio_expander.cpp)
elseif(
  ${PRINTER} STREQUAL "COREONE"
  OR ${PRINTER} STREQUAL "COREONEL"
  OR ${PRINTER} STREQUAL "COREONE_INDX"
  OR ${PRINTER} STREQUAL "COREONEL_INDX"
  )
  set(EXTENSION_VARIANT ${EXTENSION_STANDARD})

  target_sources(firmware PRIVATE hal_gpio_expander.cpp)
elseif(${PRINTER} STREQUAL "XL")
  set(EXTENSION_VARIANT ${EXTENSION_XL_CAN})
else()
  message(FATAL_ERROR "Building xBuddyExtension for an incompatible printer")
endif()

if(HAS_MMU2)
  target_sources(firmware PRIVATE hal_mmu.cpp)
endif()

target_compile_definitions(
  firmware PRIVATE EXTENSION_STANDARD=${EXTENSION_STANDARD} EXTENSION_IX=${EXTENSION_IX}
                   EXTENSION_XL_CAN=${EXTENSION_XL_CAN} EXTENSION_VARIANT=${EXTENSION_VARIANT}
  )
