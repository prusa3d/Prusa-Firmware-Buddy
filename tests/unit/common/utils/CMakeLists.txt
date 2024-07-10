# center_approx
add_executable(center_approx center_approx.cpp)

target_include_directories(
  center_approx
  PUBLIC ${CMAKE_SOURCE_DIR}/src/common/utils ${CMAKE_SOURCE_DIR}/lib/Marlin/Marlin/src/core
         ${CMAKE_SOURCE_DIR}/tests/stubs ${CMAKE_SOURCE_DIR}/lib/Marlin/Marlin/src/inc
  )

add_catch_test(center_approx)

# dynamic_item_mapping
add_executable(dynamic_index_mapping dynamic_index_mapping_tests.cpp)

target_include_directories(dynamic_index_mapping PUBLIC ${CMAKE_SOURCE_DIR}/src/common/utils)

add_catch_test(dynamic_index_mapping)
