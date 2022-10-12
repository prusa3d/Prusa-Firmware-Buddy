cmake_minimum_required(VERSION 3.15)
include(cmake/Utilities.cmake)
include(cmake/GetGitRevisionDescription.cmake)
include(cmake/ProjectVersion.cmake)
include(cmake/Littlefs.cmake)
include(cmake/Features.cmake)

project(
  Buddy
  LANGUAGES C CXX ASM
  VERSION ${PROJECT_VERSION}
  )

if(NOT CMAKE_CROSSCOMPILING)
  #
  # If we are not crosscompiling, include `utils` with host tools.
  #
  add_subdirectory(utils)
endif()

#
# Command Line Options
#
# You should specify those options when invoking CMake. Example:
# ~~~
# cmake .. <other options> -DPRINTER=MINI
# ~~~

set(PRINTER_VALID_OPTS "MINI")
set(BOARD_VALID_OPTS "<default>" "BUDDY")
set(MCU_VALID_OPTS "<default>" "STM32F407VG" "STM32F429VI")
set(BOOTLOADER_VALID_OPTS "NO" "EMPTY" "YES")

set(PRINTER
    "MINI"
    CACHE
      STRING
      "Select the printer for which you want to compile the project (valid values are ${PRINTER_VALID_OPTS})."
    )
set(BOOTLOADER
    "NO"
    CACHE STRING "Selects the bootloader mode (valid values are ${BOOTLOADER_VALID_OPTS})."
    )
set(BOARD
    "<default>"
    CACHE
      STRING
      "Select the board for which you want to compile the project (valid values are ${BOARD_VALID_OPTS})."
    )
set(BOARD_VERSION
    "<default>"
    CACHE STRING "Specify the version of the board to comiple the project for (e.g. 1.2.3)"
    )
set(MCU
    "<default>"
    CACHE
      STRING
      "Select the MCU for which you want to compile the project (valid values are ${MCU_VALID_OPTS})."
    )
set(GENERATE_BBF
    "NO"
    CACHE STRING "Whether a .bbf version should be generated."
    )
set(GENERATE_DFU
    "NO"
    CACHE BOOL "Whether a .dfu file should be generated. Implies GENERATE_BBF."
    )
set(SIGNING_KEY
    ""
    CACHE FILEPATH "Path to a PEM EC private key to be used to sign the firmware."
    )
set(PROJECT_VERSION_SUFFIX
    "<auto>"
    CACHE
      STRING
      "Full version suffix to be shown on the info screen in settings (e.g. full_version=4.0.3-BETA+1035.PR111.B4, suffix=-BETA+1035.PR111.B4). Defaults to '+<commit sha>.<dirty?>.<debug?>' if set to '<auto>'."
    )
set(PROJECT_VERSION_SUFFIX_SHORT
    "<auto>"
    CACHE
      STRING
      "Short version suffix to be shown on splash screen. Defaults to '+<BUILD_NUMBER>' if set to '<auto>'."
    )
set(BUILD_NUMBER
    ""
    CACHE STRING "Build number of the firmware. Resolved automatically if not specified."
    )
set(CUSTOM_COMPILE_OPTIONS
    ""
    CACHE STRING "Allows adding custom C/C++ flags"
    )
set(PRESET_COMPILE_OPTIONS
    ""
    CACHE STRING "Allows adding custom C/C++ flags. To be used from preset files."
    )
set(WUI
    "YES"
    CACHE BOOL "Enable Web User Interface"
    )
set(RESOURCES
    "YES"
    CACHE BOOL "Enable resources (managed files on external flash)"
    )

set(CONNECT
    "NO"
    CACHE BOOL "Enable Connect client"
    )
# Validate options
foreach(OPTION "PRINTER" "BOARD" "MCU" "BOOTLOADER")
  if(NOT ${OPTION} IN_LIST ${OPTION}_VALID_OPTS)
    message(FATAL_ERROR "Invalid ${OPTION} ${${OPTION}}: Valid values are ${${OPTION}_VALID_OPTS}")
  endif()
endforeach()

# set board to its default if not specified
if(${BOARD} STREQUAL "<default>")
  if(${PRINTER} STREQUAL "MINI")
    set(BOARD "BUDDY")
  else()
    message(FATAL_ERROR "No default board set for printer ${PRINTER}")
  endif()
endif()

# set board version to its default if not specified
if(${BOARD_VERSION} STREQUAL "<default>")
  if(${BOARD} STREQUAL "BUDDY")
    set(BOARD_VERSION "1.0.0")
  else()
    message(FATAL_ERROR "No default board version set for board ${BOARD}")
  endif()
endif()

# set MCU to its default if not specified
if(${MCU} STREQUAL "<default>")
  set(MCU "STM32F407VG")
endif()

# parse board version into its components
string(REGEX MATCH "^([0-9]+)\\.([0-9]+)\\.([0-9]+)" BOARD_VERSION_MATCH ${BOARD_VERSION})
set(BOARD_VERSION_MAJOR ${CMAKE_MATCH_1})
set(BOARD_VERSION_MINOR ${CMAKE_MATCH_2})
set(BOARD_VERSION_PATCH ${CMAKE_MATCH_3})

