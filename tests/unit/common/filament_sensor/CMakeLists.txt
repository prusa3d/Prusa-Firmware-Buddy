add_executable(
  filament_sensor_adc_tests
  ${CMAKE_SOURCE_DIR}/src/feature/filament_sensor/filament_sensor_adc_eval.cpp
  filament_sensor_adc_tests.cpp
  )
target_include_directories(
  filament_sensor_adc_tests PUBLIC ${CMAKE_SOURCE_DIR}/tests/stubs ${CMAKE_SOURCE_DIR}/src
  )
target_link_libraries(filament_sensor_adc_tests buddy_utils)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/2023-04-05_16-14.csv DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_catch_test(filament_sensor_adc_tests)

add_executable(
  filament_sensor_event_tests
  ${CMAKE_SOURCE_DIR}/src/feature/filament_sensor/filament_sensor.cpp
  ${CMAKE_SOURCE_DIR}/src/feature/filament_sensor/calibrator/filament_sensor_calibrator_basic.cpp
  filament_sensor_event_tests.cpp
  )
target_include_directories(
  filament_sensor_event_tests PRIVATE stubs ${CMAKE_SOURCE_DIR}/src
                                      ${CMAKE_SOURCE_DIR}/src/logging/include
  )
target_link_libraries(filament_sensor_event_tests buddy_utils)
add_catch_test(filament_sensor_event_tests)
