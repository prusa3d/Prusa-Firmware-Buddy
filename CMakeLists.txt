cmake_minimum_required(VERSION 3.22)
include(ExternalProject)
include(cmake/Utilities.cmake)
include(cmake/GetGitRevisionDescription.cmake)
include(cmake/ProjectVersion.cmake)
include(cmake/Options.cmake)
include(cmake/CCache.cmake)
include(cmake/AddPuppyFirmware.cmake)
include(cmake/Sha256.cmake)
include(cmake/Tar.cmake)

# Needded for nunavut
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
set_property(GLOBAL PROPERTY EXCLUDE_FROM_ALL ON)

project(
  Buddy
  LANGUAGES C CXX ASM
  VERSION ${PROJECT_VERSION}
  )

# Sets Python3_FIND_STRATEGY to "LOCATION"
cmake_policy(SET CMP0094 NEW)

# Enable initialization of subprojects without their submodules
cmake_policy(SET CMP0097 NEW)

if(NOT CMAKE_CROSSCOMPILING)
  option(UNITTESTS_ENABLE "Enable building of unittest" ON)
  option(COVERAGE_ENABLE "Enable code coverage instrumentation for tests" OFF)
  if(COVERAGE_ENABLE)
    add_compile_options(--coverage -fno-inline)
    add_link_options(--coverage)
  endif()
endif()
include(ProjectOptions.cmake)

# link time optimizations to disable LTO run cmake with -DDISABLE_LTO
if(NOT DEFINED DISABLE_LTO
   AND NOT BOARD STREQUAL DWARF
   AND CMAKE_CROSSCOMPILING # This will not force LTO for natively built unittests
   )
  set(COMPILE_WITH_LTO ON)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
  message(STATUS "LTO: Enabled")
else()
  set(COMPILE_WITH_LTO OFF)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)
  message(STATUS "LTO: Disabled")
endif()

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

# eclipse sets those variables, so lets just use them so we don't get a warning about unused
# variables
set(unused "${CMAKE_VERBOSE_MAKEFILE} ${CMAKE_RULE_MESSAGES}")
set(CMAKE_CXX_STANDARD 23)

# enable all warnings (well, not all, but some)
add_compile_options(-Wall -Wsign-compare -Wfloat-conversion)

add_compile_options(-Wno-deprecated-declarations)

# make all paths stored inside firmware relative
add_compile_options(-ffile-prefix-map=${CMAKE_SOURCE_DIR}=.)

# make the builds reproducible
add_compile_options(-frandom-seed=BuddyFW)

# Do not allow non-ascii identifiers and bidi shenanigans
add_compile_options(-fno-extended-identifiers -Wbidi-chars=any)

add_compile_options(
  $<$<COMPILE_LANGUAGE:CXX>:-Wno-register> $<$<COMPILE_LANGUAGE:CXX>:-Wno-volatile>
  )

if(NOT UNITTESTS_ENABLE)
  add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>)
elseif(CMAKE_CROSSCOMPILING)
  message(FATAL_ERROR "RTTI should be disabled when crosscompiling")
endif()

# append custom C/C++ flags
if(CUSTOM_COMPILE_OPTIONS)
  string(REPLACE " " ";" EXTRA_COMPILE_OPTIONS "${CUSTOM_COMPILE_OPTIONS}")
  add_compile_options(${EXTRA_COMPILE_OPTIONS})
endif()

# find bootloader.bin location
if(BOOTLOADER STREQUAL "YES")
  string(TOLOWER ${PRINTER} printer_low)
  set(BOOTLOADER_VARIANT "${printer_low}")
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
            include/usb_host
            include/usb_device
            include/marlin
            src/hw
            src/common
            src/persistent_stores
            src/persistent_stores/store_instances
            src/guiconfig
            src/lang
            src/common/utils
            src
            ${OPTIONS_INCLUDE_DIR}
  )

