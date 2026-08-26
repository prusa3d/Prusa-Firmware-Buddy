cmake_minimum_required(VERSION 3.12)

include(cmake/ProjectVersion.cmake)

project(LibBGCode VERSION ${LibBGCode_VERSION})

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(_libname ${PROJECT_NAME})
string(TOLOWER ${_libname} _libname)
string(REPLACE "lib" "" _libname ${_libname})

option(${PROJECT_NAME}_BUILD_TESTS "Build unit tests" ON)
option(${PROJECT_NAME}_BUILD_COMPONENT_Binarize "Include Binarize component in the library" ON)
option(${PROJECT_NAME}_BUILD_SANITIZERS "Turn on sanitizers" OFF)

# Dependency build management
option(${PROJECT_NAME}_BUILD_DEPS "Build dependencies before the project" OFF)
option(${PROJECT_NAME}_DEPS_OUTPUT_QUIET "Don't print build output for dependencies" ON)
option(${PROJECT_NAME}_DEPS_SHARED "Build dependencies as shared libraries" OFF)
set(${PROJECT_NAME}_DEPS_PRESET "default" CACHE STRING "Preset of the dependencies when ${PROJECT_NAME}_BUILD_DEPS is ON")
set(${PROJECT_NAME}_DEPS_BUILD_DIR "" CACHE PATH "Binary dir of the dependencies build when ${PROJECT_NAME}_BUILD_DEPS is ON")

if (${PROJECT_NAME}_BUILD_DEPS)
    include(deps/autobuild.cmake)
endif ()

if (NOT CMAKE_DEBUG_POSTFIX)
    set(CMAKE_DEBUG_POSTFIX "d")
endif ()

include(CMakeDependentOption)
cmake_dependent_option(${PROJECT_NAME}_BUILD_COMPONENT_Convert "Include Convert component in the library" ON 
                       "${PROJECT_NAME}_BUILD_COMPONENT_Binarize" OFF)
cmake_dependent_option(${PROJECT_NAME}_BUILD_CMD_TOOL "Include bgcode command line tool in the library" ON 
                       "${PROJECT_NAME}_BUILD_COMPONENT_Convert" OFF)

if (EMSCRIPTEN)
    cmake_dependent_option(${PROJECT_NAME}_BUILD_WASM "Include bgcode wasm module in the build" ON
                          "${PROJECT_NAME}_BUILD_COMPONENT_Convert" OFF)
endif ()

set(_selected_components "Core")
set(_selected_libs "${_libname}_core")
set(_highest_comp "Core")

# Set this if downstream would need to link additional boost
# components
set(Boost_DOWNSTREAM_COMPONENTS "")

set(namespace "${PROJECT_NAME}::")

# Create an export header
include(GenerateExportHeader)

set(_srcloc src/${PROJECT_NAME})

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.rc.in
  ${PROJECT_BINARY_DIR}/version.rc
  @ONLY)

if (CMAKE_COMPILER_IS_GNUCXX)
    add_compile_options(-fvisibility=hidden)
endif ()

add_subdirectory(${_srcloc}/core)

if (${PROJECT_NAME}_BUILD_COMPONENT_Binarize)
    add_subdirectory(${_srcloc}/binarize)

    list(APPEND _selected_components Binarize)
    list(APPEND _selected_libs ${_libname}_binarize)
    set(_highest_comp "Binarize")
endif ()

if (${PROJECT_NAME}_BUILD_COMPONENT_Convert)
    add_subdirectory(${_srcloc}/convert)

    list(APPEND _selected_components Convert)
    list(APPEND _selected_libs ${_libname}_convert)
    set(_highest_comp "Convert")
endif ()

add_library(${_libname} INTERFACE)
target_link_libraries(${_libname} INTERFACE ${_selected_libs})

if (${PROJECT_NAME}_BUILD_CMD_TOOL)
    add_subdirectory(${_srcloc}/cmd)
endif ()

if (${PROJECT_NAME}_BUILD_WASM)
    add_subdirectory(${_srcloc}/wasm)
endif()

if (${PROJECT_NAME}_BUILD_PYTHON_BINDING)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif ()

if(${PROJECT_NAME}_BUILD_TESTS)
    enable_testing ()
    add_subdirectory(tests)
endif()

# Create and install the CMake config script
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)
set(CONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")

set(version_config "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake")
set(project_config "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake")

install(TARGETS ${_libname}
    EXPORT ${PROJECT_NAME}{_highest_comp}Targets
    INCLUDES DESTINATION include/${PROJECT_NAME}
)

foreach(_comp ${_selected_components})
    # Install targets and headers
    string(TOLOWER ${_comp} _comp_lower)
    set(_export_targets ${_libname}_${_comp_lower})

    set_target_properties(${_libname}_${_comp_lower}
        PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION})

    install(TARGETS ${_export_targets}
        EXPORT ${PROJECT_NAME}${_comp}Targets
        INCLUDES DESTINATION include/${PROJECT_NAME}
    )

    install(FILES ${_srcloc}/${_comp_lower}/${_comp_lower}.hpp DESTINATION include/${PROJECT_NAME}/${_comp_lower})
    install(FILES
        ${PROJECT_BINARY_DIR}/${_comp_lower}/export.h DESTINATION include/${PROJECT_NAME}/${_comp_lower}/
    )
    install(
        EXPORT "${PROJECT_NAME}${_comp}Targets"
        NAMESPACE "${namespace}"
        DESTINATION "${CONFIG_INSTALL_DIR}"
    )
endforeach()

write_basic_package_version_file(
    "${version_config}"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "cmake/Config.cmake.in"
    "${project_config}"
    INSTALL_DESTINATION "${CONFIG_INSTALL_DIR}"
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
)

install(
    FILES "${project_config}" "${version_config}"
    DESTINATION "${CONFIG_INSTALL_DIR}"
)
