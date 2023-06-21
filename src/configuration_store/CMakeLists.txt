target_sources(firmware PRIVATE configuration_store.cpp eeprom_journal_config_store.cpp)
add_subdirectory(journal)
target_include_directories(firmware PRIVATE .)
target_precompile_headers(firmware PRIVATE $<$<COMPILE_LANGUAGE:CXX>:configuration_store.hpp>)
