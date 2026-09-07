target_sources(
  firmware PRIVATE store_c_api.cpp store_definition.cpp store_instance.cpp store_journal.cpp
  )

if(NOT HAS_CONFIG_STORE_WO_BACKEND)
  target_sources(firmware PRIVATE filament_variant_decode.cpp migrations.cpp)

endif()
