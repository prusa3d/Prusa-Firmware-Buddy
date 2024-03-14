find_package(Catch2 2.9 REQUIRED)

include(Catch)

find_package(Boost ${Boost_VER} REQUIRED COMPONENTS nowide)

if (MSVC)
    set_target_properties(Boost::nowide PROPERTIES MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release)
endif ()

set(CATCH_EXTRA_ARGS "" CACHE STRING "Extra arguments for catch2 test suites.")

set(TEST_DATA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/data)
file(TO_NATIVE_PATH "${TEST_DATA_DIR}" TEST_DATA_DIR)

add_library(test_common INTERFACE)
target_include_directories(test_common INTERFACE ${CMAKE_CURRENT_LIST_DIR})
target_compile_definitions(test_common INTERFACE TEST_DATA_DIR=R"\(${TEST_DATA_DIR}\)" CATCH_CONFIG_FAST_COMPILE)
target_link_libraries(test_common INTERFACE Catch2::Catch2 Boost::nowide)

add_subdirectory(core)

if (${PROJECT_NAME}_BUILD_COMPONENT_Binarize)
    add_subdirectory(binarize)
endif()

if (${PROJECT_NAME}_BUILD_COMPONENT_Convert)
    add_subdirectory(convert)
endif ()
