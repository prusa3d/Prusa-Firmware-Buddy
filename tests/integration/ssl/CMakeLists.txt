cmake_minimum_required(VERSION 3.15)

project(
  SSL-tests
  LANGUAGES C CXX ASM
  VERSION 0.0.1
  )

set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../)

set(BOARD_TYPE BUDDY_BOARD)

add_library(A3idesHeaders INTERFACE)
target_include_directories(
  A3idesHeaders
  INTERFACE ${ROOT_DIR}/include ${ROOT_DIR}/include/stm32f4_hal ${ROOT_DIR}/include/usb_host
            ${ROOT_DIR}/include/usb_device ${ROOT_DIR}/include/marlin ${ROOT_DIR}/include/freertos
  )
target_link_libraries(A3idesHeaders INTERFACE STM32F4::HAL FreeRTOS::FreeRTOS)
target_compile_definitions(
  A3idesHeaders
  INTERFACE
  MOTHERBOARD=1823
  MCU=STM32F407VG
  PRINTER_TYPE=PRINTER_PRUSA_MINI
  BOARD_TYPE=${BOARD_TYPE}
  MARLIN_DISABLE_INFINITE_LOOP
  PROCESS_CUSTOM_GCODE
  STM32GENERIC
  STM32F4
  _EXTUI
  )

#
# Configure Arduino Core
#

set(ARDUINO_CORE_VARIANT "2209")
set(ARDUINO_CORE_TARGET "STM32F4xx")

#
# Configure STMicroelectronics Libraries
#

set(STM32F4_HAL_TARGET "STM32F407xx")
add_library(STM32F4_HAL_Config INTERFACE)
target_include_directories(STM32F4_HAL_Config INTERFACE ${ROOT_DIR}/include/stm32f4_hal)

# STM32::USBHost
add_library(STM32_USBHost_Config ALIAS A3idesHeaders)

# STM32::USBDevice
add_library(STM32_USBDevice_Config ALIAS A3idesHeaders)

# STM32::Utilities::CPU
add_library(STM32_Utilities_CPU_Config ALIAS A3idesHeaders)

#
# Configure FreeRTOS
#

add_library(FreeRTOS_Config INTERFACE)
target_include_directories(FreeRTOS_Config INTERFACE ${ROOT_DIR}/include/freertos)
target_link_libraries(FreeRTOS_Config INTERFACE STM32::CPU)

#
# Configure LwIP
#

add_library(LwIP_Config INTERFACE)
target_link_libraries(LwIP_Config INTERFACE A3idesHeaders)

#
# Configure FatFs
#

add_library(FatFs_Config INTERFACE)
target_link_libraries(FatFs_Config INTERFACE A3idesHeaders STM32::USBHost)

#
# Configure Marlin
#

add_library(Marlin_Config INTERFACE)
# TODO: fix dependency on src/common and src/gui
target_include_directories(
  Marlin_Config
  INTERFACE ${ROOT_DIR}/include/marlin ${ROOT_DIR}/src/common ${ROOT_DIR}/src/common/selftest
            ${ROOT_DIR}/src/gui ${ROOT_DIR}/src/marlin_stubs/include
  )
target_link_libraries(Marlin_Config INTERFACE A3idesHeaders FreeRTOS::FreeRTOS)
add_compile_options(-DconfigUSE_NEWLIB_REENTRANT=1)
# include symbols
add_compile_options(-g)

# optimizations
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_options(-Og)
else() # != "Debug"
  add_compile_options(-Os)
endif() # "Debug"

# mcu related settings
set(MCU_FLAGS -mthumb -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16)
add_compile_options(${MCU_FLAGS})
add_link_options(${MCU_FLAGS})

# better FreeRTOS support
add_link_options(-Wl,--undefined=uxTopUsedPriority,--undefined=init_task)

# split and gc sections
add_compile_options(-ffunction-sections -fdata-sections)
add_link_options(-Wl,--gc-sections)

# disable exceptions and related metadata
add_compile_options(-fno-exceptions -fno-unwind-tables)
add_link_options(-Wl,--defsym,__exidx_start=0,--defsym,__exidx_end=0)

# use custom printf implementation instead of the one in newlib (see lib/printf)
add_link_options(
  -Wl,--defsym=printf=printf_,--defsym=sprintf=sprintf_,--defsym=snprintf=snprintf_,--defsym=vprintf=vprintf_,--defsym=vsnprintf=vsnprintf_
  )

# enable all warnings (well, not all, but some)
add_compile_options(-Wall -Wsign-compare)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wno-register> $<$<COMPILE_LANGUAGE:CXX>:-std=c++1z>)

# support _DEBUG macro (some code uses to recognize debug builds)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_definitions(_DEBUG)
endif()

set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/../../../src/STM32F407VG_FLASH.ld")
add_link_options("-Wl,-T,${LINKER_SCRIPT}")

message(STATUS "${CMAKE_BINARY_DIR}")
add_subdirectory(${ROOT_DIR}/lib ${CMAKE_BINARY_DIR}/lib)

add_executable(firmware)

