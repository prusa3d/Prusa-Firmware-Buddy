include(ConfigurationStoreUtils.cmake)
generate_config_store_structure(firmware "${CMAKE_CURRENT_SOURCE_DIR}/store_structure.json")

if(BOARD MATCHES ".*BUDDY")
  target_sources(
    firmware PRIVATE config_store_c_api.cpp config_store_c_api.cpp configuration_store.cpp
                     eeprom_access.cpp migration_functions.cpp SteelSheets.cpp
    )
endif()

target_include_directories(firmware PRIVATE . include)
