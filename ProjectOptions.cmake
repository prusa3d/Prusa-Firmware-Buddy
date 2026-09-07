#
# Command Line Options
#
# You should specify those options when invoking CMake. Example:
# ~~~
# cmake .. <other options> -DPRINTER=MINI
# ~~~

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PRINTER_VALID_OPTS
    "COREONE"
    "COREONE_INDX"
    "COREONEL"
    "COREONEL_INDX"
    "MINI"
    "MK4"
    "MK3.5"
    "XL"
    "iX"
    "XL_DEV_KIT"
    "NONE"
    )
set(BOARD_VALID_OPTS
    "BUDDY"
    "XBUDDY"
    "XLBUDDY"
    "DWARF"
    "MODULARBED"
    "XL_DEV_KIT_XLB"
    "XBUDDY_EXTENSION"
    "ANFC"
    "TOOL_OFFSET_SENSOR"
    "INDX_HEAD"
    )
set(MCU_VALID_OPTS
    "<default>"
    "STM32F407VG"
    "STM32F429VI"
    "STM32F427ZI"
    "STM32G070RBT6"
    "STM32H503CBU7"
    "STM32C092KCUX"
    )
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
set(MCU
    "<default>"
    CACHE
      STRING
      "Select the MCU for which you want to compile the project (valid values are ${MCU_VALID_OPTS})."
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
set(SIGNATURE_OAK
    "OFF"
    CACHE BOOL "Build Signature Oak variant (luxury limited edition with brass UI theme)"
    )
if(SIGNATURE_OAK AND NOT PRINTER STREQUAL "COREONE")
  message(FATAL_ERROR "SIGNATURE_OAK is only supported for COREONE builds")
endif()
define_boolean_option(SIGNATURE_OAK ${SIGNATURE_OAK})

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
define_boolean_option(BOARD_IS_MASTER_BOARD ${BOARD_IS_MASTER_BOARD})

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
  elseif(${BOARD} STREQUAL "XBUDDY_EXTENSION")
    set(MCU "STM32H503CBU7")
  elseif(${BOARD} STREQUAL "ANFC")
    set(MCU "STM32C092KCUX")
  elseif(${BOARD} STREQUAL "TOOL_OFFSET_SENSOR")
    set(MCU "STM32C092KCUX")
  elseif(${BOARD} STREQUAL "INDX_HEAD")
    set(MCU "STM32C092KCUX")
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
   OR ${BOARD} STREQUAL "XBUDDY_EXTENSION"
   OR ${BOARD} STREQUAL "ANFC"
   OR ${BOARD} STREQUAL "INDX_HEAD"
   OR ${BOARD} STREQUAL "XL_DEV_KIT_XLB"
   OR ${BOARD} STREQUAL "TOOL_OFFSET_SENSOR"
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
message(STATUS "MCU: ${MCU}")
message(STATUS "Custom Compile Options (C/C++ flags): ${CUSTOM_COMPILE_OPTIONS}")
message(STATUS "Web User Interface: ${WUI}")
message(STATUS "Connect client: ${CONNECT}")
message(STATUS "Resources: ${RESOURCES}")

# Set printer features
function(set_feature_for_printers FEATURE_NAME)
  set(FEATURE_PRINTER_LIST ${ARGV})
  list(REMOVE_AT FEATURE_PRINTER_LIST 0) # First argument is the feature name
  if(DEFINED ${FEATURE_NAME})
    # override from manual configuration
    set(FEATURE_VALUE ${${FEATURE_NAME}})
  elseif(${PRINTER} IN_LIST FEATURE_PRINTER_LIST)
    # set from feature list
    set(FEATURE_VALUE YES)
  elseif("UNITTESTS" IN_LIST FEATURE_PRINTER_LIST AND UNITTESTS_ENABLE)
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
  if(BOARD_IS_MASTER_BOARD)
    if(DEFINED ${FEATURE_NAME})
      # override from manual configuration
      set(FEATURE_VALUE ${${FEATURE_NAME}})
    elseif(${PRINTER} IN_LIST FEATURE_PRINTER_LIST)
      # set from feature list
      set(FEATURE_VALUE YES)
    elseif("UNITTESTS" IN_LIST FEATURE_PRINTER_LIST AND UNITTESTS_ENABLE)
      set(FEATURE_VALUE YES)
    else()
      set(FEATURE_VALUE NO)
    endif()
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
set(PRINTERS_WITH_FILAMENT_SENSOR_ADC "MK4" "XL" "iX" "XL_DEV_KIT" "COREONE" "COREONEL")

set_feature_for_printers(
  HAS_TRINAMIC
  "MINI"
  "MK4"
  "MK3.5"
  "iX"
  "XL"
  "XL_DEV_KIT"
  "COREONE"
  "COREONE_INDX"
  "COREONEL"
  "COREONEL_INDX"
  )
set_feature_for_printers_master_board(
  HAS_PAUSE
  "MINI"
  "MK4"
  "MK3.5"
  "iX"
  "XL"
  "XL_DEV_KIT"
  "COREONE"
  "COREONE_INDX"
  "COREONEL"
  "COREONEL_INDX"
  )
# CRASH_DETECTION requires SELFTEST to work
set_feature_for_printers_master_board(
  HAS_CRASH_DETECTION
  "MINI"
  "MK4"
  "MK3.5"
  "iX"
  "XL"
  "COREONE"
  "COREONE_INDX"
  "COREONEL"
  "COREONEL_INDX"
  )
# Detection of fallen tools
set_feature_for_printers_master_board(HAS_TOOL_CRASH_RECOVERY "XL" "COREONE_INDX" "COREONEL_INDX")
# POWER_PANIC requires SELFTEST and CRASH_DETECTION to work
set_feature_for_printers_master_board(
  HAS_POWER_PANIC
  "MK4"
  "MK3.5"
  "iX"
  "XL"
  "COREONE"
  "COREONE_INDX"
  "COREONEL"
  "COREONEL_INDX"
  )
define_enum_option(NAME POWER_PANIC_STORAGE VALUE FLASH ALL_VALUES "FLASH;BKPSRAM")
# Probing for the print sheet during Z_SAFE_HOMING
set_feature_for_printers_master_board(
  HAS_PRINT_SHEET_DETECTION
  "MK4"
  "iX"
  "COREONE"
  "COREONE_INDX"
  "COREONEL"
  "COREONEL_INDX"
  )
set_feature_for_printers(HAS_PRECISE_HOMING "MK4" "MK3.5")
set_feature_for_printers(
  HAS_SELFTEST_DEPENDENCIES "COREONE" "COREONEL" "COREONE_INDX" "COREONEL_INDX" "XL"
  )
set_feature_for_printers(
  HAS_PRECISE_HOMING_COREXY
  "iX"
  "XL"
  "XL_DEV_KIT"
  "COREONE"
  "COREONE_INDX"
  "COREONEL"
  "COREONEL_INDX"
  )
set_feature_for_printers(
  HAS_SWITCHABLE_HOMING_CALIBRATION "iX" "XL" "XL_DEV_KIT" "COREONE" "COREONEL"
  )
set_feature_for_printers_master_board(
  HAS_PHASE_STEPPING
  "XL"
  "iX"
  "COREONE"
  "COREONE_INDX"
  "COREONEL"
  "COREONEL_INDX"
  "MK4"
  )
set_feature_for_printers_master_board(
  HAS_PHASE_STEPPING_SELFTEST "iX" "XL" "COREONE_INDX" "COREONEL" "COREONEL_INDX"
  )
set_feature_for_printers_master_board(
  HAS_PHASE_STEPPING_CALIBRATION
  "XL"
  "iX"
  "COREONE"
  "COREONE_INDX"
  "COREONEL"
  "COREONEL_INDX"
  "MK4"
  )
set(PRINTERS_WITH_BURST_STEPPING
    "XL"
    "MK4"
    "iX"
    "COREONE"
    "COREONE_INDX"
    "COREONEL"
    "COREONEL_INDX"
    )
set_feature_for_printers_master_board(
  HAS_INPUT_SHAPER_CALIBRATION
  "MK4"
  "MK3.5"
  "XL"
  "XL_DEV_KIT"
  "COREONE"
  "COREONE_INDX"
  "COREONEL"
  "COREONEL_INDX"
  )
set_feature_for_printers(
  HAS_SELFTEST
  "MK4"
  "MK3.5"
  "XL"
  "iX"
  "MINI"
  "COREONE"
  "COREONE_INDX"
  "COREONEL"
  "COREONEL_INDX"
  )
# Heater selftest done as a gcode-based FSM wizard (M1987) instead of the legacy mask-based
# CSelftest state machine. All selftest printers except XL (which keeps the legacy path).
set_feature_for_printers(
  HAS_HEATERS_SELFTEST_GCODE
  "MK4"
  "MK3.5"
  "iX"
  "MINI"
  "COREONE"
  "COREONE_INDX"
  "COREONEL"
  "COREONEL_INDX"
  )
# Bed heater fail -> "refit the steel sheet and retry" prompt (legacy: MK4 / MK3.5 / MINI).
set_feature_for_printers(HAS_HEATERS_SELFTEST_BED_SHEET_RETRY "MK4" "MK3.5" "MINI")
# Nozzle heater fail -> "revise printer setup" (ScreenPrinterSetup) prompt. Legacy had this only on
# MK4 / MK3.5; MINI offered just the bed-sheet retry above.
set_feature_for_printers(HAS_HEATERS_SELFTEST_REVISE "MK4" "MK3.5")
set_feature_for_printers(
  HAS_HUMAN_INTERACTIONS
  "MINI"
  "MK4"
  "MK3.5"
  "XL"
  "COREONE"
  "COREONE_INDX"
  "COREONEL"
  "COREONEL_INDX"
  )
set_feature_for_printers_master_board(
  HAS_LOADCELL
  "MK4"
  "iX"
  "XL"
  "XL_DEV_KIT"
  "COREONE"
  "COREONE_INDX"
  "COREONEL"
  "COREONEL_INDX"
  )
set_feature_for_printers_master_board(
  HAS_NEXTRUDER
  "MK4"
  "iX"
  "XL"
  "XL_DEV_KIT"
  "COREONE"
  "COREONEL"
  )
# Compensation of the nozzle thermal expansion between bed leveling and printing
set_feature_for_printers_master_board(HAS_NOZZLE_THERMAL_COMPENSATION "XL")
set_feature_for_printers_master_board(HAS_SHEET_PROFILES "MK3.5" "MINI")
set_feature_for_printers_master_board(
  HAS_HEATBREAK_TEMP
  "MK4"
  "iX"
  "XL"
  "XL_DEV_KIT"
  "COREONE"
  "COREONEL"
  )
set_feature_for_printers_master_board(HAS_FILAMENT_HEATBREAK_PARAM "iX")
set_feature_for_printers_master_board(
  HAS_FILAMENT_BASE_PRESET_PARAM "COREONE_INDX" "COREONEL_INDX" "iX"
  )
set(PRINTERS_WITH_RESOURCES
    "MINI"
    "MK4"
    "MK3.5"
    "XL"
    "iX"
    "COREONE"
    "COREONE_INDX"
    "COREONEL"
    "COREONEL_INDX"
    )
set_feature_for_printers(HAS_BOWDEN "MINI")
set(PRINTERS_WITH_DWARF "XL" "XL_DEV_KIT")

# MODULAR_BED is a bed consisting of several bedlets
set_feature_for_printers_master_board(HAS_MODULAR_BED "iX" "XL" "XL_DEV_KIT")
# REMOTE_BED means there is a daughterboard controlling the bed
set_feature_for_printers_master_board(
  HAS_REMOTE_BED "iX" "XL" "XL_DEV_KIT" "COREONEL" "COREONEL_INDX"
  )
# LOCAL_BED means the motherboard is directly controlling the bed
set_feature_for_printers_master_board(HAS_LOCAL_BED "COREONE" "COREONE_INDX" "MINI" "MK4" "MK3.5")
# PUPPY_MODULARBED is remote modular bed implemented as a puppy, i.e. communicating over modbus
set_feature_for_printers_master_board(HAS_PUPPY_MODULARBED "iX" "XL" "XL_DEV_KIT")

set_feature_for_printers_master_board(
  HAS_XBUDDY_EXTENSION "COREONE" "COREONE_INDX" "COREONEL" "COREONEL_INDX" "iX"
  )
if(NOT HAS_XBUDDY_EXTENSION OR NOT DEFINED XBUDDY_EXTENSION_VARIANT)
  set(XBUDDY_EXTENSION_VARIANT "NONE")
endif()
define_enum_option(
  NAME XBUDDY_EXTENSION_VARIANT VALUE "${XBUDDY_EXTENSION_VARIANT}" ALL_VALUES "NONE;STANDARD;iX"
  )

# MK4 technically doesn't have door sensor but needs to check valid FW-HW
set_feature_for_printers_master_board(
  HAS_DOOR_SENSOR "COREONE" "COREONE_INDX" "COREONEL" "COREONEL_INDX" "MK4"
  )

set_feature_for_printers_master_board(
  HAS_TOOLCHANGER "XL" "XL_DEV_KIT" "COREONE_INDX" "COREONEL_INDX"
  )
set_feature_for_printers(
  HAS_SIDE_FSENSOR
  "iX"
  "XL"
  "COREONE"
  "COREONEL"
  "COREONE_INDX"
  "COREONEL_INDX"
  )
set_feature_for_printers(HAS_ADC_SIDE_FSENSOR "XL")
set_feature_for_printers(HAS_SIDE_FSENSOR_INVERTIBLE "COREONE_INDX" "COREONEL_INDX")
set_feature_for_printers(HAS_FSENSOR_INVERTIBLE)
set_feature_for_printers_master_board(HAS_SIDE_FSENSOR_REMAP "XL")
set_feature_for_printers(
  HAS_FILAMENT_SENSORS_MENU "XL" "COREONE" "COREONE_INDX" "COREONEL" "COREONEL_INDX"
  )

set_feature_for_printers(
  HAS_ESP
  "MK4"
  "MK3.5"
  "XL"
  "MINI"
  "COREONE"
  "COREONE_INDX"
  "COREONEL"
  "COREONEL_INDX"
  )

set_feature_for_printers(HAS_EMBEDDED_ESP32 "XL")
set_feature_for_printers_master_board(HAS_INTERNAL_STORAGE_FLASH "iX")
set(PRINTERS_WITH_SIDE_LEDS "XL" "COREONE" "COREONE_INDX" "COREONEL" "COREONEL_INDX")
set(PRINTERS_WITH_TRANSLATIONS
    "COREONE"
    "COREONE_INDX"
    "COREONEL"
    "COREONEL_INDX"
    "MK4"
    "MK3.5"
    "XL"
    "MINI"
    )
set_feature_for_printers(HAS_LOVE_BOARD "MK4" "iX" "COREONE" "COREONEL")
set_feature_for_printers(HAS_TMC_UART "MINI")
set_feature_for_printers(
  HAS_XLCD
  "MK4"
  "MK3.5"
  "iX"
  "XL"
  "COREONE"
  "COREONE_INDX"
  "COREONEL"
  "COREONEL_INDX"
  )
set_feature_for_printers(HAS_MMU2 "MK4" "MK3.5" "COREONE" "COREONEL")
set_feature_for_printers(HAS_CONFIG_STORE_WO_BACKEND "XL_DEV_KIT")
set_feature_for_printers_master_board(
  HAS_CHAMBER_API "XL" "COREONE" "COREONE_INDX" "COREONEL" "COREONEL_INDX"
  )
set_feature_for_printers_master_board(
  HAS_CHAMBER_FILTRATION_API "COREONE" "COREONE_INDX" "COREONEL" "COREONEL_INDX" "XL"
  )
set_feature_for_printers(HAS_CYPHAL_TIMESYNC "HONEYBEE_NEXT")
set_feature_for_printers_master_board(XL_ENCLOSURE_SUPPORT "XL")
set_feature_for_printers(HAS_SWITCHED_FAN_TEST "MK4" "MK3.5" "COREONE" "COREONEL")
set_feature_for_printers_master_board(
  HAS_HOTEND_TYPE_SUPPORT
  "MK4"
  "MK3.5"
  "iX"
  "COREONE"
  "COREONEL"
  "XL"
  )
set_feature_for_printers(HAS_EMERGENCY_STOP "COREONE" "COREONE_INDX" "COREONEL" "COREONEL_INDX")
set_feature_for_printers(HAS_15GT_BELTS "COREONE" "COREONE_INDX" "COREONEL" "COREONEL_INDX" "XL")
# The NTC and PT1000 ADC ranges overlap, so boot detection cannot tell them apart on a warm restart
if(DEVELOPMENT_ITEMS_ENABLED)
  set_feature_for_printers(HAS_HT_HOTEND "COREONE" "COREONEL")
else()
  set_feature_for_printers(HAS_HT_HOTEND)
endif()
set_feature_for_printers(HAS_EXPANSION_JOINTS_GEN_2 "COREONE" "COREONE_INDX")
set_feature_for_printers(HAS_CEILING_CLEARANCE "COREONE" "COREONE_INDX" "COREONEL" "COREONEL_INDX")
set_feature_for_printers(
  HAS_CANCEL_OBJECT
  "MK4"
  "MK3.5"
  "iX"
  "COREONE"
  "COREONE_INDX"
  "COREONEL"
  "COREONEL_INDX"
  "XL"
  "MINI"
  )
set_feature_for_printers(
  HAS_AUTO_RETRACT
  "COREONE"
  "COREONE_INDX"
  "COREONEL"
  "COREONEL_INDX"
  "MK4"
  "iX"
  "XL"
  )
set_feature_for_printers(HAS_SWITCHABLE_AUTO_RETRACT "COREONE" "COREONEL" "MK4" "iX" "XL")
set_feature_for_printers(
  HAS_FILAMENT_TRACKER
  "COREONE"
  "COREONE_INDX"
  "COREONEL"
  "COREONEL_INDX"
  "MK4"
  "iX"
  "XL"
  ) # TODO: When INDX gets fixed, also try enabling HAS_ANFC
set_feature_for_printers_master_board(
  HAS_E2EE_SUPPORT
  "MK4"
  "MK3.5"
  "iX"
  "COREONE"
  "COREONE_INDX"
  "COREONEL"
  "COREONEL_INDX"
  "XL"
  "UNITTESTS"
  )

# Printers that support any form of backwards gcode compatibility modes XL: needed for XL-on-XLS
# compat mode (xl_compatibility_mode in M106 fan scaling)
set_feature_for_printers(HAS_GCODE_COMPATIBILITY "MK3.5" "MK4" "COREONE" "COREONEL" "XL")

# Checks for bed evenness during G29 and if it's too uneven, offers Z alignment calibration.
# Requires SELFTEST to work
set_feature_for_printers(HAS_UNEVEN_BED_PROMPT "COREONE" "COREONE_INDX" "COREONEL" "COREONEL_INDX")

set_feature_for_printers_master_board(HAS_TOOL_OFFSET_PIN_CALIBRATION "XL" "XL_DEV_KIT")

set_feature_for_printers(
  HAS_DOOR_SENSOR_CALIBRATION "COREONE" "COREONE_INDX" "COREONEL" "COREONEL_INDX"
  )

# Set GUI settings
set(PRINTERS_WITH_GUI
    "COREONE"
    "COREONE_INDX"
    "COREONEL"
    "COREONEL_INDX"
    "MINI"
    "MK4"
    "MK3.5"
    "XL"
    "iX"
    )
set(PRINTERS_WITH_GUI_W480H320
    "COREONE"
    "COREONE_INDX"
    "COREONEL"
    "COREONEL_INDX"
    "MK4"
    "MK3.5"
    "XL"
    "iX"
    )
set(PRINTERS_WITH_GUI_W240H320 "MINI")
set_feature_for_printers(
  HAS_LEDS
  "MK4"
  "MK3.5"
  "XL"
  "iX"
  "COREONE"
  "COREONE_INDX"
  "COREONEL"
  "COREONEL_INDX"
  )
# disable serial printing for MINI to save flash
set_feature_for_printers(
  HAS_SERIAL_PRINT
  "MK4"
  "MK3.5"
  "XL"
  "iX"
  "MINI"
  "COREONE"
  "COREONE_INDX"
  "COREONEL"
  "COREONEL_INDX"
  )

# Local accelerometer communicates directly over SPI
set_feature_for_printers(HAS_LOCAL_ACCELEROMETER "MK3.5" "MK4" "iX" "COREONE" "COREONEL")
# Remote accelerometer communicates indirectly over MODBUS
set_feature_for_printers(HAS_REMOTE_ACCELEROMETER "XL" "XL_DEV_KIT" "COREONE_INDX" "COREONEL_INDX")
# Some printers require manual mounting of accelerometer to the board, nozzle or bed
set_feature_for_printers(HAS_ATTACHABLE_ACCELEROMETER "MK3.5" "MK4" "COREONE")

set_feature_for_printers(HAS_COLDPULL "MK3.5" "MK4" "XL" "COREONE" "COREONEL")

set_feature_for_printers(HAS_BED_LEVEL_CORRECTION "MK3.5" "MINI")

set_feature_for_printers(HAS_SHEET_SUPPORT "MINI" "MK3.5")

set_feature_for_printers(
  HAS_NFC
  "MK3.5"
  "MK4"
  "COREONE"
  "COREONE_INDX"
  "COREONEL"
  "COREONEL_INDX"
  )

set_feature_for_printers(HAS_NOZZLE_CLEANER "iX" "COREONE_INDX" "COREONEL_INDX")
set_feature_for_printers(HAS_NOZZLE_CLEANER_LITE "XL" "COREONE" "COREONEL")
set_feature_for_printers(
  HAS_MANUAL_BELT_TUNING "COREONE" "COREONE_INDX" "COREONEL" "COREONEL_INDX" "iX"
  )
set_feature_for_printers_master_board(
  HAS_I2C_EXPANDER
  "MK3.5"
  "MK4"
  "COREONE"
  "COREONE_INDX"
  "COREONEL"
  "COREONEL_INDX"
  )
set_feature_for_printers(HAS_WASTEBIN "iX" "COREONE_INDX" "COREONEL_INDX")
set_feature_for_printers_master_board(HAS_PRINT_FAN_TYPE "XL")
# CPU cooling fan on the XLBuddy sandwich board. Compiled in for all XL-family builds; actually
# started at runtime only on the XLS variant.
set_feature_for_printers_master_board(HAS_CPU_FAN "XL")
# GEARBOX_ALIGNMENT requires SELFTEST
set_feature_for_printers_master_board(HAS_GEARBOX_ALIGNMENT "MK4" "COREONE" "COREONEL" "XL")
set_feature_for_printers_master_board(
  HAS_CHAMBER_VENTS "COREONE" "COREONE_INDX" "COREONEL" "COREONEL_INDX"
  )
set_feature_for_printers_master_board(HAS_BED_FAN "COREONEL" "COREONEL_INDX")
set_feature_for_printers_master_board(HAS_PSU_FAN "COREONEL" "COREONEL_INDX")
set_feature_for_printers(HAS_AC_CONTROLLER "COREONEL" "COREONEL_INDX")

set_feature_for_printers(HAS_TOOL_OFFSET_SENSOR "COREONE_INDX" "COREONEL_INDX" "XL")

# Coil layout of the contactless tool-offset sensor: single-coil sweeps both axes over one coil,
# dual-coil (XLS) sweeps each axis over its own coil. An image only ever uses one of them, so only
# the matching measurement flow is compiled in.
if(NOT HAS_TOOL_OFFSET_SENSOR)
  set(TOOL_OFFSET_SENSOR_GEOMETRY "NONE")
elseif(PRINTER STREQUAL "XL")
  set(TOOL_OFFSET_SENSOR_GEOMETRY "DUAL_COIL")
else()
  set(TOOL_OFFSET_SENSOR_GEOMETRY "SINGLE_COIL")
endif()
define_enum_option(
  NAME TOOL_OFFSET_SENSOR_GEOMETRY VALUE "${TOOL_OFFSET_SENSOR_GEOMETRY}" ALL_VALUES
  "NONE;SINGLE_COIL;DUAL_COIL"
  )

# XL-CAN puppy. Compiled in for the XL family (shared xlBuddy master image); discovered at bootstrap
# and gated at runtime via XlCan::is_enabled(). Plain XL leaves it disabled (no bridge on the bus);
# XLS enables it when bootstrap finds the bridge on dock 9. Per the XLS HW spec every XLS has the
# bridge, so missing-bridge on XLS is a hardware fault rather than a normal config.
set_feature_for_printers_master_board(HAS_XL_CAN "XL")

if(DEVELOPMENT_ITEMS_ENABLED)
  set_feature_for_printers(HAS_ANFC "COREONE" "COREONEL")
else()
  set_feature_for_printers(HAS_ANFC)
endif()
set_feature_for_printers(HAS_HEATBED_SCREWS_DURING_TRANSPORT "COREONEL" "COREONEL_INDX")

# Use websocket to talk to Connect instead of many http requests.
#
# iX can't have websockets yet because of AFS version of Connect, all other printers should have it
# enabled.
#
# Eventually, this'll become the only used and supported way to talk to Connect. At that point, both
# this option and the "old" code will be removed.
set_feature_for_printers(
  WEBSOCKET
  "MINI"
  "MK3.5"
  "MK4"
  "XL"
  "COREONE"
  "COREONE_INDX"
  "COREONEL"
  "COREONEL_INDX"
  "XL_DEV_KIT"
  )

set_feature_for_printers(HAS_INDX "COREONE_INDX" "COREONEL_INDX")
set_feature_for_printers(HAS_INDX_HEAD "COREONE_INDX" "COREONEL_INDX")
set_feature_for_printers(HAS_MOTOR_CURRENT_PROFILES "COREONE_INDX" "COREONEL_INDX")

# Wastebin fill-tracking (persistent pellet counter + pre-print / mid-print overfill warnings). Only
# INDX printers that actually have a wastebin (CoreOne / CoreOneL INDX).
if(HAS_WASTEBIN AND HAS_INDX)
  define_boolean_option(HAS_WASTEBIN_FILL_TRACKING yes)
else()
  define_boolean_option(HAS_WASTEBIN_FILL_TRACKING no)
endif()

if(HAS_TOOLCHANGER OR HAS_MMU2)
  define_boolean_option(HAS_TOOL_MAPPING yes)
  define_boolean_option(HAS_SPOOL_JOIN yes)
else()
  define_boolean_option(HAS_TOOL_MAPPING no)
  define_boolean_option(HAS_SPOOL_JOIN no)
endif()

if(HAS_TOOLCHANGER AND NOT HAS_INDX)
  define_boolean_option(HAS_PER_TOOL_TEMPERATURES yes)
else()
  define_boolean_option(HAS_PER_TOOL_TEMPERATURES no)
endif()

set_feature_for_printers(HAS_CYPHAL_METRICS)
set_feature_for_printers(HAS_CYPHAL_LOGGING)

# Set printer board
set(BOARDS_WITH_ADVANCED_POWER "XBUDDY" "XLBUDDY" "DWARF")
set(BOARDS_WITH_ILI9488 "XBUDDY" "XLBUDDY")
set(BOARDS_WITH_ST7789V "BUDDY")
set(BOARDS_WITH_ACCELEROMETER "XBUDDY" "DWARF")
set(BOARDS_WITH_USB_DEVICE "BUDDY" "XBUDDY" "XLBUDDY")

if(${TRANSLATIONS_ENABLED} STREQUAL "<default>")
  if(${PRINTER} IN_LIST PRINTERS_WITH_TRANSLATIONS)
    set(TRANSLATIONS_ENABLED YES)
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
    UK
    )
