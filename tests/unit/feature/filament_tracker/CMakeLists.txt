# filament_tracker_tests
add_executable(
  filament_tracker_tests ${CMAKE_SOURCE_DIR}/src/feature/filament_tracker/filament_tracker.cpp
                         filament_tracker_tests.cpp
  )
target_include_directories(
  filament_tracker_tests PRIVATE stub ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/include
  )
target_link_libraries(filament_tracker_tests buddy_utils freertos-mock random)
add_catch_test(filament_tracker_tests)
