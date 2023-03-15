#
# Command Line Options
#
# You should specify those options when invoking CMake. Example:
# ~~~
# cmake .. <other options> -DPRINTER=MINI
# ~~~

set(PRINTER_VALID_OPTS "MINI" "MK404" "XL" "IXL")
set(BOARD_VALID_OPTS "<default>" "BUDDY" "XBUDDY" "XLBUDDY" "DWARF" "MODULARBED")
set(MCU_VALID_OPTS "<default>" "STM32F407VG" "STM32F429VI" "STM32F427ZI" "STM32G070RBT6")
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
    "<auto>"
    CACHE BOOL "Enable resources (managed files on external flash)"
    )
set(DEVELOPMENT_ITEMS_ENABLED
    "YES"
    CACHE BOOL "Show development (green) items  in menus"
    )
define_boolean_option(DEVELOPMENT_ITEMS ${DEVELOPMENT_ITEMS_ENABLED})

set(IS_KNOBLET
    "FALSE"
    CACHE BOOL "Knoblet version of FW"
    )
define_boolean_option(IS_KNOBLET ${IS_KNOBLET})

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
    set(BOARD
        "BUDDY"
        CACHE STRING "System board" FORCE
        )
  elseif(${PRINTER} MATCHES "^(IXL|MK404)$")
    set(BOARD
        "XBUDDY"
        CACHE STRING "System board" FORCE
        )
  elseif(${PRINTER} STREQUAL "XL")
    set(BOARD
        "XLBUDDY"
        CACHE STRING "System board" FORCE
        )
  else()
    message(FATAL_ERROR "No default board set for printer ${PRINTER}")
  endif()
endif()

# set board version to its default if not specified
if(${BOARD_VERSION} STREQUAL "<default>")
  if(${BOARD} STREQUAL "BUDDY")
    set(BOARD_VERSION
        "1.0.0"
        CACHE STRING "Buddy board version" FORCE
        )
  elseif(${BOARD} STREQUAL "XBUDDY")
    if(${PRINTER} STREQUAL "MK404")
      set(BOARD_VERSION
          "0.2.1"
          CACHE STRING "XBuddy board version" FORCE
          )
    else()
      set(BOARD_VERSION
          "0.1.8"
          CACHE STRING "XBuddy board version" FORCE
          )
    endif()
  elseif(${BOARD} STREQUAL "XLBUDDY")
    set(BOARD_VERSION
        "0.5.0"
        CACHE STRING "XLBuddy board version" FORCE
        )
  elseif(${BOARD} STREQUAL "DWARF")
    set(BOARD_VERSION
        "0.6.0"
        CACHE STRING "Dwarf board version" FORCE
        )
  elseif(${BOARD} STREQUAL "MODULARBED")
    set(BOARD_VERSION
        "0.7.0"
        CACHE STRING "ModularBed board version" FORCE
        )
  else()
    message(FATAL_ERROR "No default board version set for board ${BOARD}")
  endif()
endif()

# set MCU to its default if not specified
if(${MCU} STREQUAL "<default>")
  if(${BOARD} STREQUAL "BUDDY")
    if(${PRINTER} MATCHES "^(XL)$")
      set(MCU "STM32F429VI")
    else()
      set(MCU "STM32F407VG")
    endif()
  elseif(${BOARD} STREQUAL "XBUDDY" AND ${BOARD_VERSION} VERSION_LESS 0.2.0)
    set(MCU "STM32F407VG")
  elseif(${BOARD} STREQUAL "XBUDDY" AND ${BOARD_VERSION} VERSION_GREATER_EQUAL 0.2.0)
    set(MCU "STM32F427ZI")
  elseif(${BOARD} STREQUAL "XLBUDDY")
    set(MCU "STM32F427ZI")
  elseif(${BOARD} STREQUAL "DWARF")
    set(MCU "STM32G070RBT6")
  elseif(${BOARD} STREQUAL "MODULARBED")
    set(MCU "STM32G070RBT6")
  else()
    message(FATAL_ERROR "Don't know what MCU to set as default for this board/version")
  endif()