if("${TRANSLATIONS_LIST}" STREQUAL "<default>")
  if(PRINTER STREQUAL "MINI")
    # Do not include translations to some build - Mini has explicitly listed translations
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

if(FILAMENT_SENSOR STREQUAL "NO")
  set(HAS_EXTRUDER_FSENSOR NO)
else()
  set(HAS_EXTRUDER_FSENSOR YES)
endif()
define_boolean_option(HAS_EXTRUDER_FSENSOR ${HAS_EXTRUDER_FSENSOR})

if(${RESOURCES} STREQUAL "<auto>")
  if(${PRINTER} IN_LIST PRINTERS_WITH_RESOURCES AND BOARD_IS_MASTER_BOARD)
    set(RESOURCES "YES")
  else()
    set(RESOURCES "NO")
  endif()
endif()
define_boolean_option(RESOURCES ${RESOURCES})

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
  else()
    message(FATAL_ERROR "Printer with GUI must have resolution set")
  endif()
  message(STATUS "RESOLUTION: ${RESOLUTION}")
else()
  set(GUI NO)
endif()
message(STATUS "Graphical User Interface: ${GUI}")
define_boolean_option(HAS_GUI ${GUI})

if(BOARD_IS_MASTER_BOARD)
  set(HAS_PLANNER true)
