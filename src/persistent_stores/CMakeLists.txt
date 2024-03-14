target_include_directories(firmware PRIVATE .)

add_subdirectory(store_instances)
add_subdirectory(journal)
add_subdirectory(storage_drivers)

include(GenerateJournalHashes.cmake)

create_generate_hashes_target(
  TARGET_NAME
  generate_journal_hashes_firmware
  OUTPUT_FILE_PATH
  "${CMAKE_BINARY_DIR}/include/persistent_stores/journal/gen_journal_hashes.hpp"
  GENERATOR_SCRIPT_PATH
  "${CMAKE_SOURCE_DIR}/utils/persistent_stores/journal_hashes_generator.py"
  INPUT_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/store_instances/config_store/store_definition.hpp"
  )

add_dependencies(firmware generate_journal_hashes_firmware)
add_dependencies(BuddyHeaders generate_journal_hashes_firmware)