endif()
# define MCU option
list(REMOVE_ITEM MCU_VALID_OPTS "<default>")
define_enum_option(NAME MCU VALUE ${MCU} ALL_VALUES ${MCU_VALID_OPTS})

if(BOARD STREQUAL "XLBUDDY")
  set(ENABLE_PUPPY_BOOTLOAD
      "YES"
      CACHE BOOL "Pack puppy firmwares into resources and bootload them on startup of the XL"
      )
endif()

if(ENABLE_PUPPY_BOOTLOAD)
  set(DWARF_BINARY_PATH
      ""
      CACHE PATH
            "Where to get the Dwarf's binary from. If set, the project won't try to build anything."
      )
  if(NOT DWARF_BINARY_PATH)
    set(DWARF_SOURCE_DIR
        "${CMAKE_SOURCE_DIR}"
        CACHE PATH "From which source directory to build the dwarf firmware."
        )
    set(DWARF_BINARY_DIR
        "${CMAKE_BINARY_DIR}/dwarf-build"
        CACHE PATH "Where to have build directory for the dwarf firmware."
        )
  endif()

  set(MODULARBED_BINARY_PATH
      ""
      CACHE
        PATH
        "Where to get the Modularbed's binary from. If set, the project won't try to build anything."
      )
  if(NOT MODULARBED_BINARY_PATH)
    set(MODULARBED_SOURCE_DIR
        "${CMAKE_SOURCE_DIR}"
        CACHE PATH "From which source directory to build the modular bed firmware."
        )
    set(MODULARBED_BINARY_DIR
        "${CMAKE_BINARY_DIR}/modularbed-build"
        CACHE PATH "Where to have build directory for the modular bed firmware."
        )
  endif()
endif()

# Set connect status/availability
if(${BOARD} STREQUAL "DWARF" OR ${BOARD} STREQUAL "MODULARBED")
  set(CONNECT
      "NO"
      CACHE BOOL "Enable Connect client"
      )
else()
  set(CONNECT
      "YES"
      CACHE BOOL "Enable Connect client"
      )
endif()

# parse board version into its components
string(REGEX MATCH "^([0-9]+)\\.([0-9]+)\\.([0-9]+)" BOARD_VERSION_MATCH ${BOARD_VERSION})
set(BOARD_VERSION_MAJOR ${CMAKE_MATCH_1})
set(BOARD_VERSION_MINOR ${CMAKE_MATCH_2})
set(BOARD_VERSION_PATCH ${CMAKE_MATCH_3})

# Resolve BUILD_NUMBER and PROJECT_VERSION_* variables
resolve_version_variables()

if(${PRINTER} STREQUAL "MK404" AND ${BOARD_VERSION} VERSION_LESS 0.2.0)
  message(STATUS "Disabling WUI and Connect for MK404 with 1MB chip")
  set(WUI NO)
  set(CONNECT NO)
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
message(STATUS "Connect client: ${CONNECT}")
message(STATUS "Resources: ${RESOURCES}")