else()
  set(HAS_PLANNER false)
endif()
define_boolean_option(HAS_PLANNER ${HAS_PLANNER})

if(ENABLE_BURST
   AND ${PRINTER} IN_LIST PRINTERS_WITH_BURST_STEPPING
   AND BOARD_IS_MASTER_BOARD
   )
  set(HAS_BURST_STEPPING YES)
else()
  set(HAS_BURST_STEPPING NO)
endif()
define_boolean_option(HAS_BURST_STEPPING ${HAS_BURST_STEPPING})

if((${BOARD} STREQUAL "DWARF") OR (${BOARD} STREQUAL "XBUDDY" AND NOT (PRINTER STREQUAL "MK3.5"
                                                                       OR HAS_INDX))
   )
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

if(${PRINTER} IN_LIST PRINTERS_WITH_DWARF AND BOARD_IS_MASTER_BOARD)
  set(HAS_DWARF YES)
else()
  set(HAS_DWARF NO)
endif()
define_boolean_option(HAS_DWARF ${HAS_DWARF})

if(HAS_DWARF
   OR HAS_INDX_HEAD
   OR HAS_PUPPY_MODULARBED
   OR HAS_XBUDDY_EXTENSION
   OR HAS_ANFC
   OR HAS_TOOL_OFFSET_SENSOR
   OR HAS_INDX_HEAD
   OR HAS_XL_CAN
   )
  set(HAS_PUPPIES YES)
