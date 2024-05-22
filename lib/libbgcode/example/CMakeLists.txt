cmake_minimum_required(VERSION 3.12)
project(bgcode_client)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option (BGCODE_AS_SUBPROJECT "Import bgcode library with add_subdirectory, instead of find_package" OFF)

if (BGCODE_AS_SUBPROJECT)
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/upstream)
else ()
    find_package(LibBGCode REQUIRED)
endif ()

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} LibBGCode::bgcode_convert)