# in order to generate DFU file for bootloader, we need a BFU
if(GENERATE_DFU
   AND BOOTLOADER
   OR RESOURCES
   )
  set(GENERATE_BBF "YES")
endif()

# Resolve BUILD_NUMBER and PROJECT_VERSION_* variables
resolve_version_variables()

# Check GCC Version
get_recommended_gcc_version(RECOMMENDED_TOOLCHAIN_VERSION)
if(CMAKE_CROSSCOMPILING AND NOT CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL
                            ${RECOMMENDED_TOOLCHAIN_VERSION}
   )
  message(WARNING "Recommended ARM toolchain is ${RECOMMENDED_TOOLCHAIN_VERSION}"
                  ", but you have ${CMAKE_CXX_COMPILER_VERSION}"
          )

elseif(NOT CMAKE_CROSSCOMPILING AND NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  message(
    WARNING
      "Recommended compiler for host tools and unittests is GCC, you have ${CMAKE_CXX_COMPILER_ID}."
    )
endif()

# Inform user about the resolved settings
message(STATUS "Project version: ${PROJECT_VERSION}")
message(STATUS "Project version with full suffix: ${PROJECT_VERSION_FULL}")
message(
  STATUS "Project version with short suffix: ${PROJECT_VERSION}${PROJECT_VERSION_SUFFIX_SHORT}"
  )
message(STATUS "Using toolchain file: ${CMAKE_TOOLCHAIN_FILE}.")
message(STATUS "Bootloader: ${BOOTLOADER}")
message(STATUS "Printer: ${PRINTER}")
message(STATUS "Board: ${BOARD}")
message(
  STATUS "Board Version: ${BOARD_VERSION_MAJOR}.${BOARD_VERSION_MINOR}.${BOARD_VERSION_PATCH}"
  )
message(STATUS "MCU: ${MCU}")
message(STATUS "Custom Compile Options (C/C++ flags): ${CUSTOM_COMPILE_OPTIONS}")
message(STATUS "Preset Compile Options (C/C++ flags): ${PRESET_COMPILE_OPTIONS}")
message(STATUS "Web User Interface: ${WUI}")
message(STATUS "Resources: ${RESOURCES}")
message(STATUS "Connect client: ${CONNECT}")

# eclipse sets those variables, so lets just use them so we don't get a warning about unused
# variables
set(unused "${CMAKE_VERBOSE_MAKEFILE} ${CMAKE_RULE_MESSAGES}")

# Set printer features
set(PRINTERS_WITH_FILAMENT_SENSOR_YES "MINI")
set(PRINTERS_WITH_INIT_TRINAMIC_FROM_MARLIN_ONLY)
set(PRINTERS_WITH_ADVANCED_PAUSE "MINI")
set(PRINTERS_WITH_BOWDEN_EXTRUDER "MINI")

# Set GUI settings
set(PRINTERS_WITH_GUI "MINI")
set(PRINTERS_WITH_GUI_W240H320 "MINI")

# Set printer board
set(PRINTERS_WITH_BUDDY "MINI")

if(${PRINTER} IN_LIST PRINTERS_WITH_FILAMENT_SENSOR_YES)
  set(FILAMENT_SENSOR YES)
else()
  set(FILAMENT_SENSOR NO)
endif()

if(${PRINTER} IN_LIST PRINTERS_WITH_GUI)
  set(GUI YES)

  if(${PRINTER} IN_LIST PRINTERS_WITH_GUI_W240H320)
    set(RESOLUTION W240H320)
  elseif()
    message(FATAL_ERROR "Printer with GUI must have resolution set")
  endif()
  message(STATUS "RESOLUTION: ${RESOLUTION}")
else()
  set(GUI NO)
endif()
message(STATUS "Graphical User Interface: ${GUI}")

if(${PRINTER} IN_LIST PRINTERS_WITH_INIT_TRINAMIC_FROM_MARLIN_ONLY)
  set(INIT_TRINAMIC_FROM_MARLIN_ONLY YES)
else()
  set(INIT_TRINAMIC_FROM_MARLIN_ONLY NO)
endif()

set(HAS_SELFTEST YES)

set(HAS_MMU2 NO)
message(STATUS "MMU2: ${HAS_MMU2}")

if(${PRINTER} IN_LIST PRINTERS_WITH_ADVANCED_PAUSE)
  set(HAS_PAUSE YES)
else()
  set(HAS_PAUSE NO)
endif()
message(STATUS "ADVANCED PAUSE: ${HAS_PAUSE}")

if(${PRINTER} IN_LIST PRINTERS_WITH_BOWDEN_EXTRUDER)
  set(HAS_BOWDEN YES)
else()
  set(HAS_BOWDEN NO)
endif()
message(STATUS "BOWDEN EXTRUDER: ${HAS_BOWDEN}")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(DEBUG YES)
else()
  set(DEBUG NO)
endif()

# define enabled features
define_feature(BOOTLOADER ${BOOTLOADER})
define_feature(RESOURCES ${RESOURCES})

# enable all warnings (well, not all, but some)
add_compile_options(-Wall -Wsign-compare)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wno-register> $<$<COMPILE_LANGUAGE:CXX>:-std=c++1z>)

