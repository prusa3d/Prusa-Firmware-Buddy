target_sources(firmware PRIVATE configuration_store.cpp eeprom_storage.cpp)

target_include_directories(firmware PRIVATE .)

if(${CMAKE_HOST_SYSTEM_NAME} MATCHES "Windows" AND ${CMAKE_CXX_COMPILER_VERSION} EQUAL "10.3.1")
  target_compile_definitions(firmware PRIVATE EEPROM_JOURNAL_SKIP_DUPLICITY_CHECK=1)
  message(NOTICE "Skipping EEPROM journal duplicity check, because it would fail on 32bit gcc")
endif()

add_subdirectory(journal)
add_subdirectory(
  eeprom_journal
  ) # note: eeprom_journal is more of a store 'name' rather than 'peripheral' + 'storage method'
    # merged into one, ie other stores can have more creative names

target_precompile_headers(firmware PRIVATE $<$<COMPILE_LANGUAGE:CXX>:configuration_store.hpp>)
