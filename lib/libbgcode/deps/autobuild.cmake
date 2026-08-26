if (NOT ${PROJECT_NAME}_DEPS_PRESET)
    set (${PROJECT_NAME}_DEPS_PRESET "default")
endif ()

set (_output_quiet "")
if (${PROJECT_NAME}_DEPS_OUTPUT_QUIET)
    set (_output_quiet OUTPUT_QUIET)
endif ()

message(STATUS "Building the dependencies with preset ${${PROJECT_NAME}_DEPS_PRESET}")

set(_gen_arg "")
if (CMAKE_GENERATOR)
    set (_gen_arg "-G${CMAKE_GENERATOR}")
endif ()

set(_build_args "")

if (CMAKE_C_COMPILER)
    list(APPEND _build_args "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}")
endif ()

if (CMAKE_CXX_COMPILER)
    list(APPEND _build_args "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}")
endif ()

if (CMAKE_TOOLCHAIN_FILE)
    list(APPEND _build_args "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}")
endif ()

if (${PROJECT_NAME}_DEPS_SHARED) # if deps needs to be built as shared libs
    list(APPEND _build_args "-DBUILD_SHARED_LIBS:BOOL=ON")
endif ()

if (BUILD_SHARED_LIBS) # forward if PIC is needed (static deps but project is shared library)
    list(APPEND _build_args "-DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON")
endif ()

set(_build_dir "${CMAKE_CURRENT_LIST_DIR}/build-${${PROJECT_NAME}_DEPS_PRESET}")
if (${PROJECT_NAME}_DEPS_BUILD_DIR)
    set(_build_dir "${${PROJECT_NAME}_DEPS_BUILD_DIR}")
endif ()

message(STATUS "build dir = ${_build_dir}")

set(_preset_arg "--preset ${${PROJECT_NAME}_DEPS_PRESET}")
if (CMAKE_VERSION VERSION_LESS 3.19)
    message(WARNING "CMake presets are not supported with this version of CMake. Building all dependency packages!")
    set(_preset_arg "." )
    list(APPEND _build_args "-DLibBGCode_Deps_BUILD_ALL:BOOL=ON")
endif ()

execute_process(
    COMMAND ${CMAKE_COMMAND} "${_preset_arg}" "${_gen_arg}" -B ${_build_dir} ${_build_args}
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    ${_output_quiet}
    ERROR_VARIABLE _deps_configure_output
    RESULT_VARIABLE _deps_configure_result
)

if (NOT _deps_configure_result EQUAL 0)
    message(FATAL_ERROR "Dependency configure failed with output:\n${_deps_configure_output}")
else ()
    execute_process(
        COMMAND ${CMAKE_COMMAND} --build .
        WORKING_DIRECTORY ${_build_dir}
        ${_output_quiet}
        ERROR_VARIABLE _deps_build_output
        RESULT_VARIABLE _deps_build_result
    )
    if (NOT _deps_build_result EQUAL 0)
        message(FATAL_ERROR "Dependency build failed with output:\n${_deps_build_output}")
    endif ()
endif ()

if (${PROJECT_NAME}_DEPS_PRESET STREQUAL "wasm")
    list(APPEND CMAKE_FIND_ROOT_PATH ${_build_dir}/destdir/usr/local)
    set(CMAKE_FIND_ROOT_PATH "${CMAKE_FIND_ROOT_PATH}" CACHE STRING "" FORCE)
else ()
    list(APPEND CMAKE_PREFIX_PATH ${_build_dir}/destdir/usr/local)
    set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}" CACHE STRING "" FORCE)
endif ()