# append custom C/C++ flags
if(CUSTOM_COMPILE_OPTIONS)
  string(REPLACE " " ";" CUSTOM_COMPILE_OPTIONS "${CUSTOM_COMPILE_OPTIONS}")
  add_compile_options(${CUSTOM_COMPILE_OPTIONS})
endif()
if(PRESET_COMPILE_OPTIONS)
  string(REPLACE " " ";" PRESET_COMPILE_OPTIONS "${PRESET_COMPILE_OPTIONS}")
  add_compile_options(${PRESET_COMPILE_OPTIONS})
endif()

#
# BuddyHeaders
#
# This library provides headers in the /include directory. When a library requires a configuration
# header, e.g. STM32::USBHost requires usbh_conf.h, we can just place the header to /include and
# then add BuddyHeaders as a dependency to STM32::USBHost.
#
# TODO: Refactor this to make it clear what header files are associated with which targets.
#

add_library(BuddyHeaders INTERFACE)
target_include_directories(
  BuddyHeaders
  INTERFACE include
            include/stm32f4_hal
            include/usb_host
            include/usb_device
            include/marlin
            include/freertos
            ${FEATURES_INCLUDE_DIR}
  )
target_link_libraries(BuddyHeaders INTERFACE STM32F4::HAL FreeRTOS::FreeRTOS)
target_compile_definitions(
  BuddyHeaders
  INTERFACE
  MOTHERBOARD=1823
  MCU=${MCU}
  PRINTER_TYPE=PRINTER_PRUSA_${PRINTER}
  BOARD=BOARD_${BOARD}
  BOARD_VERSION_MAJOR=${BOARD_VERSION_MAJOR}
  BOARD_VERSION_MINOR=${BOARD_VERSION_MINOR}
  BOARD_VERSION_PATCH=${BOARD_VERSION_PATCH}
  MARLIN_DISABLE_INFINITE_LOOP
  PROCESS_CUSTOM_GCODE
  STM32GENERIC
  STM32F4
  HAS_MMU2=$<BOOL:${HAS_MMU2}>
  _EXTUI
  )

add_library(BuddyLogging INTERFACE)
target_include_directories(BuddyLogging INTERFACE src/logging)
target_link_libraries(BuddyHeaders INTERFACE BuddyLogging)

#
# Configure Arduino Core
#

if(${PRINTER} STREQUAL "MINI")
  set(ARDUINO_CORE_VARIANT "2209")
  set(ARDUINO_CORE_TARGET "STM32F4xx")
else()
  message(FATAL_ERROR "Don't know how to configure arduino core with this settings.")
endif()

#
# Configure STMicroelectronics Libraries
#

# STM32F4::HAL
if(${PRINTER} STREQUAL "MINI")
  set(STM32F4_HAL_TARGET "STM32F407xx")
else()
  message(FATAL_ERROR "Don't know how to configure STM32F4::HAL for printer ${PRINTER}")
endif()
add_library(STM32F4_HAL_Config INTERFACE)
target_include_directories(STM32F4_HAL_Config INTERFACE include/stm32f4_hal)

# STM32::USBHost
add_library(STM32_USBHost_Config ALIAS BuddyHeaders)

# STM32::Utilities::CPU
add_library(STM32_Utilities_CPU_Config ALIAS BuddyHeaders)

#
# Configure SEGGER
#
add_library(Segger_Config INTERFACE)
target_include_directories(Segger_Config INTERFACE include/segger src/segger)

#
# Configure FreeRTOS
#

add_library(FreeRTOS_Config INTERFACE)
target_include_directories(FreeRTOS_Config INTERFACE include/freertos)
target_link_libraries(FreeRTOS_Config INTERFACE Segger STM32::CPU)

#
# Configure LwIP
#

add_library(LwIP_Config INTERFACE)
target_link_libraries(LwIP_Config INTERFACE BuddyHeaders BuddyLogging)

#
# Configure FatFs
#

add_library(FatFs_Config INTERFACE)
target_link_libraries(FatFs_Config INTERFACE BuddyHeaders BuddyLogging STM32::USBHost)

#
# Configure Marlin
#

add_library(Marlin_Config INTERFACE)
# TODO: fix dependency on src/common and src/gui
target_include_directories(
  Marlin_Config INTERFACE include/marlin src/common src/common/selftest src/gui
                          src/marlin_stubs/include
  )
target_link_libraries(Marlin_Config INTERFACE BuddyHeaders FreeRTOS::FreeRTOS)

#
# Configure tinyusb
#
add_library(tinyusb_dependencies INTERFACE)
target_include_directories(tinyusb_dependencies INTERFACE include/tinyusb)
target_link_libraries(tinyusb_dependencies INTERFACE FreeRTOS::FreeRTOS)

#
# Global Compiler & Linker Configuration
#

# Thread safe newlib
add_compile_options(-DconfigUSE_NEWLIB_REENTRANT=1)

# include symbols
add_compile_options(-g)

# optimizations
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  if(MCU STREQUAL "STM32F429VI")
    add_compile_options(-O0)
  else() # != "STM32F429VI"
    add_compile_options(-Og)
  endif() # "STM32F429VI"
else() # != "Debug"
  add_compile_options(-Os)
endif() # "Debug"

if(CMAKE_CROSSCOMPILING)
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
endif()

# support _DEBUG macro (some code uses to recognize debug builds)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_definitions(_DEBUG)
endif()

