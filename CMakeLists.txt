cmake_minimum_required(VERSION 3.15)
include(cmake/Utilities.cmake)
include(cmake/GetGitRevisionDescription.cmake)
include(cmake/ProjectVersion.cmake)

project(
  Buddy
  LANGUAGES C CXX ASM
  VERSION ${PROJECT_VERSION}
  )

if(NOT CMAKE_CROSSCOMPILING)
  #
  # If we are not crosscompiling, include `utils` with host tools and exit.
  #
  add_subdirectory(utils)
  return()
endif()

#
# Command Line Options
#
# You should specify those options when invoking CMake. Example:
# ~~~
# cmake .. <other options> -DPRINTER=MINI
# ~~~

set(PRINTER_VALID_OPTS "MINI")
set(MOTHERBOARD_VALID_OPTS "1823")
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
set(MOTHERBOARD
    "1823"
    CACHE
      STRING
      "Select the motherboard for which you want to compile the project (valid values are ${MOTHERBOARD_VALID_OPTS})."
    )
set(GENERATE_BBF
    "NO"
    CACHE STRING "Whether a .bbf version should be generated."
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

# Validate options
foreach(OPTION "PRINTER" "MOTHERBOARD" "BOOTLOADER")
  if(NOT ${OPTION} IN_LIST ${OPTION}_VALID_OPTS)
    message(FATAL_ERROR "Invalid ${OPTION} ${${OPTION}}: Valid values are ${${OPTION}_VALID_OPTS}")
  endif()
endforeach()

# Resolve BUILD_NUMBER and PROJECT_VERSION_* variables
resolve_version_variables()

# Check GCC Version
get_recommended_gcc_version(RECOMMENDED_TOOLCHAIN_VERSION)
if(NOT CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL ${RECOMMENDED_TOOLCHAIN_VERSION})
  message(WARNING "Recommended ARM toolchain is ${RECOMMENDED_TOOLCHAIN_VERSION}"
                  ", but you have ${CMAKE_CXX_COMPILER_VERSION}"
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

# eclipse sets those variables, so lets just use them so we don't get a warning about unused
# variables
set(unused "${CMAKE_VERBOSE_MAKEFILE} ${CMAKE_RULE_MESSAGES}")

#
# A3idesHeaders
#
# This library provides headers in the /include directory. When a library requires a configuration
# header, e.g. STM32::USBHost requires usbh_conf.h, we can just place the header to /include and
# then add A3idesHeaders as a dependency to STM32::USBHost.
#
# TODO: Refactor this to make it clear what header files are associated with which targets.
#

add_library(A3idesHeaders INTERFACE)
target_include_directories(
  A3idesHeaders INTERFACE include include/stm32f4_hal include/usb_host include/usb_device
                          include/marlin include/freertos
  )
target_link_libraries(A3idesHeaders INTERFACE STM32F4::HAL FreeRTOS::FreeRTOS)
target_compile_definitions(
  A3idesHeaders
  INTERFACE
  MOTHERBOARD=${MOTHERBOARD}
  PRINTER_TYPE=PRINTER_PRUSA_${PRINTER}
  MARLIN_DISABLE_INFINITE_LOOP
  STM32GENERIC
  STM32F4
  _EXTUI
  )

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
add_library(STM32_USBHost_Config ALIAS A3idesHeaders)

# STM32::USBDevice
add_library(STM32_USBDevice_Config ALIAS A3idesHeaders)

# STM32::Utilities::CPU
add_library(STM32_Utilities_CPU_Config ALIAS A3idesHeaders)

#
# Configure FreeRTOS
#

add_library(FreeRTOS_Config INTERFACE)
target_include_directories(FreeRTOS_Config INTERFACE include/freertos)

#
# Configure LwIP
#

set(LWIP_HTTPD_CUSTOM_FILES "1")
add_library(LwIP_Config ALIAS A3idesHeaders)

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
target_include_directories(Marlin_Config INTERFACE include/marlin src/common src/gui)
target_link_libraries(Marlin_Config INTERFACE A3idesHeaders FreeRTOS::FreeRTOS)

#
# Global Compiler & Linker Configuration
#

# mcu related settings
set(MCU_FLAGS -mthumb -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16)
add_compile_options(${MCU_FLAGS})
add_link_options(${MCU_FLAGS})

# include symbols
add_compile_options(-g)

# optimizations
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_options(-Og)
else()
  add_compile_options(-Os)
endif()

# better FreeRTOS support
add_link_options(-Wl,--undefined=uxTopUsedPriority,--undefined=init_task)

# split and gc sections
add_compile_options(-ffunction-sections -fdata-sections)
add_link_options(-Wl,--gc-sections)

# enable all warnings (well, not all, but some)
add_compile_options(-Wall)

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

# configure linker script
if(BOOTLOADER)
  set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/src/STM32F407VG_FLASH_boot.ld")
else(NOT BOOTLOADER)
  set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/src/STM32F407VG_FLASH.ld")
endif()
add_link_options("-Wl,-T,${LINKER_SCRIPT}")

#
# Import definitions of all libraries
#

add_subdirectory(lib)

#
# A3ides firmware
#

add_executable(firmware)

set_target_properties(firmware PROPERTIES CXX_STANDARD 11)

# generate firmware.bin file
objcopy(firmware "binary" ".bin")

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

  pack_firmware(
    firmware "${PROJECT_VERSION_FULL}" "${BUILD_NUMBER}" "${PRINTER_TYPE}" "${SIGNING_KEY}"
    )
elseif(SIGNING_KEY)
  message(WARNING "SIGNING_KEY specified but BBF generation is not enabled.")
endif()

target_include_directories(
  firmware PRIVATE include src/common src/gui src/gui/dialogs src/guiapi/include src/wui
  )

target_compile_options(firmware PRIVATE -Wdouble-promotion)

# generate datetime string (RFC1123) to be used by web ui
rfc1123_datetime(RFC1123_DATETIME)
target_compile_definitions(firmware PRIVATE RFC1123_DATETIME="$RFC1123_DATETIME")

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
  )

target_sources(
  firmware
  PRIVATE $<$<IN_LIST:${BOOTLOADER},YES;EMPTY>:src/startup/startup_stm32f407xx_boot.s>
          $<$<STREQUAL:${BOOTLOADER},NO>:src/startup/startup_stm32f407xx.s>
          src/lwip.c
          src/usbd_cdc_if.c
          src/usb_device.c
          src/freertos.c
          src/freertos_openocd.c
          src/usbh_diskio.c
          src/ethernetif.c
          src/main.c
          src/usbd_desc.c
          src/fs_custom.c
          $<$<IN_LIST:${BOOTLOADER},YES;EMPTY>:src/system_stm32f4xx_boot.c>
          $<$<STREQUAL:${BOOTLOADER},NO>:src/system_stm32f4xx.c>
          src/stm32f4xx_hal_msp.c
          src/stm32f4xx_it.c
          src/fatfs.c
          src/usb_host.c
          src/usbh_conf.c
          src/usbd_conf.c
          src/stm32f4xx_hal_timebase_tim.c
          src/common/filament_sensor.c
          src/common/thread_measurement.c
          src/common/diag.c
          src/common/eeprom.c
          src/common/marlin_vars.c
          src/common/marlin_host.c
          src/common/sim_bed.c
          src/common/sys.cpp
          src/common/sim_nozzle.c
          src/common/hwio_a3ides_2209_02.c
          src/common/putslave.c
          src/common/safe_state.c
          src/common/variant8.c
          src/common/dbg.c
          src/common/marlin_client.c
          src/common/bsod.c
          src/common/adc.c
          src/common/uartslave.c
          src/common/version.c
          src/common/marlin_events.c
          src/common/marlin_errors.c
          src/common/uartrxbuff.c
          src/common/Marlin_CardReader.cpp
          src/common/appmain.cpp
          src/common/sim_heater.cpp
          src/common/marlin_server.cpp
          src/common/sim_motion.cpp
          src/common/st25dv64k.c
          src/common/trinamic.cpp
          src/common/trinamic.h
          src/common/w25x.c
          src/common/gcode_file.cpp
          src/common/gcode_thumb_decoder.cpp
          src/common/print_utils.cpp
          src/common/Marlin_eeprom.cpp
          src/common/base64_stream_decoder.cpp
          src/gui/screen_menu_filament.c
          src/gui/dialogs/window_dlg_change.c
          src/gui/dialogs/window_dlg_loadunload_shared.c
          src/gui/dialogs/window_dlg_statemachine.c
          src/gui/dialogs/window_dlg_load.c
          src/gui/dialogs/window_dlg_wait.c
          src/gui/dialogs/window_dlg_popup.c
          src/gui/dialogs/window_dlg_purge.c
          src/gui/dialogs/window_dlg_preheat.c
          src/gui/dialogs/window_dlg_unload.c
          src/gui/screen_menu_temperature.c
          src/gui/resource.c
          src/gui/window_file_list.c
          src/gui/pngview.c
          src/gui/guimain.c
          src/gui/wizard/selftest_temp.c
          src/gui/window_temp_graph.c
          src/gui/screen_menu_settings.c
          src/gui/wizard/wizard_progress_bar.c
          src/gui/wizard/firstlay.c
          src/gui/wizard/selftest_home.c
          src/gui/wizard/selftest_cool.c
          src/gui/window_header.c
          src/gui/screen_sysinf.c
          src/gui/screen_version_info.cpp
          src/gui/test_display.c
          src/gui/screen_splash.c
          src/gui/window_logo.c
          src/gui/screen_menu_service.c
          src/gui/screen_menu_calibration.c
          src/gui/screen_menu.c
          src/gui/screen_print_preview.c
          src/gui/screen_messages.c
          src/gui/screen_menu_info.c
          src/gui/screen_watchdog.c
          src/gui/screen_menu_move.c
          src/gui/screen_lan_settings.c
          src/gui/test/screen_test.c
          src/gui/test/screen_test_disp_mem.c
          src/gui/test/screen_test_gui.c
          src/gui/test/screen_test_term.c
          src/gui/test/screen_test_msgbox.c
          src/gui/test/screen_test_graph.c
          src/gui/wizard/selftest_fans_axis.c
          src/gui/wizard/selftest_home.c
          src/gui/wizard/selftest_temp.c
          src/gui/wizard/selftest_cool.c
          src/gui/wizard/selftest.c
          src/gui/wizard/firstlay.c
          src/gui/wizard/wizard_load_unload.c
          src/gui/wizard/wizard_ui.c
          src/gui/wizard/wizard.c
          src/gui/wizard/screen_wizard.c
          src/gui/wizard/xyzcalib.c
          src/gui/wizard/wizard_progress_bar.c
          src/gui/screen_home.cpp
          src/gui/screen_filebrowser.cpp
          src/gui/screen_menu_preheat.cpp
          src/gui/status_footer.cpp
          src/gui/filament.cpp
          src/gui/menu_vars.cpp
          src/gui/screen_printing.cpp
          src/gui/screen_menu_tune.cpp
          src/gui/screen_PID.cpp
          src/gui/screen_menu_fw_update.c
          src/gui/test/screen_test_temperature.cpp
          src/gui/wizard/selftest_fans_axis.c
          src/gui/test/screen_mesh_bed_lv.cpp
          src/gui/wizard/Marlin_PID_wrapper.cpp
          src/guiapi/src/button_draw.c
          src/guiapi/src/display.c
          src/guiapi/src/gui.c
          src/guiapi/src/term.c
          src/guiapi/src/guitypes.c
          src/guiapi/src/window_progress.c
          src/guiapi/src/window_frame.c
          src/guiapi/src/gui_timer.c
          src/guiapi/src/display_helper.c
          src/guiapi/src/window_menu.c
          src/guiapi/src/screen.c
          src/guiapi/src/window_term.c
          src/guiapi/src/window_list.c
          src/guiapi/src/jogwheel.c
          src/guiapi/src/window_numb.c
          src/guiapi/src/st7789v.c
          src/guiapi/src/window_icon.c
          src/guiapi/src/window_spin.c
          src/guiapi/src/window_text.c
          src/guiapi/src/window_msgbox.c
          src/guiapi/src/window.c
          src/marlin_stubs/M600.cpp
          src/wui/http_states.c
          src/wui/lwsapi.cpp
          src/wui/connect.cpp
          src/wui/connection.cpp
  )

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
