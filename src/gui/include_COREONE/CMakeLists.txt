target_include_directories(firmware PRIVATE .)

if(HAS_SELFTEST)
  target_sources(firmware PRIVATE selftest_snake_config.cpp selftest_snake_submenus.cpp)
endif()