if(MCU MATCHES "STM32G0")
  target_include_directories(
    BuddyHeaders BEFORE INTERFACE include/stm32g0_hal include/device/stm32g0
    )
  target_link_libraries(BuddyHeaders INTERFACE STM32G0::HAL STM32G0xx::CMSIS)
elseif(MCU MATCHES "STM32F4")
  target_include_directories(
    BuddyHeaders BEFORE INTERFACE include/stm32f4_hal include/device/stm32f4
    )
  target_link_libraries(BuddyHeaders INTERFACE STM32F4::HAL STM32F4xx::CMSIS)
elseif(MCU MATCHES "STM32H5")
  target_include_directories(
    BuddyHeaders BEFORE INTERFACE include/stm32h5_hal include/device/stm32h5
    )
  target_link_libraries(BuddyHeaders INTERFACE STM32H5::HAL STM32H5xx::CMSIS)
elseif(MCU MATCHES "STM32C0")
  target_include_directories(
    BuddyHeaders BEFORE INTERFACE include/stm32c0_hal include/device/stm32c0
    )
  target_link_libraries(BuddyHeaders INTERFACE STM32C0::HAL STM32C0xx::CMSIS)
else()
  message(FATAL_ERROR "Undefined HAL for ${MCU}")
endif()

target_include_directories(BuddyHeaders INTERFACE include)

target_link_libraries(BuddyHeaders INTERFACE FreeRTOS::FreeRTOS)

# Define printer type and version
set(PRINTER_VERSION "1") # default version
set(PRINTER_SUBVERSION "0") # default subversion

# The PRINTER_TYPE is (besides being used in the code) being stored in the .bbf and checked by the
# bootloader (which contains the same "enum" as below). Preferably do not use it on an interface
# though, there should be a better number to use there, e.g. the PRINTER_CODE (USB PID)
if(PRINTER STREQUAL "MK4")
  set(PRINTER_TYPE "1")
  set(PRINTER_VERSION "4")
  set(PRINTER_CODE "13")
elseif(PRINTER STREQUAL "MK3.5")
  set(PRINTER_TYPE "1")
  set(PRINTER_VERSION "3")
  set(PRINTER_SUBVERSION "5")
  set(PRINTER_CODE "23")
elseif(PRINTER STREQUAL "MINI")
  set(PRINTER_TYPE "2")
  set(PRINTER_CODE "12")
elseif(PRINTER STREQUAL "XL")
  set(PRINTER_TYPE "3")
  set(PRINTER_CODE "17")
elseif(PRINTER STREQUAL "iX")
  set(PRINTER_TYPE "4")
  set(PRINTER_CODE "16")
elseif(PRINTER STREQUAL "XL_DEV_KIT")
  set(PRINTER_TYPE "5")
  set(PRINTER_CODE "18")
elseif(PRINTER STREQUAL "COREONE")
  set(PRINTER_TYPE "7")
  set(PRINTER_CODE "31")
elseif(PRINTER STREQUAL "COREONE_INDX")
  set(PRINTER_TYPE "7")
  set(PRINTER_CODE "36")
  set(PRINTER_VERSION "10")
elseif(PRINTER STREQUAL "COREONEL")
  set(PRINTER_TYPE "8")
  set(PRINTER_CODE "35")
elseif(PRINTER STREQUAL "COREONEL_INDX")
  set(PRINTER_TYPE "8")
  set(PRINTER_CODE "37")
  set(PRINTER_VERSION "10")
elseif(PRINTER STREQUAL "NONE")
  set(PRINTER_TYPE "0")
  set(PRINTER_CODE "0")
else()
  message(FATAL_ERROR "Unknown printer \"${PRINTER}\".")
endif()