else()
  set(HAS_PUPPIES NO)
endif()
define_boolean_option(HAS_PUPPIES ${HAS_PUPPIES})

# Puppy bootstrap is needed for puppies other than XBE (which has its own)
if(HAS_DWARF
   OR HAS_INDX_HEAD
   OR HAS_PUPPY_MODULARBED
   )
  set(HAS_PUPPY_BOOTSTRAP YES)
else()
  set(HAS_PUPPY_BOOTSTRAP NO)
endif()
define_boolean_option(HAS_PUPPY_BOOTSTRAP ${HAS_PUPPY_BOOTSTRAP})

if(${BOARD} IN_LIST BOARDS_WITH_USB_DEVICE)
  set(HAS_USB_DEVICE YES)
else()
  set(HAS_USB_DEVICE NO)
endif()
define_boolean_option(HAS_USB_DEVICE ${HAS_USB_DEVICE})

if(${BOARD} STREQUAL "XBUDDY" AND HAS_MMU2)
  # for XBUDDY based printers, UART6 is being used either for puppies/MODBUS or directly for the MMU
  if(HAS_PUPPIES)
    # UART already occupied by the puppies/MODBUS
    set(HAS_MMU2_OVER_UART NO)
  else()
    # UART used for the MMU
    set(HAS_MMU2_OVER_UART YES)
  endif()