add_subdirectory(${ROOT_DIR}/src/lang ${CMAKE_BINARY_DIR}/lang)
set_target_properties(firmware PROPERTIES CXX_STANDARD 17)

# generate firmware.bin file
objcopy(firmware "binary" ".bin")

# generate linker map file
target_link_options(firmware PUBLIC -Wl,-Map=firmware.map)

# inform about the firmware's size in terminal
report_size(firmware)

add_link_dependency(firmware "${LINKER_SCRIPT}")

target_include_directories(
  firmware
  PRIVATE ${ROOT_DIR}/include
          ${ROOT_DIR}/src/common
          ${ROOT_DIR}/src/common/include
          ${ROOT_DIR}/src/gui
          ${ROOT_DIR}/src/gui/dialogs
          ${ROOT_DIR}/src/gui/footer
          ${ROOT_DIR}/src/gui/wizard
          ${ROOT_DIR}/src/guiapi/include
          ${ROOT_DIR}/src/guiconfig
          ${ROOT_DIR}/src/lang
          ${ROOT_DIR}/src/hw
          ${CMAKE_CURRENT_SOURCE_DIR}
  )

target_compile_options(firmware PRIVATE -Wdouble-promotion)

target_link_libraries(
  firmware
  PRIVATE A3idesHeaders
          Marlin
          Arduino::Core
          Arduino::TMCStepper
          Arduino::LiquidCrystal
          LwIP
          FatFs
          lpng
          STM32::USBHost
          STM32::USBDevice
          CPU
          inih::inih
          WUI
          jsmn::jsmn
          QR
          Buddy::Lang
          printf::printf
  )

