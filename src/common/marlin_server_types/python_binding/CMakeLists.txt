cmake_minimum_required(VERSION 3.27) # needed for $<COMPILE_ONLY:xx> generator expression

# Turn RTTI on for this module (disabled for firmware, on for python binding)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-frtti>)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE
      Release
      CACHE STRING "Choose the type of build." FORCE
      )
  set_property(
    CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo"
    )
endif()

# find the nanobind package
find_package(
  Python
  COMPONENTS Interpreter Development.Module
  REQUIRED
  )

# Detect the installed nanobind package and import it into CMake
execute_process(
  COMMAND "${Python3_EXECUTABLE}" -m nanobind --cmake_dir
  OUTPUT_STRIP_TRAILING_WHITESPACE
  OUTPUT_VARIABLE NB_DIR
  )
list(APPEND CMAKE_PREFIX_PATH "${NB_DIR}")
find_package(nanobind CONFIG REQUIRED)

message(STATUS "Using Python_INCLUDE_DIRS: ${Python_INCLUDE_DIRS}, nanobind dir: ${NB_DIR}")

nanobind_build_library(nanobind-static)

# nanobind_add_module(marlin_server_types_python_module_impl fsm_python_binding.cpp)
add_library(marlin_server_types_python_module_impl MODULE fsm_python_binding.cpp)

target_include_directories(
  marlin_server_types_python_module_impl PRIVATE ${OPTIONS_INCLUDE_DIR} ${NB_DIR}/include
                                                 ${Python_INCLUDE_DIRS}
  )
target_link_libraries(marlin_server_types_python_module_impl PUBLIC marlin_server_types)
target_link_libraries(
  marlin_server_types_python_module_impl PUBLIC $<COMPILE_ONLY:BuddyHeaders> $<COMPILE_ONLY:Marlin>
                                                $<COMPILE_ONLY:firmware> magic_enum nanobind-static
  )

set_target_properties(marlin_server_types_python_module_impl PROPERTIES LINKER_LANGUAGE CXX)
nanobind_link_options(marlin_server_types_python_module_impl)
nanobind_extension(marlin_server_types_python_module_impl)
nanobind_set_visibility(marlin_server_types_python_module_impl)

add_custom_target(marlin_server_types_python_module)
add_dependencies(marlin_server_types_python_module marlin_server_types_python_module_impl)

# Install directive for scikit-build-core, will populate python module "marlin_server_types" with
# marlin_server_types_python_module_impl shared library
install(TARGETS marlin_server_types_python_module_impl LIBRARY DESTINATION marlin_server_types)