# Set printer features
set(PRINTERS_WITH_FILAMENT_SENSOR_BINARY "MINI")
set(PRINTERS_WITH_FILAMENT_SENSOR_ADC "MK404" "XL" "IXL")
set(PRINTERS_WITH_INIT_TRINAMIC_FROM_MARLIN_ONLY "MINI" "MK404" "XL" "IXL")
set(PRINTERS_WITH_ADVANCED_PAUSE "MINI" "MK404" "IXL" "XL")
set(PRINTERS_WITH_CRASH_DETECTION "MINI" "MK404" "XL")
set(PRINTERS_WITH_POWER_PANIC "MK404" "XL")
# private MINI would not fit to 1MB so it has disabled selftest set(PRINTERS_WITH_SELFTEST "MINI"
# "MK404")
set(PRINTERS_WITH_SELFTEST "MK404" "XL" "MINI")
set(PRINTERS_WITH_LOADCELL "MK404" "IXL" "XL")
set(PRINTERS_WITH_RESOURCES "MINI" "MK404" "XL" "IXL")
set(PRINTERS_WITH_BOWDEN_EXTRUDER "MINI")
set(PRINTERS_WITH_PUPPIES "XL")
set(PRINTERS_WITH_PUPPIES_BOOTLOADER "XL")
set(PRINTERS_WITH_TOOLCHANGER "XL")
set(PRINTERS_WITH_SIDE_FSENSOR "XL")
set(PRINTERS_WITH_EMBEDDED_ESP32 "XL")
set(PRINTERS_WITH_SIDE_LEDS "XL")

# Set GUI settings
set(PRINTERS_WITH_GUI "MINI" "MK404" "XL" "IXL")
set(PRINTERS_WITH_GUI_W480H320 "MK404" "XL" "IXL")
set(PRINTERS_WITH_GUI_W240H320 "MINI")
set(PRINTERS_WITH_LEDS "MK404" "XL" "IXL")
# disable serial printing for MINI to save flash
set(PRINTERS_WITH_SERIAL_PRINTING "MK404" "XL" "IXL" "MINI")

# Set printer board
set(BOARDS_WITH_ADVANCED_POWER "XBUDDY" "XLBUDDY" "DWARF")
set(BOARDS_WITH_ILI9488 "XBUDDY" "XLBUDDY")
set(BOARDS_WITH_ST7789V "BUDDY")

if(${PRINTER} IN_LIST PRINTERS_WITH_FILAMENT_SENSOR_BINARY AND BOARD MATCHES ".*BUDDY")
  set(FILAMENT_SENSOR BINARY)
elseif(${PRINTER} IN_LIST PRINTERS_WITH_FILAMENT_SENSOR_ADC AND BOARD MATCHES ".*BUDDY")
  set(FILAMENT_SENSOR ADC)
else()
  set(FILAMENT_SENSOR NO)
endif()
define_enum_option(NAME FILAMENT_SENSOR VALUE "${FILAMENT_SENSOR}" ALL_VALUES "BINARY;ADC;NO")

if(${RESOURCES} STREQUAL "<auto>")
  if(${PRINTER} IN_LIST PRINTERS_WITH_RESOURCES AND BOARD MATCHES ".*BUDDY")
    set(RESOURCES YES)
  else()
    set(RESOURCES NO)
  endif()
endif()
define_boolean_option(RESOURCES ${RESOURCES})

# in order to generate DFU file for bootloader, we need a BFU
if(GENERATE_DFU
   AND BOOTLOADER
   OR RESOURCES
   )

  set(GENERATE_BBF "YES")
elseif(NOT BOARD MATCHES ".*BUDDY")
  set(GENERATE_BBF "NO")
endif()

if(${PRINTER} IN_LIST PRINTERS_WITH_GUI AND BOARD MATCHES ".*BUDDY")
  set(GUI YES)

  if(${PRINTER} IN_LIST PRINTERS_WITH_GUI_W480H320 AND ${PRINTER} IN_LIST
                                                       PRINTERS_WITH_GUI_W240H320
     )
    message(FATAL_ERROR "Printer can only have one GUI resolution")
  endif()

  if(${PRINTER} IN_LIST PRINTERS_WITH_GUI_W480H320)
    set(RESOLUTION W480H320)
  elseif(${PRINTER} IN_LIST PRINTERS_WITH_GUI_W240H320)
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

if(${PRINTER} IN_LIST PRINTERS_WITH_LOADCELL AND BOARD MATCHES ".*BUDDY")
  set(HAS_LOADCELL YES)
else()
  set(HAS_LOADCELL NO)
endif()
define_boolean_option(HAS_LOADCELL ${HAS_LOADCELL})