add_library(printers INTERFACE)
target_include_directories(printers INTERFACE ${CMAKE_SOURCE_DIR}/include)
target_compile_definitions(
  printers INTERFACE PRINTER_TYPE=${PRINTER_TYPE} PRINTER_VERSION=${PRINTER_VERSION}
                     PRINTER_SUBVERSION=${PRINTER_SUBVERSION}
  )

target_compile_definitions(
  BuddyHeaders INTERFACE PRINTER_CODE=${PRINTER_CODE} BOARD=BOARD_${BOARD} PROCESS_CUSTOM_GCODE
  )
target_link_libraries(BuddyHeaders INTERFACE printers)

if(MCU MATCHES "STM32F4")
  target_compile_definitions(BuddyHeaders INTERFACE STM32GENERIC STM32F4 STM32F4xx)
elseif(MCU MATCHES "STM32G0")
  target_compile_definitions(BuddyHeaders INTERFACE STM32GENERIC STM32G0 STM32G0xx)
elseif(MCU MATCHES "STM32H5")
  target_compile_definitions(BuddyHeaders INTERFACE STM32GENERIC STM32H5 STM32H5xx)
elseif(MCU MATCHES "STM32C0")
  target_compile_definitions(BuddyHeaders INTERFACE STM32GENERIC STM32C0 STM32C0xx)
else()
  message(FATAL_ERROR "Undefined macros for ${MCU}")
endif()

#
# Configure STMicroelectronics Libraries
#

# STM32::USBHost
add_library(STM32_USBHost_Config INTERFACE)
target_link_libraries(STM32_USBHost_Config INTERFACE BuddyHeaders FreeRTOS::CMSIS)

# STM32::Utilities::CPU
add_library(STM32_Utilities_CPU_Config INTERFACE)
target_link_libraries(STM32_Utilities_CPU_Config INTERFACE BuddyHeaders FreeRTOS::CMSIS)

#
# Configure SEGGER
#
add_library(Segger_Config INTERFACE)
target_include_directories(Segger_Config INTERFACE include/segger src/segger)
target_link_libraries(Segger_Config INTERFACE BuddyHeaders)

#
# Configure LwIP
#

add_library(LwIP_Config INTERFACE)
target_link_libraries(LwIP_Config INTERFACE BuddyHeaders log_interface FreeRTOS::CMSIS)

#
# Configure FatFs
#

add_library(FatFs_Config INTERFACE)
target_link_libraries(FatFs_Config INTERFACE BuddyHeaders logging STM32::USBHost FreeRTOS::CMSIS)

#
# Configure Marlin
#

add_library(Marlin_Config INTERFACE)
# TODO: fix dependency on src/common and src/gui
target_include_directories(
  Marlin_Config INTERFACE include/marlin src/common src/gui src/marlin_stubs/include
  )
target_include_directories(Marlin_Config INTERFACE src/common/selftest src/common/gcode)
target_link_libraries(
  Marlin_Config INTERFACE BuddyHeaders FreeRTOS::FreeRTOS FreeRTOS::CMSIS mbedTLS
  )

#
# Configure tinyusb
#
add_library(tinyusb_dependencies INTERFACE)
target_include_directories(tinyusb_dependencies INTERFACE include/tinyusb)
target_link_libraries(tinyusb_dependencies INTERFACE FreeRTOS::FreeRTOS FreeRTOS::CMSIS)

#
# Global Compiler & Linker Configuration
#

# include symbols
add_compile_options(-g)

# optimizations
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_options(-Og -ggdb3)
else()
  add_compile_options(-Os)
endif()

