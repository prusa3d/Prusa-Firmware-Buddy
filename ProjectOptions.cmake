#
# Command Line Options
#
# You should specify those options when invoking CMake. Example:
# ~~~
# cmake .. <other options> -DPRINTER=MINI
# ~~~

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PRINTER_VALID_OPTS "MINI" "MK4" "MK3.5" "XL" "iX" "XL_DEV_KIT")
set(BOARD_VALID_OPTS "BUDDY" "XBUDDY" "XLBUDDY" "DWARF" "MODULARBED" "XL_DEV_KIT_XLB")
set(MCU_VALID_OPTS "<default>" "STM32F407VG" "STM32F429VI" "STM32F427ZI" "STM32G070RBT6")
set(BOOTLOADER_VALID_OPTS "NO" "EMPTY" "YES")
set(TRANSLATIONS_ENABLED_VALID_OPTS "<default>" "NO" "YES")
set(TOUCH_ENABLED_VALID_OPTS "<default>" "NO" "YES")
set(BUDDY_BOARDS "BUDDY" "XBUDDY" "XLBUDDY")

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
    "<invalid>"
    CACHE
      STRING
      "Select the board for which you want to compile the project (valid values are ${BOARD_VALID_OPTS})."
    )
set(BOARD_VERSION
    "<invalid>"
    CACHE STRING "Specify the version of the board to comiple the project for (e.g. 1.2.3)"
    )
set(MCU
    "<default>"
    CACHE
      STRING
      "Select the MCU for which you want to compile the project (valid values are ${MCU_VALID_OPTS})."
    )
