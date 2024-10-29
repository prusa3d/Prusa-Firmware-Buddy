add_executable(
  eeprom_unit_tests
  ${CMAKE_CURRENT_SOURCE_DIR}/EEPROM_journal_test.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/dummy_eeprom_chip.cpp
  ${CMAKE_SOURCE_DIR}/src/common/random_sw.cpp
  ${CMAKE_SOURCE_DIR}/src/persistent_stores/journal/backend.cpp
  ${CMAKE_SOURCE_DIR}/src/persistent_stores/storage_drivers/eeprom_storage.cpp
  ${CMAKE_SOURCE_DIR}/src/common/str_utils.cpp
  ../mock/bsod.cpp
  ../mock/timing_dummy.cpp
  ../mock/crc32_sw.cpp
  )

target_compile_definitions(eeprom_unit_tests PUBLIC EEPROM_UNITTEST)
target_include_directories(
  eeprom_unit_tests
  PUBLIC .
         ${CMAKE_SOURCE_DIR}/src/persistent_stores
         ${CMAKE_SOURCE_DIR}/src/persistent_stores/store_instances
         ${CMAKE_SOURCE_DIR}/src
         ${CMAKE_SOURCE_DIR}/src/lang
         ${CMAKE_SOURCE_DIR}/src/guiconfig
         ${CMAKE_BINARY_DIR}/tests/unit
         ${CMAKE_SOURCE_DIR}/include
         ${CMAKE_BINARY_DIR}/include
         ${CMAKE_SOURCE_DIR}/tests/stubs
         ${CMAKE_SOURCE_DIR}/src/logging
         ${CMAKE_SOURCE_DIR}/src/common
         ${CMAKE_SOURCE_DIR}/src/common/utils
         ${CMAKE_SOURCE_DIR}/lib/Marlin/Marlin/src/
  )

target_link_libraries(eeprom_unit_tests SG14 freertos_tests)

add_catch_test(eeprom_unit_tests)

include(${CMAKE_SOURCE_DIR}/src/persistent_stores/GenerateJournalHashes.cmake)

create_generate_hashes_target(
  TARGET_NAME
  generate_journal_hashes_tests
  OUTPUT_FILE_PATH
  "${CMAKE_CURRENT_BINARY_DIR}/journal/gen_journal_hashes.hpp"
  GENERATOR_SCRIPT_PATH
  "${CMAKE_SOURCE_DIR}/utils/persistent_stores/journal_hashes_generator.py"
  INPUT_FILES
  "${CMAKE_SOURCE_DIR}/src/persistent_stores/store_instances/config_store/store_definition.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/EEPROM_journal_test.cpp"
  )

add_dependencies(eeprom_unit_tests generate_journal_hashes_tests)
