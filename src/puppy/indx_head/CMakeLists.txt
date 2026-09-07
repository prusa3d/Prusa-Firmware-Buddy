if(NOT BOARD STREQUAL "INDX_HEAD")
  message(FATAL_ERROR "Unsupported indx_head board: ${BOARD}")
endif()

target_link_options(firmware PUBLIC "-L" "${CMAKE_CURRENT_SOURCE_DIR}/link")
if(BOOTLOADER STREQUAL "EMPTY" OR BOOTLOADER)
  target_set_linker_script(firmware "${CMAKE_CURRENT_SOURCE_DIR}/link/link_boot.ld")
  set(BOOTLOADER_SIZE 8192)

  get_dependency_directory("bootloader-indx_head" _indx_head_bootloader_dir)
  set(_indx_head_bootloader_bin "${_indx_head_bootloader_dir}/bootloader.bin")
  set(_bootloader_code_o "${CMAKE_CURRENT_BINARY_DIR}/bootloader_code.o")
  file(SIZE "${_indx_head_bootloader_bin}" _indx_head_bootloader_bin_size)
  if(_indx_head_bootloader_bin_size EQUAL BOOTLOADER_SIZE)
    add_custom_command(
      OUTPUT "${_bootloader_code_o}"
      COMMAND
        "${CMAKE_OBJCOPY}" -I binary -O elf32-littlearm -B arm --rename-section
        .data=.rodata.bootloader_code,alloc,load,readonly,data,contents --set-section-alignment
        .rodata.bootloader_code=4 --redefine-sym _binary_bootloader_bin_start=bootloader_code
        --redefine-sym _binary_bootloader_bin_end=bootloader_code_end --redefine-sym
        _binary_bootloader_bin_size=bootloader_code_size bootloader.bin "${_bootloader_code_o}"
      WORKING_DIRECTORY "${_indx_head_bootloader_dir}"
      DEPENDS "${_indx_head_bootloader_bin}"
      VERBATIM
      )
    add_custom_target(indx_head_bootloader_code DEPENDS "${_bootloader_code_o}")
    add_dependencies(firmware indx_head_bootloader_code)
    set_source_files_properties(
      "${_bootloader_code_o}" PROPERTIES EXTERNAL_OBJECT TRUE GENERATED TRUE
      )
    target_sources(firmware PRIVATE "${_bootloader_code_o}")
  else()
    message(FATAL_ERROR "bootloader size mismatch")
  endif()
else()
  target_set_linker_script(firmware "${CMAKE_CURRENT_SOURCE_DIR}/link/link_noboot.ld")
  set(BOOTLOADER_SIZE 0)
endif()

target_compile_definitions(
  firmware PRIVATE BOOTLOADER_SIZE=${BOOTLOADER_SIZE} STM32GENERIC STM32C0 STM32C0xx
  )

add_library(HAL_Config INTERFACE)
target_compile_definitions(HAL_Config INTERFACE STM32C092xx USE_HAL_DRIVER USE_FULL_LL_DRIVER)
target_include_directories(HAL_Config INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/config")
add_library(STM32C0_HAL_Config ALIAS HAL_Config)

target_sources(
  firmware
  PRIVATE src/app.cpp
          src/bootloader_update.cpp
          src/hal.cpp
          src/hal_adc.cpp
          src/hal_clk.cpp
          src/hal_crc.cpp
          src/hal_i2c.cpp
          src/hal_rs485.cpp
          src/hal_spi.cpp
          src/hal_tim.cpp
          src/hal_watchdog.cpp
          src/heater.cpp
          src/hotend_temp_compensation.cpp
          src/main.cpp
          src/modbus.cpp
          src/rtt.cpp
          src/spi_task.cpp
          src/startup.s
          src/stdlib.cpp
          src/system.c
          src/timing.cpp
          src/watchdog.cpp
  )

target_link_libraries(
  firmware
  PRIVATE STM32C0::HAL
          HAL_Config
          freertos
          indx_head
          bsod
          modbus
          raii
          fpm
          buddy_utils
          ADS131M02
          AIS2IH
          fifo_encoder
          LP5817
          cortex-m0-atomic
          tpis
          coroutines
  )

target_include_directories(firmware PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

# TODO For now, we need to also explicitly depend on `FreeRTOS::FreeRTOS` because we are using
# functions that are not wrapped by `freertos`.
target_link_libraries(firmware PRIVATE FreeRTOS::FreeRTOS)

add_library(FreeRTOS_Config INTERFACE)
target_link_libraries(FreeRTOS_Config INTERFACE STM32C0xx::CMSIS)
target_include_directories(FreeRTOS_Config INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/config")
