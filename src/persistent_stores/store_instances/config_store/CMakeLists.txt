target_sources(
  firmware PRIVATE migrations.cpp store_c_api.cpp store_definition.cpp store_instance.cpp
  )

add_subdirectory(old_eeprom)