# disable unaligned access
#
# * Otherwise, with optimizations turned on, the firmware crashes on startup.
#
# The main problem was caused by zlib, thus this switch was propagated directly to it, it seems to
# be solved now And let's keep this line commented here for emergency situations ;)
#
# add_compile_options(-mno-unaligned-access)

if(CMAKE_CROSSCOMPILING)
  # configure linker script
  if(BOOTLOADER)
    if(MCU STREQUAL "STM32F429VI")
      set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/src/STM32F429VI_FLASH_boot.ld")
    elseif(MCU STREQUAL "STM32F407VG")
      set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/src/STM32F407VG_FLASH_boot.ld")
    endif()
  else(NOT BOOTLOADER)
    if(MCU STREQUAL "STM32F429VI")
      set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/src/STM32F429VI_FLASH.ld")
    elseif(MCU STREQUAL "STM32F407VG")
      set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/src/STM32F407VG_FLASH.ld")
    endif()
  endif()
  add_link_options("-Wl,-T,${LINKER_SCRIPT}")
endif()

#
# Import definitions of all libraries
#

add_subdirectory(lib)

#
# Buddy firmware
#

add_executable(firmware)

add_subdirectory(src/lang)

if(RESOURCES)
  add_subdirectory(src/resources)
endif()

set_target_properties(firmware PROPERTIES CXX_STANDARD 17)

if(WUI)
  target_compile_definitions(firmware PRIVATE BUDDY_ENABLE_WUI)
endif()

if(CONNECT)
  if(WUI)
    target_sources(
      firmware
      PRIVATE src/Connect/connect.cpp src/Connect/httpc.cpp src/Connect/httpc.cpp
              src/Connect/tls/tls.cpp src/Connect/tls/net_sockets.c src/Connect/tls/hardware_rng.c
      )
    target_compile_definitions(firmware PRIVATE BUDDY_ENABLE_CONNECT)
    # target_link_libraries(firmware PRIVATE Connect)
    target_include_directories(firmware PRIVATE src/Connect src/Connect/tls)
  else()
    message(STATUS "Connect needs WUI!")
  endif()
endif()

# generate firmware.bin file
objcopy(firmware "binary" ".bin")

# generate linker map file
target_link_options(firmware PUBLIC -Wl,-Map=firmware.map)

# inform about the firmware's size in terminal
report_size(firmware)

add_link_dependency(firmware "${LINKER_SCRIPT}")

# generate .bbf version if requested
if(GENERATE_BBF)
  message(STATUS "Configured to generate .bbf version of the firmware.")
  message(STATUS "Signing Key: ${SIGNING_KEY}")

  if(PRINTER STREQUAL "MINI")
    set(PRINTER_TYPE "2")
  else()
    message(FATAL_ERROR "Don't know how to encode printer type for ${PRINTER}.")
  endif()

  if(PROJECT_VERSION_SUFFIX)
    if(NOT "${PROJECT_VERSION_SUFFIX}" MATCHES "\\+${BUILD_NUMBER}")
      message(WARNING "The generated .bbf is configured to use the build number ${BUILD_NUMBER},
      but the version suffix (${PROJECT_VERSION_SUFFIX}) does not contain +${BUILD_NUMBER}.
      Are you sure you know what you are doing?"
              )
    endif()
  endif()

  if(RESOURCES)
    set(pack_fw_resources_opts RESOURCE_IMAGES resources-image resources-bootloader-image
                               RESOURCE_IMAGE_NAMES "RESOURCES_IMAGE" "RESOURCES_BOOTLOADER_IMAGE"
        )
  endif()

  pack_firmware(
    firmware
    FW_VERSION
    "${PROJECT_VERSION_FULL}"
    BUILD_NUMBER
    "${BUILD_NUMBER}"
    PRINTER_TYPE
    "${PRINTER_TYPE}"
    SIGNING_KEY
    "${SIGNING_KEY}"
    BBF_VERSION
    "2"
    ${pack_fw_resources_opts}
    )

  pack_firmware(
    firmware
    FW_VERSION
    "${PROJECT_VERSION_FULL}"
    BUILD_NUMBER
    "${BUILD_NUMBER}"
    PRINTER_TYPE
    "${PRINTER_TYPE}"
    SIGNING_KEY
    "${SIGNING_KEY}"
    BBF_VERSION
    "1"
    OUTPUT_PATH
    "firmware_update_pre_4.4.bbf"
    )
elseif(SIGNING_KEY)
  message(WARNING "SIGNING_KEY specified but BBF generation is not enabled.")
endif()

# generate .dfu version if requested
if(GENERATE_DFU)
  if(BOOTLOADER)
    set(firmware_addr "0x08020000")
    if(BOOTLOADER STREQUAL "YES")
      string(TOLOWER ${PRINTER} printer_low)
      get_dependency_directory("bootloader-${printer_low}" bootloader_dir)
      set(bootloader_input "0x08000000:${bootloader_dir}/bootloader.bin")
    endif()
    set(firmware_input "0x08020000:${CMAKE_BINARY_DIR}/firmware.bbf")
  else()
    set(firmware_input "0x08000000:${CMAKE_BINARY_DIR}/firmware.bin")
  endif()

  create_dfu(
    TARGET
    firmware
    INPUT
    "${bootloader_input}"
    "${firmware_input}"
    OUTPUT
    "${CMAKE_CURRENT_BINARY_DIR}/firmware.dfu"
    )
