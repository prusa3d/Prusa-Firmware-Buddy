cmake_minimum_required(VERSION 3.25)
project(
  bitsery
  LANGUAGES CXX
  VERSION 5.2.4
  )

# ======== build options ===================================
option(BITSERY_BUILD_EXAMPLES "Build examples" OFF)
option(BITSERY_BUILD_TESTS "Build tests" OFF)

# ============= setup target ======================
add_library(bitsery INTERFACE)
# create alias, so that user could always write target_link_libraries(... Bitsery::bitsery) despite
# of bitsery target is imported or not
add_library(Bitsery::bitsery ALIAS bitsery)

include(GNUInstallDirs)
target_include_directories(
  bitsery INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )
target_compile_features(
  bitsery INTERFACE cxx_auto_type cxx_constexpr cxx_lambdas cxx_nullptr cxx_variadic_templates
  )

# =============== setup installation =======================
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/BitseryConfigVersion.cmake COMPATIBILITY SameMajorVersion
  )
install(
  TARGETS bitsery
  EXPORT bitseryTargets
  INCLUDES
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
install(
  EXPORT bitseryTargets
  FILE "BitseryConfig.cmake"
  NAMESPACE Bitsery::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/bitsery
  )
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/BitseryConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/bitsery
        )
install(DIRECTORY include/bitsery DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# ================ handle sub-projects =====================

if(BITSERY_BUILD_EXAMPLES)
  message("build bitsery examples")
  add_subdirectory(examples)
else()
  message("skip bitsery examples")
endif()

if(BITSERY_BUILD_TESTS)
  message("build bitsery tests")
  enable_testing()
  add_subdirectory(tests)
else()
  message("skip bitsery tests")
endif()