else()
  # other boards: set to NO, whatever new edge cases that may bring...
  set(HAS_MMU2_OVER_UART NO)
endif()
define_boolean_option(HAS_MMU2_OVER_UART ${HAS_MMU2_OVER_UART})

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
  set(INDX_HEAD_BINARY_PATH
      ""
      CACHE
        PATH
        "Where to get the INDX head's binary from. If set, the project won't try to build anything."
      )
  set(MODULARBED_BINARY_PATH
      ""
      CACHE
        PATH
        "Where to get the Modularbed's binary from. If set, the project won't try to build anything."
      )
  set(XBUDDY_EXTENSION_BINARY_PATH
      ""
      CACHE
        PATH
        "Where to get the enclosure exention's binary from. If set, the project won't try to build anything."
      )
  set(ANFC_BINARY_PATH
      ""
      CACHE
        PATH
        "Where to get the active NFC reader's binary from. If set, the project won't try to build anything."
      )
  set(TOOL_OFFSET_SENSOR_BINARY_PATH
      ""
      CACHE
        PATH
        "Where to get the tool offset sensor's binary from. If set, the project won't try to build anything."
      )
  set(XL_CAN_BINARY_PATH
      ""
      CACHE
        PATH
        "Where to get the XLS XL-CAN's binary from. If set, the project won't try to build anything."
      )
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

