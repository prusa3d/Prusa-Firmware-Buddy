include(${CMAKE_SOURCE_DIR}/src/configuration_store/ConfigurationStoreUtils.cmake)
add_executable(
  eeprom_unit_tests
  ${CMAKE_CURRENT_SOURCE_DIR}/eeprom_test.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/dummy_eeprom_chip.cpp
  ${CMAKE_SOURCE_DIR}/src/common/crc32.c
  ${CMAKE_SOURCE_DIR}/src/logging/log.c
  ../logging/utils.cpp
  ${CMAKE_SOURCE_DIR}/src/configuration_store/configuration_store.cpp
  ${CMAKE_SOURCE_DIR}/src/configuration_store/eeprom_access.cpp
  set_tests.cpp
  bsod.cpp
  ../mock/timing_dummy.cpp
  ../mock/freertos_mutex.cpp
  )

# define custom macros
target_compile_definitions(eeprom_unit_tests PUBLIC EEPROM_UNITTEST)
target_include_directories(
  eeprom_unit_tests
  PUBLIC ${CMAKE_SOURCE_DIR}/src/configuration_store eeprom_unit_tests
  PUBLIC ${CMAKE_SOURCE_DIR}/src
  )
target_include_directories(eeprom_unit_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/common)
add_catch_test(eeprom_unit_tests)

target_include_directories(eeprom_unit_tests PUBLIC . ${CMAKE_SOURCE_DIR}/src/logging)
generate_config_store_structure(
  eeprom_unit_tests "${CMAKE_CURRENT_SOURCE_DIR}/store_structure.json"
  )
