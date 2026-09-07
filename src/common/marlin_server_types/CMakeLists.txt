add_library(marlin_server_types INTERFACE)
target_include_directories(marlin_server_types INTERFACE .)
target_link_libraries(marlin_server_types INTERFACE bsod)
target_sources(marlin_server_types PUBLIC client_response.cpp warning_type.cpp)

add_subdirectory(fsm)

if(NOT CMAKE_CROSSCOMPILING AND NOT UNITTESTS_ENABLE)
  add_subdirectory(python_binding)
endif()
