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
    CACHE BOOL "Whether a .bbf version should be generated."
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
    "YES"
    CACHE BOOL "Enable Connect client"
    )
set(DEVELOPMENT_ITEMS_ENABLED
    "YES"
    CACHE BOOL "Show development (green) items  in menus"
    )
define_boolean_option(DEVELOPMENT_ITEMS ${DEVELOPMENT_ITEMS_ENABLED})
# Validate options
foreach(OPTION "PRINTER" "BOARD" "MCU" "BOOTLOADER")
  if(NOT ${OPTION} IN_LIST ${OPTION}_VALID_OPTS)
    message(FATAL_ERROR "Invalid ${OPTION} ${${OPTION}}: Valid values are ${${OPTION}_VALID_OPTS}")
  endif()
endforeach()

# define simple options
define_boolean_option(BOOTLOADER ${BOOTLOADER})

# set board to its default if not specified
if(${BOARD} STREQUAL "<default>")
  if(${PRINTER} MATCHES "^(MINI)$")
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
  if(${BOARD} STREQUAL "BUDDY")
    set(MCU "STM32F407VG")
  else()
    message(FATAL_ERROR "Don't know what MCU to set as default for this board/version")
  endif()
endif()
# define MCU option
list(REMOVE_ITEM MCU_VALID_OPTS "<default>")
define_enum_option(NAME MCU VALUE ${MCU} ALL_VALUES ${MCU_VALID_OPTS})

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
message(STATUS "Connect client: ${CONNECT}")
message(STATUS "Resources: ${RESOURCES}")

# Set printer features
set(PRINTERS_WITH_FILAMENT_SENSOR_BINARY "MINI")
set(PRINTERS_WITH_INIT_TRINAMIC_FROM_MARLIN_ONLY "MINI")
set(PRINTERS_WITH_ADVANCED_PAUSE "MINI")
set(PRINTERS_WITH_POWER_PANIC)
set(PRINTERS_WITH_SELFTEST "MINI")
set(PRINTERS_WITH_RESOURCES "MINI")
set(PRINTERS_WITH_BOWDEN_EXTRUDER "MINI")

# Set GUI settings
set(PRINTERS_WITH_GUI "MINI")
set(PRINTERS_WITH_GUI_W240H320 "MINI")
set(PRINTERS_WITH_SERIAL_PRINTING "MINI")

# Set printer board
set(BOARDS_WITH_ST7789V "BUDDY")

if(${PRINTER} IN_LIST PRINTERS_WITH_FILAMENT_SENSOR_BINARY AND BOARD MATCHES ".*BUDDY")
  set(FILAMENT_SENSOR BINARY)
else()
  set(FILAMENT_SENSOR NO)
endif()
define_enum_option(NAME FILAMENT_SENSOR VALUE "${FILAMENT_SENSOR}" ALL_VALUES "BINARY;NO")

if(${PRINTER} IN_LIST PRINTERS_WITH_RESOURCES AND BOARD MATCHES ".*BUDDY")
  set(RESOURCES YES)
else()
  set(RESOURCES NO)
endif()
define_boolean_option(RESOURCES ${RESOURCES})

if(${PRINTER} IN_LIST PRINTERS_WITH_GUI AND BOARD MATCHES ".*BUDDY")
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
define_boolean_option(HAS_GUI ${GUI})

if(${PRINTER} IN_LIST PRINTERS_WITH_INIT_TRINAMIC_FROM_MARLIN_ONLY)
  set(INIT_TRINAMIC_FROM_MARLIN_ONLY YES)
else()
  set(INIT_TRINAMIC_FROM_MARLIN_ONLY NO)
endif()

if(${PRINTER} IN_LIST PRINTERS_WITH_SELFTEST)
  set(HAS_SELFTEST YES)
else()
  set(HAS_SELFTEST NO)
endif()

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

if(${PRINTER} IN_LIST PRINTERS_WITH_SERIAL_PRINTING)
  set(HAS_SERIAL_PRINT YES)
else()
  set(HAS_SERIAL_PRINT NO)
endif()
message(STATUS "Support for printing via serial interface: ${HAS_SERIAL_PRINT}")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(DEBUG YES)
else()
  set(DEBUG NO)
endif()

# define enabled features
define_feature(BOOTLOADER ${BOOTLOADER})
define_feature(RESOURCES ${RESOURCES})
define_feature(HAS_SELFTEST ${HAS_SELFTEST})

if(BOOTLOADER AND ${PRINTER} STREQUAL "MINI")
  set(BOOTLOADER_UPDATE YES)
else()
  set(BOOTLOADER_UPDATE NO)
endif()
define_boolean_option(BOOTLOADER_UPDATE ${BOOTLOADER_UPDATE})
