add_compile_definitions(UNITTEST)
add_executable(
  Marlin_EStallDetector_tests
  ${CMAKE_CURRENT_SOURCE_DIR}/e-stall_detector_test.cpp
  ${CMAKE_SOURCE_DIR}/lib/Marlin/Marlin/src/feature/prusa/e-stall_detector.cpp
  )
target_include_directories(
  Marlin_EStallDetector_tests PUBLIC . ${CMAKE_SOURCE_DIR}/lib/Marlin/Marlin/src/feature/prusa
  )

add_catch_test(Marlin_EStallDetector_tests)