if(${BOARD} IN_LIST BOARDS_WITH_ADVANCED_POWER AND ${BOARD_VERSION} VERSION_GREATER_EQUAL 0.2.0)
  set(HAS_ADVANCED_POWER YES)
else()
  set(HAS_ADVANCED_POWER NO)
endif()

set(HAS_MMU2 NO)
message(STATUS "MMU2: ${HAS_MMU2}")

if((${BOARD} STREQUAL "XBUDDY" AND ${BOARD_VERSION} VERSION_GREATER_EQUAL "0.2.0")
   OR ${BOARD} STREQUAL "XLBUDDY"
   )
  set(HAS_XLCD_TOUCH_DRIVER YES)
else()
  set(HAS_XLCD_TOUCH_DRIVER NO)
endif()
message(STATUS "XLCD_TOUCH_DRIVER: ${HAS_XLCD_TOUCH_DRIVER}")

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

if(${PRINTER} IN_LIST PRINTERS_WITH_PUPPIES AND BOARD MATCHES ".*BUDDY")
  set(HAS_PUPPIES YES)
else()
  set(HAS_PUPPIES NO)
endif()
define_boolean_option(HAS_PUPPIES ${HAS_PUPPIES})

if(${PRINTER} IN_LIST PRINTERS_WITH_LEDS)
  set(HAS_LEDS YES)
else()
  set(HAS_LEDS NO)
endif()

if(${PRINTER} IN_LIST PRINTERS_WITH_PUPPIES_BOOTLOADER
   AND BOARD MATCHES ".*BUDDY"
   AND RESOURCES
   AND ENABLE_PUPPY_BOOTLOAD
   )
  set(HAS_PUPPIES_BOOTLOADER YES)
else()
  set(HAS_PUPPIES_BOOTLOADER NO)
endif()
define_boolean_option(HAS_PUPPIES_BOOTLOADER ${HAS_PUPPIES_BOOTLOADER})
define_boolean_option(PUPPY_FLASH_FW ${HAS_PUPPIES_BOOTLOADER})

if(${PRINTER} IN_LIST PRINTERS_WITH_TOOLCHANGER)
  set(HAS_TOOLCHANGER YES)
else()
  set(HAS_TOOLCHANGER NO)
endif()
define_boolean_option(HAS_TOOLCHANGER ${HAS_TOOLCHANGER})

if(${PRINTER} IN_LIST PRINTERS_WITH_SIDE_FSENSOR)
  define_boolean_option(HAS_SIDE_FSENSOR YES)
else()
  define_boolean_option(HAS_SIDE_FSENSOR NO)
endif()

if(${PRINTER} IN_LIST PRINTERS_WITH_EMBEDDED_ESP32)
  define_boolean_option(HAS_EMBEDDED_ESP32 YES)
else()
  define_boolean_option(HAS_EMBEDDED_ESP32 NO)
endif()

if(${PRINTER} IN_LIST PRINTERS_WITH_SIDE_LEDS)
  define_boolean_option(HAS_SIDE_LEDS YES)
else()
  define_boolean_option(HAS_SIDE_LEDS NO)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(DEBUG YES)
else()
  set(DEBUG NO)
endif()

# define enabled features

if(BOOTLOADER STREQUAL "YES"
   AND (PRINTER STREQUAL "MINI"
        OR PRINTER STREQUAL "MK404"
        OR BOARD STREQUAL "XLBUDDY")
   )
  set(BOOTLOADER_UPDATE YES)
else()
  set(BOOTLOADER_UPDATE NO)
endif()
define_boolean_option(BOOTLOADER_UPDATE ${BOOTLOADER_UPDATE})

set(DEVELOPER_MODE
    "OFF"
    CACHE BOOL "Disable wizards, prompts and user-friendliness. Developers like it rough!"
    )
define_boolean_option(DEVELOPER_MODE ${DEVELOPER_MODE})