endif()

target_include_directories(
  firmware
  PRIVATE include
          src/common
          src/common/include
          src/gui
          src/gui/dialogs
          src/gui/footer
          src/gui/wizard
          src/guiapi/include
          src/guiconfig
          src/lang
          src/hw
          src/segger
          src/logging
          src/syslog
          include/mbedtls
  )

target_compile_options(firmware PRIVATE -Wdouble-promotion)

target_link_libraries(
  firmware
  PRIVATE BuddyHeaders
          Marlin
          Arduino::Core
          Arduino::TMCStepper
          Arduino::LiquidCrystal
          LwIP
          FatFs
          flasher
          littlefs
          lpng
          STM32::USBHost
          CPU
          inih::inih
          $<$<BOOL:${WUI}>:WUI>
          jsmn::jsmn
          QR
          Buddy::Lang
          printf::printf
          sysbase
          Segger
          tinyusb::tinyusb
          $<$<BOOL:${HAS_MMU2}>:MMU2::MMU2>
          mbedTLS
  )

if(HAS_SELFTEST)
  target_sources(firmware PRIVATE src/common/selftest/selftest_esp_update.cpp)
endif()

target_sources(
  firmware
  PRIVATE $<$<IN_LIST:${BOOTLOADER},YES;EMPTY>:src/startup/startup_stm32f407xx_boot.s>
          $<$<STREQUAL:${BOOTLOADER},NO>:src/startup/startup_stm32f407xx.s>
          src/usb_device.c
          src/usb_device_cdc.cpp
          src/usb_device_msc.c
          src/freertos.c
          src/freertos_openocd.c
          src/lwip.c
          src/usbh_diskio.c
          src/main.cpp
          $<$<IN_LIST:${BOOTLOADER},YES;EMPTY>:src/system_stm32f4xx_boot.c>
          $<$<STREQUAL:${BOOTLOADER},NO>:src/system_stm32f4xx.c>
          src/stm32f4xx_hal_msp.c
          src/stm32f4xx_it.c
          src/common/bbf.cpp
          src/fatfs.c
          src/littlefs_internal.c
          src/littlefs_bbf.cpp
          src/filesystem.c
          src/filesystem_fatfs.c
          src/filesystem_littlefs.c
          src/filesystem_littlefs_internal.c
          src/filesystem_littlefs_bbf.cpp
          src/filesystem_root.c
          src/filesystem_semihosting.cpp
          src/semihosting/semihosting.cpp
          src/libsysbase_syscalls.c
          src/usb_host.c
          src/usbh_conf.c
          src/common/freertos_mutex.cpp
          src/common/non_file_printing_counter.cpp
          src/common/print_processor.cpp
          $<$<BOOL:${BOOTLOADER}>:src/bootloader/bootloader.cpp>
          $<$<BOOL:${FILAMENT_SENSOR}>:src/common/filament_sensor.cpp>
          $<$<BOOL:${FILAMENT_SENSOR}>:src/common/filament_sensor_api.cpp>
          $<$<NOT:$<BOOL:${HAS_MMU2}>>:src/common/filament_sensor_api_no_mmu.cpp>
          $<$<STREQUAL:${FILAMENT_SENSOR},ADC>:src/common/filament_sensor_adc.cpp>
          $<$<STREQUAL:${FILAMENT_SENSOR},YES>:src/common/filament_sensor_photoeletric.cpp>
          $<$<BOOL:${FILAMENT_SENSOR}>:src/common/thread_measurement.cpp>
          src/common/adc.cpp
          src/common/basename.c
          src/common/lfn.cpp
          src/common/i2c.cpp
          src/common/hardware_serial.cpp
          src/common/eeprom.cpp
          src/common/eeprom_loadsave.c
          src/common/ExtInterruptHandler.cpp
          src/common/MarlinPin.cpp
          src/common/marlin_vars.c
          src/common/media.cpp
          src/common/otp.c
          src/common/Pin.cpp
          src/common/SteelSheets.cpp
          src/common/PinsCheck.cpp
          src/common/sim_bed.c
          src/common/sys.cpp
          src/common/sim_nozzle.c
          src/common/heap.c
          src/common/stubs.c
          src/common/hwio_buddy_2209_02.cpp
          src/common/safe_state.cpp
          src/common/variant8.cpp
          src/common/wdt.cpp
          src/common/dump.c
          src/common/marlin_client.c
          $<$<BOOL:${GUI}>:src/common/bsod_gui.cpp>
          $<$<NOT:$<BOOL:${GUI}>>:src/common/bsod_nogui.cpp>
          src/common/version.c
          src/common/marlin_events.c
          src/common/marlin_errors.c
          src/common/metric.c
          src/common/metric_handlers.c
          src/common/uartrxbuff.c
          src/common/appmain.cpp
          src/common/app_metrics.cpp
          src/common/sim_heater.cpp
          src/common/marlin_server.cpp
          src/common/fsm_types.cpp
          src/common/jsmn.c
          src/common/selftest/selftest_MINI.cpp
          src/common/selftest/selftest_fan.cpp
          src/common/selftest/selftest_axis.cpp
          src/common/selftest/selftest_heater.cpp
          src/common/st25dv64k.c
          src/common/w25x.c
          src/common/w25x_communication.cpp
          src/common/gcode_file.cpp
          src/common/gcode_filename.cpp
          src/common/gcode_filter.cpp
          src/common/gcode_info.cpp
          src/common/gcode_thumb_decoder.cpp
          src/common/print_utils.cpp
          src/common/base64_stream_decoder.cpp
          src/common/sound.cpp
          src/common/language_eeprom.cpp
          src/common/support_utils_lib.cpp
          src/common/support_utils.cpp
          src/common/str_utils.cpp
          src/common/errors.cpp
          src/common/client_response.cpp
          src/common/client_response_texts.cpp
          src/common/crc32.c
          src/common/crc32_zlib.c
          src/common/ini_handler.c
          src/common/z_calibration_fsm.cpp
          src/common/scratch_buffer.cpp
          src/common/odometer.cpp
          src/common/footer_eeprom.cpp
          $<$<STREQUAL:${PRINTER},MINI>:src/common/minda_broken_cable_detection.cpp>
          $<$<STREQUAL:${PRINTER},MINI>:src/common/MindaRedscreen.cpp>
          src/common/Z_probe.cpp
          src/hw/buffered_serial.cpp
          src/hw/main_check.cpp
          src/marlin_stubs/gcode.cpp
          src/marlin_stubs/G26.cpp
          src/marlin_stubs/G162.cpp
          src/marlin_stubs/M50.cpp
          src/marlin_stubs/M300.cpp
          src/marlin_stubs/M330.cpp
          src/marlin_stubs/M505.cpp
          src/marlin_stubs/M876.cpp
          src/marlin_stubs/M997.cpp
          src/marlin_stubs/M999.cpp
          $<$<BOOL:${WUI}>:src/marlin_stubs/M1587.cpp>
          src/marlin_stubs/pause/G27.cpp
          src/marlin_stubs/pause/M125.cpp
          $<$<BOOL:${HAS_PAUSE}>:src/marlin_stubs/pause/M600.cpp>
          $<$<BOOL:${HAS_PAUSE}>:src/marlin_stubs/pause/M601_2.cpp>
          src/marlin_stubs/pause/M603.cpp
          $<$<BOOL:${HAS_PAUSE}>:src/marlin_stubs/pause/M701_2.cpp>
          $<$<BOOL:${HAS_PAUSE}>:src/marlin_stubs/pause/M701_2_parse.cpp>
          $<$<BOOL:${HAS_MMU2}>:src/marlin_stubs/pause/M704-6_parse.cpp>
          $<$<BOOL:${HAS_MMU2}>:src/marlin_stubs/pause/M707-9_parse.cpp>
          $<$<BOOL:${HAS_MMU2}>:src/marlin_stubs/pause/M707-9_parse.cpp>
          $<$<BOOL:${HAS_PAUSE}>:src/marlin_stubs/pause/M70X_preheat.cpp>
          $<$<BOOL:${HAS_PAUSE}>:src/marlin_stubs/pause/M1700_1_parse.cpp>
          $<$<BOOL:${HAS_PAUSE}>:src/marlin_stubs/pause/pause.cpp>
          $<$<BOOL:${HAS_PAUSE}>:src/marlin_stubs/pause/pause_settings.cpp>
          src/marlin_stubs/feature/safety_timer.cpp
          src/marlin_stubs/sdcard/M20-M30_M32-M34.cpp
          src/marlin_stubs/host/M115.cpp
          src/gui/guimain.cpp
          src/common/filament.cpp
          src/common/fanctl.cpp
          src/common/trinamic.cpp
          src/common/timing.c
          src/common/timing_sys.c
          src/segger/SEGGER_SYSVIEW_FreeRTOS.c
          src/segger/SEGGER_SYSVIEW_Config_FreeRTOS.c
          src/syslog/syslog.c
          src/logging.c
          src/logging/log.c
          src/logging/log_platform.c
          src/logging/log_dest_swo.c
          src/logging/log_dest_rtt.c
          src/logging/log_dest_shared.c
          src/logging/log_dest_syslog.c
          src/logging/log_dest_usb.c
  )