target_sources(
  firmware
  PRIVATE ${ROOT_DIR}/src/startup/startup_stm32f407xx.s
          ${ROOT_DIR}/src/usbd_cdc_if.c
          ${ROOT_DIR}/src/usb_device.c
          ${ROOT_DIR}/src/freertos.c
          ${ROOT_DIR}/src/freertos_openocd.c
          ${ROOT_DIR}/src/usbh_diskio.c
          ${ROOT_DIR}/src/usbd_desc.c
          ${ROOT_DIR}/src/system_stm32f4xx.c
          ${ROOT_DIR}/src/stm32f4xx_hal_msp.c
          ${ROOT_DIR}/src/stm32f4xx_it.c
          ${ROOT_DIR}/src/fatfs.c
          ${ROOT_DIR}/src/usb_host.c
          ${ROOT_DIR}/src/usbh_conf.c
          ${ROOT_DIR}/src/usbd_conf.c
          ${ROOT_DIR}/src/stm32f4xx_hal_timebase_tim.c
          ${ROOT_DIR}/src/common/filament_sensor.cpp
          ${ROOT_DIR}/src/common/filament_sensor_photoeletric.cpp
          ${ROOT_DIR}/src/common/thread_measurement.cpp
          ${ROOT_DIR}/src/common/hardware_serial.cpp
          ${ROOT_DIR}/src/common/diag.cpp
          ${ROOT_DIR}/src/common/eeprom.cpp
          ${ROOT_DIR}/src/common/eeprom_loadsave.c
          ${ROOT_DIR}/src/common/marlin_vars.c
          ${ROOT_DIR}/src/common/media.cpp
          ${ROOT_DIR}/src/common/Pin.cpp
          ${ROOT_DIR}/src/common/PinsCheck.cpp
          ${ROOT_DIR}/src/common/sim_bed.c
          ${ROOT_DIR}/src/common/sys.cpp
          ${ROOT_DIR}/src/common/sim_nozzle.c
          ${ROOT_DIR}/src/common/heap.c
          ${ROOT_DIR}/src/common/hwio_a3ides_2209_02.cpp
          ${ROOT_DIR}/src/common/putslave.cpp
          ${ROOT_DIR}/src/common/safe_state.cpp
          ${ROOT_DIR}/src/common/variant8.cpp
          ${ROOT_DIR}/src/common/dbg.c
          ${ROOT_DIR}/src/common/wdt.cpp
          ${ROOT_DIR}/src/common/dump.c
          ${ROOT_DIR}/src/common/marlin_client.c
          ${ROOT_DIR}/src/common/bsod.cpp
          ${ROOT_DIR}/src/common/adc.c
          ${ROOT_DIR}/src/common/uartslave.c
          ${ROOT_DIR}/src/common/version.c
          ${ROOT_DIR}/src/common/marlin_events.c
          ${ROOT_DIR}/src/common/marlin_errors.c
          ${ROOT_DIR}/src/common/metric.c
          ${ROOT_DIR}/src/common/metric_handlers.c
          ${ROOT_DIR}/src/common/uartrxbuff.c
          ${ROOT_DIR}/src/common/appmain.cpp
          ${ROOT_DIR}/src/common/app_metrics.cpp
          ${ROOT_DIR}/src/common/sim_heater.cpp
          ${ROOT_DIR}/src/common/marlin_server.cpp
          ${ROOT_DIR}/src/common/fsm_types.cpp
          ${ROOT_DIR}/src/common/selftest/selftest_MINI.cpp
          ${ROOT_DIR}/src/common/selftest/selftest_fan.cpp
          ${ROOT_DIR}/src/common/selftest/selftest_axis.cpp
          ${ROOT_DIR}/src/common/selftest/selftest_heater.cpp
          ${ROOT_DIR}/src/common/sim_motion.cpp
          ${ROOT_DIR}/src/common/st25dv64k.c
          ${ROOT_DIR}/src/common/w25x.c
          ${ROOT_DIR}/src/common/gcode_file.cpp
          ${ROOT_DIR}/src/common/gcode_thumb_decoder.cpp
          ${ROOT_DIR}/src/common/print_utils.cpp
          ${ROOT_DIR}/src/common/base64_stream_decoder.cpp
          ${ROOT_DIR}/src/common/sound.cpp
          ${ROOT_DIR}/src/common/language_eeprom.cpp
          ${ROOT_DIR}/src/common/support_utils_lib.cpp
          ${ROOT_DIR}/src/common/support_utils.cpp
          ${ROOT_DIR}/src/common/str_utils.cpp
          ${ROOT_DIR}/src/common/errors.cpp
          ${ROOT_DIR}/src/common/client_response.cpp
          ${ROOT_DIR}/src/common/crc32.c
          ${ROOT_DIR}/src/common/crc32_zlib.c
          ${ROOT_DIR}/src/common/ini_handler.c
          ${ROOT_DIR}/src/common/sha256.c
          ${ROOT_DIR}/src/common/wui_api.c
          ${ROOT_DIR}/src/common/z_calibration_fsm.cpp
          ${ROOT_DIR}/src/common/scratch_buffer.cpp
          ${ROOT_DIR}/src/common/minda_broken_cable_detection.cpp
          ${ROOT_DIR}/src/common/MindaRedscreen.cpp
          ${ROOT_DIR}/src/marlin_stubs/M330.cpp
          ${ROOT_DIR}/src/marlin_stubs/M50.cpp
          ${ROOT_DIR}/src/hw/buffered_serial.cpp
          ${ROOT_DIR}/src/marlin_stubs/M876.cpp
          ${ROOT_DIR}/src/marlin_stubs/G26.cpp
          ${ROOT_DIR}/src/marlin_stubs/G162.cpp
          ${ROOT_DIR}/src/marlin_stubs/gcode.cpp
          ${ROOT_DIR}/src/marlin_stubs/M997.cpp
          ${ROOT_DIR}/src/marlin_stubs/M999.cpp
          ${ROOT_DIR}/src/marlin_stubs/pause/G27.cpp
          ${ROOT_DIR}/src/marlin_stubs/pause/M125.cpp
          ${ROOT_DIR}/src/marlin_stubs/pause/M600.cpp
          ${ROOT_DIR}/src/marlin_stubs/pause/M601_M602.cpp
          ${ROOT_DIR}/src/marlin_stubs/pause/M603.cpp
          ${ROOT_DIR}/src/marlin_stubs/pause/M701_M702.cpp
          ${ROOT_DIR}/src/marlin_stubs/pause/pause.cpp
          ${ROOT_DIR}/src/marlin_stubs/feature/safety_timer.cpp
          ${ROOT_DIR}/src/marlin_stubs/sdcard/M20-M30_M32-M34.cpp
          ${ROOT_DIR}/src/marlin_stubs/host/M115.cpp
          ${ROOT_DIR}/src/common/filament.cpp
          ${ROOT_DIR}/src/common/fanctl.cpp
          ${ROOT_DIR}/src/common/trinamic.cpp
          main.cpp
          mbedtls.c
  )

# Set fsdata filename for HTTPServer
target_compile_definitions(firmware PRIVATE -DHTTPD_FSDATA_FILE="fsdata_wui_local.c")

set_property(
  SOURCE ${ROOT_DIR}/src/common/version.c
  APPEND
  PROPERTY COMPILE_DEFINITIONS FW_BUILD_NUMBER=0 FW_VERSION_FULL=0.0.0 FW_VERSION=0.0.0
           FW_VERSION_SUFFIX="ssl-test" FW_VERSION_SUFFIX_SHORT="sslt"
  )

set_property(
  SOURCE ${ROOT_DIR}/src/common/bsod.cpp ${ROOT_DIR}/src/common/appmain.cpp
         ${ROOT_DIR}/src/common/sys.cpp ${ROOT_DIR}/src/common/putslave.cpp
  APPEND
  PROPERTY COMPILE_DEFINITIONS HAS_GUI=$<BOOL:FALSE>
  )

set_property(
  SOURCE ${ROOT_DIR}/src/common/appmain.cpp ${ROOT_DIR}/src/common/marlin_test.cpp
         ${ROOT_DIR}/src/common/putslave.cpp ${ROOT_DIR}/src/common/thread_measurement.cpp
         ${ROOT_DIR}/src/common/trinamic.cpp
  APPEND
  PROPERTY COMPILE_DEFINITIONS
           INIT_TRINAMIC_FROM_MARLIN_ONLY=$<BOOL:${INIT_TRINAMIC_FROM_MARLIN_ONLY}>
  )
