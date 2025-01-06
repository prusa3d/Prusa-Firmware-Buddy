add_library(marlin_server_types INTERFACE)
target_include_directories(marlin_server_types INTERFACE .)
target_sources(marlin_server_types PUBLIC warning_type.cpp)

if(NOT CMAKE_CROSSCOMPILING AND NOT UNITTESTS_ENABLE)
  add_subdirectory(python_binding)
endif()