if(GUI)
  if(HAS_SELFTEST)
    target_sources(
      firmware
      PRIVATE src/gui/wizard/selftest_frame_esp.cpp src/gui/wizard/selftest_frame_esp_progress.cpp
              src/gui/wizard/selftest_frame_esp_qr.cpp src/gui/wizard/selftest_invalid_state.cpp
              src/gui/ScreenSelftest.cpp src/gui/selftest_frame.cpp
      )
  else()
    target_sources(firmware PRIVATE src/gui/screen_menu_calibration_without_selftest.cpp)
  endif()
  target_sources(
    firmware
    PRIVATE src/gui/screen_menu_filament.cpp
            src/gui/screen_menu_calibration.cpp
            src/gui/screen_menu_settings.cpp
            src/gui/screen_menu_info.cpp
            src/gui/screen_menu_hw_setup.cpp
            src/gui/screen_menu_steel_sheets.cpp
            src/gui/screen_menu_footer_settings.cpp
            src/gui/screen_menu_lan_settings.cpp
            src/gui/screen_menu_languages.cpp
            src/gui/screen_menu_sensor_info.cpp
            src/gui/screen_menu_odometer.cpp
            src/gui/screen_menu_move.cpp
            src/gui/screen_menu_odometer.cpp
            src/gui/screen_menu_service.cpp
            src/gui/screen_menu_eeprom_diagnostics.cpp
            src/gui/screen_menu_temperature.cpp
            src/gui/screen_menu_tune.cpp
            src/gui/screen_menu_fw_update.cpp
            src/gui/screen_menu_eeprom.cpp
            src/gui/screen_menu_network.cpp
            src/gui/dialogs/IDialog.cpp
            src/gui/dialogs/DialogFactory.cpp
            src/gui/dialogs/DialogHandler.cpp
            src/gui/dialogs/DialogStateful.cpp
            src/gui/dialogs/DialogRadioButton.cpp
            src/gui/dialogs/DialogLoadUnload.cpp
            src/gui/dialogs/DialogG162.cpp
            src/gui/dialogs/DialogSelftestAxis.cpp
            src/gui/dialogs/DialogSelftestFans.cpp
            src/gui/dialogs/DialogSelftestTemp.cpp
            src/gui/dialogs/DialogSelftestResult.cpp
            src/gui/dialogs/window_dlg_load_unload.cpp
            src/gui/dialogs/window_dlg_wait.cpp
            src/gui/dialogs/window_dlg_popup.cpp
            src/gui/dialogs/window_dlg_strong_warning.cpp
            src/gui/dialogs/window_dlg_preheat.cpp
            src/gui/dialogs/liveadjust_z.cpp
            src/gui/dialogs/window_dlg_calib_z.cpp
            src/gui/footer/footer_icon.cpp
            src/gui/footer/footer_text.cpp
            src/gui/footer/ifooter_item.cpp
            src/gui/footer/ifooter_string_item.cpp
            src/gui/footer/footer_items_heaters.cpp
            src/gui/footer/footer_item_filament.cpp
            src/gui/footer/footer_item_sheet_profile.cpp
            src/gui/footer/footer_item_printspeed.cpp
            src/gui/footer/footer_item_live_z.cpp
            src/gui/footer/footer_items_nozzle_bed.cpp
            src/gui/footer/footer_item_fsensor.cpp
            src/gui/footer/footer_line.cpp
            src/gui/footer/ifooter.cpp
            $<$<STREQUAL:${PRINTER},MINI>:src/gui/footer/footer_doubleline.cpp>
            src/gui/ScreenFactory.cpp
            src/gui/ScreenHandler.cpp
            src/gui/gui_media_events.cpp
            src/gui/resource.c
            src/gui/gui_time.cpp
            src/gui/window_print_progress.cpp
            src/gui/window_file_list.cpp
            src/gui/window_temp_graph.cpp
            src/gui/window_header.cpp
            src/gui/screen_sysinf.cpp
            src/gui/version_info_ST7789V.cpp
            src/gui/test_display.cpp
            src/gui/screen_splash.cpp
            src/gui/screen_menu.cpp
            src/gui/screen_print_preview.cpp
            src/gui/gcode_description.cpp
            src/gui/screen_messages.cpp
            src/gui/screen_watchdog.cpp
            src/gui/screen_reset_error.cpp
            src/gui/screen_hardfault.cpp
            src/gui/screen_temperror.cpp
            src/gui/dialogs/DialogMoveZ.cpp
            src/gui/screen_qr_error.cpp
            src/gui/screen_sheet_rename.cpp
            src/gui/screen_prusa_link.cpp
            src/gui/wizard/selftest.cpp
            src/gui/wizard/firstlay.cpp
            src/gui/wizard/screen_wizard.cpp
            src/gui/wizard/xyzcalib.cpp
            src/gui/screen_home.cpp
            src/gui/screen_filebrowser.cpp
            src/gui/menu_vars.cpp
            src/gui/IScreenPrinting.cpp
            src/gui/ScreenFirstLayer.cpp
            src/gui/ScreenPrintingModel.cpp
            src/gui/screen_printing.cpp
            src/gui/screen_printing_serial.cpp
            src/gui/footer/footer_item_axis.cpp
            src/gui/footer/footer_item_fans.cpp
            src/gui/logger.c
            src/gui/MItem_menus.cpp
            src/gui/MItem_print.cpp
            src/gui/MItem_tools.cpp
            src/gui/MItem_filament.cpp
            src/gui/MItem_eeprom.cpp
            src/gui/MItem_lan.cpp
            src/gui/MItem_experimental_tools.cpp
            src/gui/network_gui_tools.cpp
            src/guiapi/src/knob_event.cpp
            src/guiapi/src/gui.cpp
            src/guiapi/src/term.cpp
            src/guiapi/src/guitypes.cpp
            src/guiapi/src/Rect16.cpp
            src/guiapi/src/window_progress.cpp
            src/guiapi/src/window_frame.cpp
            src/guiapi/src/screen.cpp
            src/guiapi/src/gui_timer.cpp
            src/guiapi/src/display_helper.cpp
            src/guiapi/src/text_roll.cpp
            src/guiapi/src/window_menu.cpp
            src/guiapi/src/window_term.cpp
            src/guiapi/src/window_arrows.cpp
            src/guiapi/src/window_numb.cpp
            src/guiapi/src/st7789v.cpp
            src/guiapi/src/display_ex.cpp
            src/guiapi/src/window_icon.cpp
            src/guiapi/src/i_window_text.cpp
            src/guiapi/src/window_text.cpp
            src/guiapi/src/window_roll_text.cpp
            src/guiapi/src/window_msgbox.cpp
            src/guiapi/src/window_qr.cpp
            src/guiapi/src/window.cpp
            src/guiapi/src/window_event.cpp
            src/guiapi/src/IWindowMenuItem.cpp
            src/guiapi/src/WindowMenuSwitch.cpp
            src/guiapi/src/WindowMenuSpin.cpp
            src/guiapi/src/WindowMenuInfo.cpp
            src/guiapi/src/WindowMenuSpinExponential.cpp
            src/guiapi/src/WindowMenuItems.cpp
            src/guiapi/src/Jogwheel.cpp
            src/guiapi/src/ScreenShot.cpp
            src/guiapi/src/menu_spin_config_types.cpp
            $<$<STREQUAL:${PRINTER},MINI>:src/guiapi/src/menu_spin_config_basic.cpp>
    )