if(CMAKE_CROSSCOMPILING)
  # mcu related settings
  set(MCU_FLAGS -mthumb)
  if(MCU MATCHES "STM32F4")
    list(APPEND MCU_FLAGS -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16)
  elseif(MCU MATCHES "STM32G0" OR MCU MATCHES "STM32C0")
    list(APPEND MCU_FLAGS -mcpu=cortex-m0plus -mfloat-abi=soft)
  elseif(MCU MATCHES "STM32H5")
    list(APPEND MCU_FLAGS -mcpu=cortex-m33 -mfloat-abi=hard -mfpu=fpv5-sp-d16)
  else()
    message(FATAL_ERROR "Unknown MCU_FLAGS for ${MCU}")
  endif()
  add_compile_options(${MCU_FLAGS})
  add_link_options(${MCU_FLAGS})

  # better FreeRTOS support
  add_link_options(-Wl,--undefined=init_task)

  # disable exceptions and related metadata
  add_compile_options(-fno-exceptions -fno-unwind-tables)
  add_link_options(-Wl,--defsym,__exidx_start=0,--defsym,__exidx_end=0)

  # wrap _dtoa to ensure formatting of floats isn't being called from ISR
  add_link_options(-Wl,--wrap=_dtoa_r)

  # disable warnings about RWX sections due to CrashCatcher's "safe" page
  add_link_options(-Wl,--no-warn-rwx-segments)
endif()

# split and gc sections
add_compile_options(-ffunction-sections -fdata-sections)
add_link_options(-Wl,--gc-sections)

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

#
# Import definitions of all libraries
#

add_subdirectory(lib)

# Polite boards are not polluting global cmake state. Keep the list sorted!
list(APPEND POLITE_BOARDS "ANFC")
list(APPEND POLITE_BOARDS "INDX_HEAD")
list(APPEND POLITE_BOARDS "TOOL_OFFSET_SENSOR")
list(APPEND POLITE_BOARDS "XBUDDY_EXTENSION")

#
# Build Puppy Firmwares
#

if(BOARD IN_LIST BUDDY_BOARDS AND HAS_PUPPIES_BOOTLOADER)
  if(NOT DWARF_BINARY_PATH AND HAS_DWARF)
    add_puppy_firmware(PUPPY_NAME dwarf OUT_BINARY_PATH DWARF_BINARY_PATH)
  endif()

  if(NOT INDX_HEAD_BINARY_PATH AND HAS_INDX)
    add_puppy_firmware(PUPPY_NAME indx_head OUT_BINARY_PATH INDX_HEAD_BINARY_PATH)
  endif()

  if(NOT MODULARBED_BINARY_PATH AND HAS_PUPPY_MODULARBED)
    add_puppy_firmware(PUPPY_NAME modularbed OUT_BINARY_PATH MODULARBED_BINARY_PATH)
  endif()

  if(NOT XBUDDY_EXTENSION_BINARY_PATH AND HAS_XBUDDY_EXTENSION)
    add_puppy_firmware(PUPPY_NAME xbuddy_extension OUT_BINARY_PATH XBUDDY_EXTENSION_BINARY_PATH)
  endif()

  if(NOT XL_CAN_BINARY_PATH AND HAS_XL_CAN)
    add_puppy_firmware(PUPPY_NAME can OUT_BINARY_PATH XL_CAN_BINARY_PATH)
  endif()

  if(NOT ANFC_BINARY_PATH AND HAS_ANFC)
    add_puppy_firmware(PUPPY_NAME anfc OUT_BINARY_PATH ANFC_BINARY_PATH)
  endif()

  if(NOT TOOL_OFFSET_SENSOR_BINARY_PATH AND HAS_TOOL_OFFSET_SENSOR)
    add_puppy_firmware(PUPPY_NAME tool_offset_sensor OUT_BINARY_PATH TOOL_OFFSET_SENSOR_BINARY_PATH)
  endif()
endif()

# Unittests
if(UNITTESTS_ENABLE)
  enable_testing()
  add_subdirectory(tests)
endif()

# std::rand/random
if(BOARD IN_LIST BUDDY_BOARDS)
  set(ENABLE_HW_STD_RAND TRUE)
else()
  set(ENABLE_HW_STD_RAND FALSE)
endif()

#
# Buddy firmware
#

add_executable(firmware)

