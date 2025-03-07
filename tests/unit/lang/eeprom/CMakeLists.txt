# define the test executable
add_executable(
  language_eeprom_tests
  ${CMAKE_SOURCE_DIR}/src/common/random_sw.cpp ${CMAKE_SOURCE_DIR}/src/common/language_eeprom.cpp
  ${CMAKE_SOURCE_DIR}/src/lang/translator.cpp ${CMAKE_SOURCE_DIR}/tests/unit/test_main.cpp
  tests.cpp
  )

# define required search paths
target_include_directories(
  language_eeprom_tests
  PUBLIC .
         ${CMAKE_SOURCE_DIR}/src
         ${CMAKE_SOURCE_DIR}/src/lang
         ${CMAKE_SOURCE_DIR}/src/common
         ${CMAKE_SOURCE_DIR}/src/common/utils
         ${CMAKE_SOURCE_DIR}/src/common/marlin_server_types
         ${CMAKE_SOURCE_DIR}/src/common/fanctl
         ${CMAKE_SOURCE_DIR}/src/gui
         ${CMAKE_SOURCE_DIR}/src/guiconfig
         ${CMAKE_BINARY_DIR}/tests/unit
         ${CMAKE_SOURCE_DIR}/include
         ${CMAKE_BINARY_DIR}/include
         ${CMAKE_SOURCE_DIR}/src/persistent_stores
         ${CMAKE_SOURCE_DIR}/src/persistent_stores/store_instances
         ${CMAKE_SOURCE_DIR}/tests/stubs
         ${CMAKE_SOURCE_DIR}/lib/Marlin/Marlin/src/
         ${CMAKE_SOURCE_DIR}/lib/Marlin/
  )

# define custom macros
target_compile_definitions(language_eeprom_tests PUBLIC LANGEEPROM_UNITTEST)

target_link_libraries(language_eeprom_tests SG14 freertos_tests)

# tell build system about the test case
add_catch_test(language_eeprom_tests)

add_dependencies(language_eeprom_tests generate_journal_hashes_tests)
