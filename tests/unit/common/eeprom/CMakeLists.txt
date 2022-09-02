add_executable(
  eeprom_unit_tests
  ${CMAKE_CURRENT_SOURCE_DIR}/eeprom_test.cpp ${CMAKE_CURRENT_SOURCE_DIR}/dummy_eeprom_chip.cpp
  ${CMAKE_SOURCE_DIR}/src/common/crc32.c ${CMAKE_SOURCE_DIR}/src/common/configuration_store.cpp
  )
# define custom macros
target_compile_definitions(eeprom_unit_tests PUBLIC EEPROM_UNITTEST)
target_include_directories(eeprom_unit_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/common)
add_catch_test(eeprom_unit_tests)