endif()

if(GUI AND DEBUG) # gui and debug
  target_sources(
    firmware
    PRIVATE src/gui/screen_menu_experimental_settings_debug.cpp
            src/gui/test/screen_test.cpp
            src/gui/test/screen_test_disp_mem.cpp
            src/gui/test/screen_test_gui.cpp
            src/gui/test/screen_test_term.cpp
            src/gui/test/screen_test_msgbox.cpp
            src/gui/test/screen_test_dlg.cpp
            src/gui/test/screen_test_wizard_icons.cpp
            src/gui/test/screen_test_graph.cpp
            src/gui/test/screen_menu_eeprom_test.cpp
            src/gui/test/screen_menu_test_temperature.cpp
            src/gui/test/screen_mesh_bed_lv.cpp
    )
elseif(GUI) # gui, not debug
  target_sources(firmware PRIVATE src/gui/screen_menu_experimental_settings_mini.cpp)
endif()

set_property(
  SOURCE src/common/version.c
  APPEND
  PROPERTY COMPILE_DEFINITIONS
           FW_BUILD_NUMBER=${BUILD_NUMBER}
           FW_VERSION_FULL=${PROJECT_VERSION_FULL}
           FW_VERSION=${PROJECT_VERSION}
           FW_VERSION_SUFFIX=${PROJECT_VERSION_SUFFIX}
           FW_VERSION_SUFFIX_SHORT=${PROJECT_VERSION_SUFFIX_SHORT}
  )

