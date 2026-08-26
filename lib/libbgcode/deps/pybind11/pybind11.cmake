set(_exclude_from_all OFF)

if (NOT ${PROJECT_NAME}_BUILD_pybind11 AND NOT ${PROJECT_NAME}_BUILD_ALL)
  set(_exclude_from_all ON)
endif ()

add_cmake_project(pybind11
    EXCLUDE_FROM_ALL ${_exclude_from_all}
    URL https://github.com/pybind/pybind11/archive/refs/tags/v2.11.1.zip
    URL_HASH SHA256=b011a730c8845bfc265f0f81ee4e5e9e1d354df390836d2a25880e123d021f89
    CMAKE_ARGS
        -DPYBIND11_TEST:BOOL=OFF
)