if(RESOURCES)
  set(firmware_without_digest_bin "${CMAKE_BINARY_DIR}/firmware_without_digest.bin")
  add_custom_command(
    TARGET firmware
    POST_BUILD
    COMMAND "${CMAKE_OBJCOPY}" -O binary -S "$<TARGET_FILE:firmware>"
            "${firmware_without_digest_bin}"
    BYPRODUCTS "${firmware_without_digest_bin}"
    VERBATIM
    )
endif()

add_subdirectory(src)

target_compile_options(
  firmware
  PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wextra>
          $<$<COMPILE_LANGUAGE:CXX>:-Wno-expansion-to-defined>
          $<$<COMPILE_LANGUAGE:CXX>:-Wno-format-zero-length>
          $<$<COMPILE_LANGUAGE:CXX>:-Wno-non-virtual-dtor>
          $<$<COMPILE_LANGUAGE:CXX>:-Werror=delete-non-virtual-dtor>
          $<$<COMPILE_LANGUAGE:CXX>:-Werror=switch>
          $<$<COMPILE_LANGUAGE:CXX>:-Wduplicated-cond>
          $<$<COMPILE_LANGUAGE:CXX>:-Wlogical-op>
          $<$<COMPILE_LANGUAGE:CXX>:-Wnull-dereference>
          $<$<COMPILE_LANGUAGE:CXX>:-Wshadow=compatible-local>
          $<$<COMPILE_LANGUAGE:CXX>:-Wvla>
          $<$<COMPILE_LANGUAGE:CXX>:-Wno-psabi>
          -Wdouble-promotion
  )
target_link_options(firmware PRIVATE -Wno-psabi)

if(ENABLE_HW_STD_RAND)
  # Wrap rand and replace the implementation with one using HW RNG
  target_link_options(firmware PUBLIC -Wl,--wrap=rand)

  # Wrap srand, do not implement the wrapper -> throws error when referenced in the code. rand()
  # uses HW RNG now, so it cannot be seeded
  target_link_options(firmware PUBLIC -Wl,--wrap=srand)
endif()

# Appending fw descriptor (with calculated fingerprint) to the ELF
if(BOARD STREQUAL "DWARF"
   OR BOARD STREQUAL "MODULARBED"
   OR BOARD IN_LIST POLITE_BOARDS
   )
  add_custom_command(
    TARGET firmware
    POST_BUILD
    # Generate a binary without fingerprint
    COMMAND "${CMAKE_OBJCOPY}" -O binary -S "$<TARGET_FILE:firmware>" "firmware_no_descriptor.bin"
    # Create the fw descriptor with calculated fingerprint of binary
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/utils/gen_puppies_descriptor.py"
            "firmware_no_descriptor.bin" "fw_descriptor.bin"
    # Set the firmware's fw descriptor in the firmware's ELF file
    COMMAND "${CMAKE_OBJCOPY}" "--update-section" ".fw_descriptor=fw_descriptor.bin"
            "$<TARGET_FILE:firmware>" "$<TARGET_FILE:firmware>"
    VERBATIM
    )
endif()

if(NOT RESOURCES)
  objcopy(firmware "binary" ".bin")
endif()

# generate linker map file
target_link_options(firmware PUBLIC -Wl,-Map=firmware.map,--print-memory-usage)

if(RESOURCES)
  message(STATUS "Configured to generate .bbf version of the firmware.")
  message(STATUS "Signing Key: ${SIGNING_KEY}")

  if(PROJECT_VERSION_SUFFIX)
    if(NOT "${PROJECT_VERSION_SUFFIX}" MATCHES "\\+${BUILD_NUMBER}")
      message(WARNING "The generated .bbf is configured to use the build number ${BUILD_NUMBER},
      but the version suffix (${PROJECT_VERSION_SUFFIX}) does not contain +${BUILD_NUMBER}.
      Are you sure you know what you are doing?"
              )
    endif()
  endif()

  pack_firmware(
    firmware
    FW_VERSION
    "${PROJECT_VERSION}${PROJECT_VERSION_SUFFIX_SHORT}"
    BUILD_NUMBER
    "${BUILD_NUMBER}"
    PRINTER_TYPE
    "${PRINTER_TYPE}"
    PRINTER_VERSION
    "${PRINTER_VERSION}"
    PRINTER_SUBVERSION
    "${PRINTER_SUBVERSION}"
    SIGNING_KEY
    "${SIGNING_KEY}"
    )