set_property(
  SOURCE src/main.cpp
         src/common/appmain.cpp
         src/common/sys.cpp
         src/marlin_stubs/M50.cpp
         src/marlin_stubs/gcode.cpp
         src/common/hwio_buddy_2209_02.cpp
         src/gui/guimain.cpp
  APPEND
  PROPERTY COMPILE_DEFINITIONS HAS_GUI=$<BOOL:${GUI}>
  )

set_property(
  SOURCE src/common/appmain.cpp src/common/marlin_test.cpp src/common/thread_measurement.cpp
         src/common/trinamic.cpp
  APPEND
  PROPERTY COMPILE_DEFINITIONS
           INIT_TRINAMIC_FROM_MARLIN_ONLY=$<BOOL:${INIT_TRINAMIC_FROM_MARLIN_ONLY}>
  )

set_property(
  SOURCE src/gui/dialogs/DialogHandler.cpp
  APPEND
  PROPERTY COMPILE_DEFINITIONS HAS_SELFTEST=$<BOOL:${HAS_SELFTEST}>
  )

set_property(
  SOURCE src/main.cpp
         src/gui/screen_menu_settings.cpp
         src/gui/screen_menu_footer_settings.cpp
         src/gui/footer/footer_line.cpp
         src/gui/test/screen_test_load.cpp
         src/common/marlin_server.cpp
         src/marlin_stubs/pause/pause.cpp
         src/gui/dialogs/DialogLoadUnload.cpp
         src/gui/guimain.cpp
  APPEND
  PROPERTY COMPILE_DEFINITIONS HAS_MMU2=$<BOOL:${HAS_MMU2}>
  )

set_property(
  SOURCE src/marlin_stubs/pause/pause.cpp src/marlin_stubs/pause/M70X.cpp
         src/marlin_stubs/pause/M701_2.cpp
  APPEND
  PROPERTY COMPILE_DEFINITIONS HAS_BOWDEN=$<BOOL:${HAS_BOWDEN}>
  )

# Due to the wrong use of interface library for WUI, the flag "GENERATED" is not properly propagated
# and cmake fails. This sets it manually and makes it work.
#
# Newer cmake (3.20) should fix this and can be removed.
#
# https://github.com/prusa3d/Prusa-Firmware-Buddy/pull/1790#issuecomment-1010525282
set_property(SOURCE ${CMAKE_BINARY_DIR}/lib/WUI/http_req_automaton.cpp PROPERTY GENERATED 1)

if(NOT CMAKE_CROSSCOMPILING)
  enable_testing()
  add_subdirectory(tests)
endif()
