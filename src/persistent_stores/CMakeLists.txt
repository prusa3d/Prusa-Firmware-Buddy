target_include_directories(firmware PRIVATE .)

if(${CMAKE_HOST_SYSTEM_NAME} MATCHES "Windows" AND ${CMAKE_CXX_COMPILER_VERSION} EQUAL "10.3.1")
  target_compile_definitions(firmware PRIVATE EEPROM_JOURNAL_SKIP_DUPLICITY_CHECK=1)
  message(NOTICE "Skipping EEPROM journal duplicity check, because it would fail on 32bit gcc")
endif()

add_subdirectory(store_instances)
add_subdirectory(journal)
add_subdirectory(storage_drivers)