set(GENERATE_DFU
    "NO"
    CACHE BOOL "Whether a .dfu file should be generated."
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

if(${BOARD} STREQUAL "XL_DEV_KIT_XLB")
  set(WUI
      "NO"
      CACHE BOOL "Enable Web User Interface"
      )
else()
  set(WUI
      "YES"
      CACHE BOOL "Enable Web User Interface"
      )
endif()
define_boolean_option("BUDDY_ENABLE_WUI" ${WUI})
set(RESOURCES
    "<auto>"
    CACHE
      STRING
      "Enable resources (managed files on external flash). Set to '<auto>' to enable according to 'PRINTERS_WITH_RESOURCES'."
    )
set(TRANSLATIONS_ENABLED
    "<default>"
    CACHE STRING "Enable languages (NO == English only)"
    )
set(TRANSLATIONS_LIST
    "<default>"
    CACHE STRING "List of languages to enable"
    )

set(TOUCH_ENABLED
    "<default>"
    CACHE STRING "Enable touch (valid values are ${TOUCH_ENABLED_VALID_OPTS})."
    )
set(DEVELOPMENT_ITEMS_ENABLED
    "YES"
    CACHE BOOL "Show development (green) items in menus and enable other devel features"
    )
define_boolean_option(DEVELOPMENT_ITEMS ${DEVELOPMENT_ITEMS_ENABLED})

set(IS_KNOBLET
    "FALSE"
    CACHE BOOL "Knoblet version of FW"
    )
define_boolean_option(IS_KNOBLET ${IS_KNOBLET})

set(ENABLE_BURST
    "NO"
    CACHE BOOL "Enable BURST stepping on supported printers."
    )

# Validate options
foreach(OPTION "PRINTER" "BOARD" "MCU" "BOOTLOADER" "TRANSLATIONS_ENABLED" "TOUCH_ENABLED")
  if(NOT ${OPTION} IN_LIST ${OPTION}_VALID_OPTS)
    message(FATAL_ERROR "Invalid ${OPTION} ${${OPTION}}: Valid values are ${${OPTION}_VALID_OPTS}")
  endif()
endforeach()

# define simple options
define_boolean_option(BOOTLOADER ${BOOTLOADER})

# Set BOARD_IS_MASTER_BOARD - means main board of entire printer, non-main board are puppies
if(BOARD IN_LIST BUDDY_BOARDS OR BOARD STREQUAL "XL_DEV_KIT_XLB")
  set(BOARD_IS_MASTER_BOARD true)
else()
  set(BOARD_IS_MASTER_BOARD false)
endif()

# set MCU to its default if not specified
if(${MCU} STREQUAL "<default>")
  if(${BOARD} STREQUAL "BUDDY")
    if(${PRINTER} MATCHES "^(XL)$")
      set(MCU "STM32F429VI")
    else()
      set(MCU "STM32F407VG")
    endif()
  elseif(${BOARD} STREQUAL "XBUDDY")
    set(MCU "STM32F427ZI")
  elseif(${BOARD} STREQUAL "XLBUDDY")
    set(MCU "STM32F427ZI")
  elseif(${BOARD} STREQUAL "XL_DEV_KIT_XLB")
    set(MCU "STM32F427ZI") # todo
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

# Set connect status/availability
if(${BOARD} STREQUAL "DWARF"
   OR ${BOARD} STREQUAL "MODULARBED"
   OR ${BOARD} STREQUAL "XL_DEV_KIT_XLB"
   )
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
define_boolean_option(BUDDY_ENABLE_CONNECT ${CONNECT})

# parse board version into its components
string(REGEX MATCH "^([0-9]+)\\.([0-9]+)\\.([0-9]+)" BOARD_VERSION_MATCH ${BOARD_VERSION})
set(BOARD_VERSION_MAJOR ${CMAKE_MATCH_1})
set(BOARD_VERSION_MINOR ${CMAKE_MATCH_2})
set(BOARD_VERSION_PATCH ${CMAKE_MATCH_3})

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
message(STATUS "Web User Interface: ${WUI}")
message(STATUS "Connect client: ${CONNECT}")
message(STATUS "Resources: ${RESOURCES}")

# Set printer features
function(set_feature_for_printers FEATURE_NAME)
  set(FEATURE_PRINTER_LIST ${ARGV})
  list(REMOVE_AT FEATURE_PRINTER_LIST 0) # First argument is the feature name
  if(${PRINTER} IN_LIST FEATURE_PRINTER_LIST)
    set(FEATURE_VALUE YES)
  else()
    set(FEATURE_VALUE NO)
  endif()
  set(${FEATURE_NAME}
      ${FEATURE_VALUE}
      PARENT_SCOPE
      )
  define_boolean_option(${FEATURE_NAME} ${FEATURE_VALUE})
endfunction()

function(set_feature_for_printers_master_board FEATURE_NAME)
  set(FEATURE_PRINTER_LIST ${ARGV})
  list(REMOVE_AT FEATURE_PRINTER_LIST 0) # First argument is the feature name
  if(BOARD_IS_MASTER_BOARD AND ${PRINTER} IN_LIST FEATURE_PRINTER_LIST)
    set(FEATURE_VALUE YES)
  else()
    set(FEATURE_VALUE NO)
  endif()
  set(${FEATURE_NAME}
      ${FEATURE_VALUE}
      PARENT_SCOPE
      )
  define_boolean_option(${FEATURE_NAME} ${FEATURE_VALUE})
endfunction()

set(PRINTERS_WITH_FILAMENT_SENSOR_BINARY "MINI" "MK3.5")
set(PRINTERS_WITH_FILAMENT_SENSOR_ADC "MK4" "XL" "iX" "XL_DEV_KIT")

set_feature_for_printers(INIT_TRINAMIC_FROM_MARLIN_ONLY "MINI" "MK4" "MK3.5" "XL" "iX")
set_feature_for_printers(
  HAS_PAUSE
  "MINI"
  "MK4"
  "MK3.5"
  "iX"
  "XL"
  "XL_DEV_KIT"
  )
set_feature_for_printers(HAS_CRASH_DETECTION "MINI" "MK4" "MK3.5" "iX" "XL") # this does require
                                                                             # selftest to
# work
set_feature_for_printers(HAS_POWER_PANIC "MK4" "MK3.5" "iX" "XL") # this does require selftest and
                                                                  # crash
# detection to work
set_feature_for_printers(HAS_PRECISE_HOMING "MK4" "MK3.5")
set_feature_for_printers(HAS_PRECISE_HOMING_COREXY "iX" "XL" "XL_DEV_KIT")
set_feature_for_printers_master_board(HAS_PHASE_STEPPING "XL" "iX")
set_feature_for_printers_master_board(HAS_PHASE_STEPPING_TOGGLE "XL")
set(PRINTERS_WITH_BURST_STEPPING "XL")
set_feature_for_printers_master_board(HAS_INPUT_SHAPER_CALIBRATION "MK4" "MK3.5" "XL" "XL_DEV_KIT")
set_feature_for_printers(HAS_SELFTEST "MK4" "MK3.5" "XL" "iX" "MINI")
set_feature_for_printers(HAS_HUMAN_INTERACTIONS "MINI" "MK4" "MK3.5" "XL")
set_feature_for_printers_master_board(HAS_LOADCELL "MK4" "iX" "XL" "XL_DEV_KIT")
set_feature_for_printers_master_board(HAS_SHEET_PROFILES "MK3.5" "MINI")
set_feature_for_printers_master_board(HAS_HEATBREAK_TEMP "MK4" "iX" "XL" "XL_DEV_KIT")
set(PRINTERS_WITH_RESOURCES "MINI" "MK4" "MK3.5" "XL" "iX")
set_feature_for_printers(HAS_BOWDEN "MINI")
set(PRINTERS_WITH_PUPPIES_BOOTLOADER "XL" "iX" "XL_DEV_KIT")
set(PRINTERS_WITH_DWARF "XL" "XL_DEV_KIT")
set_feature_for_printers_master_board(HAS_MODULARBED "iX" "XL" "XL_DEV_KIT")
set_feature_for_printers(HAS_TOOLCHANGER "XL" "XL_DEV_KIT")
set_feature_for_printers(HAS_SIDE_FSENSOR "iX" "XL")
set_feature_for_printers(HAS_ADC_SIDE_FSENSOR "XL")
set_feature_for_printers(HAS_ESP_FLASH_TASK "MK4" "MK3.5" "XL" "MINI") # iX does not need ESP
                                                                       # flashing
set_feature_for_printers(HAS_EMBEDDED_ESP32 "XL")
set(PRINTERS_WITH_SIDE_LEDS "XL" "iX")
set(PRINTERS_WITH_TRANSLATIONS "MK4" "MK3.5" "XL" "MINI")
set(PRINTERS_WITH_EXTFLASH_TRANSLATIONS "MINI")
set_feature_for_printers(HAS_LOVE_BOARD "MK4" "iX")
set_feature_for_printers(HAS_TMC_UART "MINI")
set_feature_for_printers(HAS_XLCD "MK4" "MK3.5" "iX" "XL")
set_feature_for_printers(HAS_MMU2 "MK4" "MK3.5")
set_feature_for_printers(HAS_CONFIG_STORE_WO_BACKEND "XL_DEV_KIT")
set_feature_for_printers_master_board(HAS_CHAMBER_API "XL")
set_feature_for_printers_master_board(XL_ENCLOSURE_SUPPORT "XL")
set_feature_for_printers(HAS_SWITCHED_FAN_TEST "MK4" "MK3.5")
set_feature_for_printers(HAS_HOTEND_TYPE_SUPPORT "MK4" "MK3.5" "iX")

# Set GUI settings
set(PRINTERS_WITH_GUI "MINI" "MK4" "MK3.5" "XL" "iX")
set(PRINTERS_WITH_GUI_W480H320 "MK4" "MK3.5" "XL" "iX")
set(PRINTERS_WITH_GUI_W240H320 "MINI")
set_feature_for_printers(HAS_LEDS "MK4" "MK3.5" "XL" "iX")
# disable serial printing for MINI to save flash
set_feature_for_printers(HAS_SERIAL_PRINT "MK4" "MK3.5" "XL" "iX" "MINI")

set_feature_for_printers(HAS_LOCAL_ACCELEROMETER "MK3.5" "MK4" "iX")
set_feature_for_printers(HAS_REMOTE_ACCELEROMETER "XL" "XL_DEV_KIT")

set_feature_for_printers(HAS_COLDPULL "MK3.5" "MK4" "XL")

set_feature_for_printers(HAS_BED_LEVEL_CORRECTION "MK3.5" "MINI")

set_feature_for_printers(HAS_SHEET_SUPPORT "MINI" "MK3.5")

set_feature_for_printers(HAS_NFC "MK3.5" "MK4")

set_feature_for_printers(HAS_NOZZLE_CLEANER "iX")
set_feature_for_printers(HAS_BELT_TUNING "XL" "iX")
set_feature_for_printers_master_board(HAS_I2C_EXPANDER "MK3.5" "MK4")
set_feature_for_printers(HAS_WASTEBIN "iX")
set_feature_for_printers(HAS_GEARS_CALIBRATION "MK4")

# Set printer board
set(BOARDS_WITH_ADVANCED_POWER "XBUDDY" "XLBUDDY" "DWARF")
set(BOARDS_WITH_ILI9488 "XBUDDY" "XLBUDDY")
set(BOARDS_WITH_ST7789V "BUDDY")
set(BOARDS_WITH_ACCELEROMETER "XBUDDY" "DWARF")
set(BOARDS_WITH_ARDUINO_USB_SERIAL "BUDDY" "XBUDDY" "XLBUDDY")

if(${TRANSLATIONS_ENABLED} STREQUAL "<default>")
  if(${PRINTER} IN_LIST PRINTERS_WITH_TRANSLATIONS)
    set(TRANSLATIONS_ENABLED YES)
    if(${PRINTER} IN_LIST PRINTERS_WITH_EXTFLASH_TRANSLATIONS)
      set(TRANSLATIONS_IN_EXTFLASH YES)
    else()
      set(TRANSLATIONS_IN_EXTFLASH NO)
    endif()
    define_boolean_option(TRANSLATIONS_IN_EXTFLASH ${TRANSLATIONS_IN_EXTFLASH})
  else()
    set(TRANSLATIONS_ENABLED NO)
  endif()

endif()
define_boolean_option(HAS_TRANSLATIONS ${TRANSLATIONS_ENABLED})

# Set language options
set(LANGUAGES_AVAILABLE
    CS
    DE
    ES
    FR
    IT
    JA
    PL
    )
if("${TRANSLATIONS_LIST}" STREQUAL "<default>")
  if(PRINTER STREQUAL "MINI"
     OR (CMAKE_BUILD_TYPE STREQUAL "Debug" AND (NOT ${TRANSLATIONS_IN_EXTFLASH}))
     )
    # Do not include translations to some build - Mini has explicitly listed translations - Debug
    # builds has translations disabled (due to FLASH space reasons), unless translations are in
    # extflash than its fine
  else()
    # include all translations
    foreach(LANG ${LANGUAGES_AVAILABLE})
      define_boolean_option("ENABLE_TRANSLATION_${LANG}" yes)
    endforeach()
  endif()
else()
  set(TRANSLATIONS_LIST_FOREACH ${TRANSLATIONS_LIST})
  message(STATUS "Translation list: ${TRANSLATIONS_LIST}")
  foreach(LANG ${TRANSLATIONS_LIST_FOREACH})
    string(TOUPPER ${LANG} LANG)
    define_boolean_option(ENABLE_TRANSLATION_${LANG} yes)
  endforeach()
endif()

foreach(LANG ${LANGUAGES_AVAILABLE})
  if(NOT DEFINED "ENABLE_TRANSLATION_${LANG}")
    define_boolean_option("ENABLE_TRANSLATION_${LANG}" no)
  endif()
endforeach()

if(${TOUCH_ENABLED} STREQUAL "<default>")
  if(${PRINTER} MATCHES "^(iX)$")
    set(TOUCH_ENABLED NO)
  elseif((${BOARD} STREQUAL "XBUDDY") OR ${BOARD} STREQUAL "XLBUDDY")
    set(TOUCH_ENABLED YES)
  else()
    set(TOUCH_ENABLED NO)
  endif()
endif()
define_boolean_option(HAS_TOUCH ${TOUCH_ENABLED})

if(${PRINTER} IN_LIST PRINTERS_WITH_FILAMENT_SENSOR_BINARY AND BOARD_IS_MASTER_BOARD)
  set(FILAMENT_SENSOR BINARY)
elseif(${PRINTER} IN_LIST PRINTERS_WITH_FILAMENT_SENSOR_ADC AND BOARD_IS_MASTER_BOARD)
  set(FILAMENT_SENSOR ADC)
else()
  set(FILAMENT_SENSOR NO)
endif()
define_enum_option(NAME FILAMENT_SENSOR VALUE "${FILAMENT_SENSOR}" ALL_VALUES "BINARY;ADC;NO")

if(${RESOURCES} STREQUAL "<auto>")
  if(${PRINTER} IN_LIST PRINTERS_WITH_RESOURCES AND BOARD_IS_MASTER_BOARD)
    set(RESOURCES "YES")
  else()
    set(RESOURCES "NO")
  endif()
endif()
define_boolean_option(RESOURCES ${RESOURCES})

# A DFU file with bootloader always requires a BBF
if(RESOURCES OR (GENERATE_DFU AND BOOTLOADER))
  set(GENERATE_BBF "YES")
else()
  set(GENERATE_BBF "NO")
endif()

if(${PRINTER} IN_LIST PRINTERS_WITH_GUI AND BOARD_IS_MASTER_BOARD)
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

if(ENABLE_BURST
   AND ${PRINTER} IN_LIST PRINTERS_WITH_BURST_STEPPING
   AND BOARD_IS_MASTER_BOARD
   )
  set(HAS_BURST_STEPPING YES)
else()
  set(HAS_BURST_STEPPING NO)
endif()
define_boolean_option(HAS_BURST_STEPPING ${HAS_BURST_STEPPING})

if((${BOARD} STREQUAL "DWARF") OR (${BOARD} STREQUAL "XBUDDY" AND NOT PRINTER STREQUAL "MK3.5"))
  set(HAS_LOADCELL_HX717 YES)
else()
  set(HAS_LOADCELL_HX717 NO)
endif()
define_boolean_option(HAS_LOADCELL_HX717 ${HAS_LOADCELL_HX717})

if(${BOARD} IN_LIST BOARDS_WITH_ADVANCED_POWER)
  set(HAS_ADVANCED_POWER YES)
else()
  set(HAS_ADVANCED_POWER NO)
endif()
define_boolean_option(HAS_ADVANCED_POWER ${HAS_ADVANCED_POWER})

if(${BOARD} IN_LIST BOARDS_WITH_ACCELEROMETER)
  set(HAS_ACCELEROMETER YES)
else()
  set(HAS_ACCELEROMETER NO)
endif()
define_boolean_option(HAS_ACCELEROMETER ${HAS_ACCELEROMETER})

if(${TOUCH_ENABLED})
  set(HAS_XLCD_TOUCH_DRIVER YES)
else()
  set(HAS_XLCD_TOUCH_DRIVER NO)
endif()
message(STATUS "XLCD_TOUCH_DRIVER: ${HAS_XLCD_TOUCH_DRIVER}")

if(${PRINTER} IN_LIST PRINTERS_WITH_DWARF
   OR HAS_MODULARBED
   AND BOARD_IS_MASTER_BOARD
   )
  set(HAS_PUPPIES YES)
else()
  set(HAS_PUPPIES NO)
endif()
define_boolean_option(HAS_PUPPIES ${HAS_PUPPIES})

if(${PRINTER} IN_LIST PRINTERS_WITH_DWARF AND BOARD_IS_MASTER_BOARD)
  set(HAS_DWARF YES)
else()
  set(HAS_DWARF NO)
endif()
define_boolean_option(HAS_DWARF ${HAS_DWARF})

if(HAS_PUPPIES)
  set(ENABLE_PUPPY_BOOTLOAD
      "YES"
      CACHE
        BOOL
        "Pack puppy firmwares into resources and bootload them on startup of the printer with puppies"
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

if(BOARD STREQUAL "XL_DEV_KIT_XLB")
  set(PUPPY_SKIP_FLASH_FW
      "ON"
      CACHE BOOL "Disable flashing puppies to debug puppy with bootloader."
      )
else()
  set(PUPPY_SKIP_FLASH_FW
      "OFF"
      CACHE BOOL "Disable flashing puppies to debug puppy with bootloader."
      )
endif()

if(${PRINTER} IN_LIST PRINTERS_WITH_PUPPIES_BOOTLOADER
   AND BOARD_IS_MASTER_BOARD
   AND (RESOURCES OR PUPPY_SKIP_FLASH_FW)
   AND ENABLE_PUPPY_BOOTLOAD
   )
  set(HAS_PUPPIES_BOOTLOADER YES)
else()
  set(HAS_PUPPIES_BOOTLOADER NO)
endif()
define_boolean_option(HAS_PUPPIES_BOOTLOADER ${HAS_PUPPIES_BOOTLOADER})

if(${HAS_PUPPIES_BOOTLOADER} AND NOT ${PUPPY_SKIP_FLASH_FW})
  set(PUPPY_FLASH_FW YES)
else()
  set(PUPPY_FLASH_FW NO)
endif()
define_boolean_option(PUPPY_FLASH_FW ${PUPPY_FLASH_FW})

if(${PRINTER} IN_LIST PRINTERS_WITH_SIDE_LEDS AND NOT IS_KNOBLET)
  define_boolean_option(HAS_SIDE_LEDS YES)
else()
  define_boolean_option(HAS_SIDE_LEDS NO)
endif()

if(HAS_TOOLCHANGER)
  set(HAS_FILAMENT_SENSORS_MENU YES)
else()
  set(HAS_FILAMENT_SENSORS_MENU NO)
endif()
define_boolean_option(HAS_FILAMENT_SENSORS_MENU ${HAS_FILAMENT_SENSORS_MENU})

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(DEBUG YES)
  define_boolean_option(NETWORKING_BENCHMARK_ENABLED YES)
else()
  set(DEBUG NO)
  define_boolean_option(NETWORKING_BENCHMARK_ENABLED NO)
endif()

# define enabled features

if(BOOTLOADER STREQUAL "YES"
   AND (PRINTER STREQUAL "MINI"
        OR PRINTER STREQUAL "MK4"
        OR PRINTER STREQUAL "MK3.5"
        OR PRINTER STREQUAL "iX"
        OR BOARD STREQUAL "XLBUDDY"
       )
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

set(DEBUG_WITH_BEEPS
    "OFF"
    CACHE BOOL "Colleague annoyance: achievement unlocked"
    )
define_boolean_option(DEBUG_WITH_BEEPS ${DEBUG_WITH_BEEPS})

# Use websocket to talk to Connect instead of many http requests.
#
# The server part is not ready and the protocol is in a flux too. For that reason, this is not
# enabled in "real builds", but we need to be able to have the code around and be able to turn it on
# for custom build - to allow debugging the server too.
#
# Eventually, this'll become the only used and supported way to talk to Connect. At that point, both
# this option and the "old" code will be removed.
set(WEBSOCKET
    "OFF"
    CACHE BOOL "Use websocket to talk to connect. In development"
    )
define_boolean_option(WEBSOCKET ${WEBSOCKET})
set(MDNS
    "ON"
    CACHE BOOL "Enable MDNS responder"
    )
define_boolean_option(MDNS ${MDNS})
