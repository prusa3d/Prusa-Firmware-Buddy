# center_approx
add_executable(center_approx center_approx.cpp)

target_include_directories(
  center_approx
  PUBLIC ${CMAKE_SOURCE_DIR}/src/common/utils ${CMAKE_SOURCE_DIR}/tests/stubs
         ${CMAKE_SOURCE_DIR}/lib/Marlin/Marlin/src ${CMAKE_SOURCE_DIR}/lib/Marlin/Marlin/src/inc
  )

target_link_libraries(center_approx buddy_utils)
add_catch_test(center_approx)

# dynamic_item_mapping
add_executable(dynamic_index_mapping dynamic_index_mapping_tests.cpp)
target_include_directories(dynamic_index_mapping PUBLIC ${CMAKE_SOURCE_DIR}/src/common/utils)
add_catch_test(dynamic_index_mapping)

# printer_model_tests
add_executable(
  printer_model_tests ${CMAKE_CURRENT_SOURCE_DIR}/printer_model_tests.cpp
                      ${CMAKE_SOURCE_DIR}/src/common/printer_model.cpp
  )
target_include_directories(
  printer_model_tests PUBLIC ${CMAKE_SOURCE_DIR}/include ${CMAKE_BINARY_DIR}/include
                             ${CMAKE_SOURCE_DIR}/src
  )
add_catch_test(printer_model_tests)

# progress mapper
add_executable(progress_mapper_tests progress_mapper_tests.cpp)
target_compile_definitions(progress_mapper_tests PUBLIC NDEBUG) # Disable assert failures
target_link_libraries(progress_mapper_tests buddy_utils)
add_catch_test(progress_mapper_tests)
