target_sources(firmware PRIVATE store_c_api.cpp store_definition.cpp store_instance.cpp)

if(NOT HAS_CONFIG_STORE_WO_BACKEND)
  target_sources(firmware PRIVATE migrations.cpp)

  add_subdirectory(old_eeprom)
endif()
