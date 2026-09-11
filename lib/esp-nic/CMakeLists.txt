# The following five lines of boilerplate have to be in your project's CMakeLists in this exact
# order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# cmake-format: off
# SDKs throw some warnings we cant fix - just suppress them
set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)
set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS ON CACHE INTERNAL "")
file(GLOB main_c_sources "${CMAKE_CURRENT_LIST_DIR}/main/*.c")
idf_build_set_property(C_COMPILE_OPTIONS "-w" APPEND)
if(main_c_sources)
    message(FATAL_ERROR "C warnings are disabled to suppress SDKs")
endif()
# cmake-format: on

project(uart_wifi)