if(HAS_PUPPIES
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

if(HAS_LEDS
   OR HAS_SIDE_LEDS
   OR HAS_TOOLCHANGER
   )
  set(HAS_LIGHTS_MENU YES)
else()
  set(HAS_LIGHTS_MENU NO)
endif()
define_boolean_option(HAS_LIGHTS_MENU ${HAS_LIGHTS_MENU})

# define enabled features

if(BOOTLOADER STREQUAL "YES"
   AND (PRINTER STREQUAL "COREONE"
        OR PRINTER STREQUAL "COREONE_INDX"
        OR PRINTER STREQUAL "COREONEL"
        OR PRINTER STREQUAL "COREONEL_INDX"
        OR PRINTER STREQUAL "MINI"
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

set(NETWORKING_BENCHMARK_ENABLED
    ${DEBUG}
    CACHE BOOL "Enable network benchmarking instrumentation"
    )
define_boolean_option(NETWORKING_BENCHMARK_ENABLED ${NETWORKING_BENCHMARK_ENABLED})

set(HEAP_INSTRUMENTATION_ENABLED
    "OFF"
    CACHE BOOL "Enable heap profiling instrumentation"
    )
define_boolean_option(HEAP_INSTRUMENTATION_ENABLED ${HEAP_INSTRUMENTATION_ENABLED})

if(BOARD IN_LIST BUDDY_BOARDS)
  set(RTT_METRICS_ENABLED
      ${DEBUG}
      CACHE BOOL "Enable metrics over rtt"
      )
else()
  # Puppies have no rtt_metrics implementation
  set(RTT_METRICS_ENABLED
      OFF
      CACHE BOOL "Enable metrics over rtt" FORCE
      )
endif()
define_boolean_option(RTT_METRICS_ENABLED ${RTT_METRICS_ENABLED})

set(DEVELOPER_MODE
    "OFF"
    CACHE BOOL "Disable wizards, prompts and user-friendliness. Developers like it rough!"
    )
define_boolean_option(DEVELOPER_MODE ${DEVELOPER_MODE})

set(CYPHAL_CAN_STATS
    "OFF"
    CACHE BOOL "Enable mechanisms to measure CAN bus performance (needs DEVELOPER_MODE to be ON)"
    )
define_boolean_option(CYPHAL_CAN_STATS ${CYPHAL_CAN_STATS})

set(SYSDEBUG
    "OFF"
    CACHE BOOL "Disable timeouts to ease debugging of nodes (auto ON for DEBUG and DEVELOPER_MODE)"
    )
if(${DEBUG} OR ${DEVELOPER_MODE})
  # SYSDEBUG is automatically enabled in debug and developer mode
  set(SYSDEBUG "ON")
endif()
define_boolean_option(SYSDEBUG ${SYSDEBUG})

set(DEBUG_WITH_BEEPS
    "OFF"
    CACHE BOOL "Colleague annoyance: achievement unlocked"
    )
define_boolean_option(DEBUG_WITH_BEEPS ${DEBUG_WITH_BEEPS})

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(DISABLE_WATCHDOG
      "ON"
      CACHE BOOL "Disable watchdog handlers for debugging"
      )
else()
  set(DISABLE_WATCHDOG
      "OFF"
      CACHE BOOL "Disable watchdog handlers for debugging"
      )
endif()
define_boolean_option(DISABLE_WATCHDOG ${DISABLE_WATCHDOG})

set(MDNS
    "ON"
    CACHE BOOL "Enable MDNS responder"
    )
define_boolean_option(MDNS ${MDNS})

if(DEBUG)
  set(HAS_EXTRA_EXPERIMENTAL_SETTINGS YES)
else()
  set(HAS_EXTRA_EXPERIMENTAL_SETTINGS NO)
endif()
define_boolean_option(HAS_EXTRA_EXPERIMENTAL_SETTINGS ${HAS_EXTRA_EXPERIMENTAL_SETTINGS})
