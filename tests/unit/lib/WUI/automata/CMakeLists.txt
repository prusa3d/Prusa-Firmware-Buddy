add_executable(
  automata_tests
  # Test files
  ${CMAKE_SOURCE_DIR}/lib/WUI/automata/core.cpp traversal.cpp # Tested files
  )
target_include_directories(automata_tests PRIVATE . ${CMAKE_SOURCE_DIR}/lib/WUI)

add_catch_test(automata_tests)
