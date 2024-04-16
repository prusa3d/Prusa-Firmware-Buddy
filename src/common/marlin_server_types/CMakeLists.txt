add_library(marlin_server_types INTERFACE)
target_include_directories(marlin_server_types INTERFACE .)

if(NOT CMAKE_CROSSCOMPILING)
  add_subdirectory(python_binding)

endif()
