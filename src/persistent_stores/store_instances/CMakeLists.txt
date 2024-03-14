target_include_directories(firmware PRIVATE .)

add_subdirectory(config_store)

target_precompile_headers(
  firmware PRIVATE $<$<COMPILE_LANGUAGE:CXX>:config_store/store_instance.hpp>
  )
