add_executable(
  mmu2-modbus_tests
  ${CMAKE_SOURCE_DIR}/src/puppies/xbuddy_extension.cpp test_mmu-codec.cpp
  ${CMAKE_SOURCE_DIR}/lib/Marlin/Marlin/src/feature/prusa/MMU2/protocol_logic.cpp
  ${CMAKE_SOURCE_DIR}/lib/Prusa-Firmware-MMU/src/modules/protocol.cpp
  )

add_compile_definitions(UNITTEST)

set_target_properties(
  mmu2-modbus_tests
  PROPERTIES COMPILE_DEFINITIONS_OVERRIDE "PRINTER_MODEL=\"CUBE\""
             "BOARD=\"XBUDDY\"" "BOARD_VERSION_MAJOR=0"
             "BOARD_VERSION_MINOR=3" "BOARD_VERSION_PATCH=7"
  )

target_include_directories(
  mmu2-modbus_tests
  PRIVATE include ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/src/common/utils
          ${CMAKE_SOURCE_DIR}/src/puppy/xbuddy_extension_shared
  )

add_catch_test(mmu2-modbus_tests)
