add_executable(
  eeprom_unit_tests
  ${CMAKE_CURRENT_SOURCE_DIR}/EEPROM_journal_test.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/dummy_eeprom_chip.cpp
  ${CMAKE_SOURCE_DIR}/src/configuration_store/journal/backend.cpp
  ../mock/bsod.cpp
  ../mock/timing_dummy.cpp
  ${CMAKE_SOURCE_DIR}/tests/stubs/fake_freertos_mutex.cpp
  ../mock/crc32_sw.cpp
  )

target_compile_definitions(eeprom_unit_tests PUBLIC EEPROM_UNITTEST)
target_include_directories(
  eeprom_unit_tests
  PUBLIC ${CMAKE_SOURCE_DIR}/src/configuration_store
  PUBLIC ${CMAKE_SOURCE_DIR}/src
  PUBLIC ${CMAKE_SOURCE_DIR}/include
  PUBLIC ${CMAKE_SOURCE_DIR}/tests/stubs
  PUBLIC . ${CMAKE_SOURCE_DIR}/src/logging
  PUBLIC ${CMAKE_SOURCE_DIR}/src/common
  PUBLIC ${CMAKE_SOURCE_DIR}/lib/cthash/include
  )

add_catch_test(eeprom_unit_tests)
