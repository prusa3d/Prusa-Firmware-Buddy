add_executable(
  filament_sensor_adc_tests
  ${CMAKE_SOURCE_DIR}/src/common/filters/median_filter.cpp filament_sensor_adc_tests.cpp
  ${CMAKE_SOURCE_DIR}/src/common/filament_sensor_adc_eval.cpp
  )
target_include_directories(
  filament_sensor_adc_tests PUBLIC ${CMAKE_SOURCE_DIR}/tests/stubs ${CMAKE_SOURCE_DIR}/src/common
  )

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/2023-04-05_16-14.csv DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_catch_test(filament_sensor_adc_tests)
