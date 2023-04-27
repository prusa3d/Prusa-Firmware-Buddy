add_compile_definitions(UNITTEST)
add_executable(
  Marlin_MMU2_tests
  ${CMAKE_CURRENT_SOURCE_DIR}/mmu2_test.cpp
  ${CMAKE_SOURCE_DIR}/lib/Marlin/Marlin/src/feature/prusa/MMU2/protocol_logic.cpp
  ${CMAKE_SOURCE_DIR}/lib/Prusa-Firmware-MMU/src/modules/protocol.cpp
  )
target_include_directories(
  Marlin_MMU2_tests PUBLIC . ${CMAKE_SOURCE_DIR}/lib/Marlin/Marlin/src/feature/prusa/MMU2
  )

add_catch_test(Marlin_MMU2_tests)