elseif(SIGNING_KEY)
  message(WARNING "SIGNING_KEY specified but BBF generation is not enabled.")
endif()

# Library that does some magic to save resources. Link everywhere.
target_link_libraries(firmware PRIVATE magic_optimizations)

if(BOARD IN_LIST BUDDY_BOARDS)
  target_link_libraries(
    firmware
    PRIVATE BuddyHeaders
            bsod
            CrashCatcher
            Marlin
            Arduino::Core
            Arduino::TMCStepper
            LwIP
            FatFs
            littlefs
            STM32::USBHost
            inih::inih
            $<$<BOOL:${WUI}>:WUI>
            jsmn::jsmn
            QR
            Buddy::Lang
            CppStdExtensions
            printf::printf
            sysbase
            Segger
            tinyusb::tinyusb
            $<$<BOOL:${HAS_MMU2}>:MMU2::MMU2>
            mbedTLS
            $<$<BOOL:${HAS_PUPPIES}>:lightmodbus>
            freertos
            error_codes
            error_codes_mmu # TODO once MMU is not necessary for the build, replace with:
                            # $<$<BOOL:${HAS_MMU2}>:error_codes_mmu>
            bgcode_core
            heatshrink_decoder
            version
            otp
    )
elseif(BOARD STREQUAL "DWARF")
  target_link_libraries(
    firmware
    PRIVATE Arduino::Core
            Arduino::TMCStepper
            BuddyHeaders
            bsod
            CrashCatcher
            error_codes
            freertos
            Marlin
            printf::printf
            cortex-m0-atomic
            otp
    )
elseif(BOARD STREQUAL "MODULARBED")
  target_link_libraries(
    firmware
    PRIVATE BuddyHeaders
            bsod
            CrashCatcher
            error_codes
            freertos
            printf::printf
            otp
            cortex-m0-atomic
    )
elseif(BOARD IN_LIST POLITE_BOARDS)
  # let us manage the libraries where needed, not globally
elseif(BOARD STREQUAL "XL_DEV_KIT_XLB")
  target_link_libraries(
    firmware
    PRIVATE BuddyHeaders
            bsod
            Marlin
            Arduino::Core
            Arduino::TMCStepper
            inih::inih
            CppStdExtensions
            printf::printf
            sysbase
            Segger
            $<$<BOOL:${HAS_MMU2}>:MMU2::MMU2>
            mbedTLS
            $<$<BOOL:${HAS_PUPPIES}>:lightmodbus>
            freertos
            error_codes
            error_codes_mmu # TODO once MMU is not necessary for the build, replace with:
                            # $<$<BOOL:${HAS_MMU2}>:error_codes_mmu>
            bgcode_core # todo: remove
            heatshrink_decoder # todo: remove
            version
    )
else()
  message(FATAL_ERROR "Add ${BOARD} to POLITE_BOARDS (probably)")
endif()

option(DSDL_TRANSPILE "Enable DSDL transcompilation" ON)
if(DSDL_TRANSPILE)
  # Convert dsdl files to headers and include them
  include(src/can/data_types/TranspileDsdl.cmake)
  transpile_dsdl()
endif()

if(NOT DEFINED UNITTESTS_ENABLE OR NOT UNITTESTS_ENABLE)
  set_target_properties(firmware PROPERTIES EXCLUDE_FROM_ALL OFF)
endif()
