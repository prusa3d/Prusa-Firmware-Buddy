include(ConfigurationStoreUtils.cmake)
if(BOARD MATCHES ".*BUDDY")
  target_sources(firmware PRIVATE eeprom_access.cpp)
endif()

generate_config_store_structure(firmware "${CMAKE_CURRENT_SOURCE_DIR}/store_structure.json")

target_include_directories(firmware PRIVATE . include)
