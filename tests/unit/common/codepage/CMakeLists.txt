add_executable(
  common_codepage_tests ${CMAKE_CURRENT_SOURCE_DIR}/437.cpp
                        ${CMAKE_SOURCE_DIR}/src/common/codepage/437.cpp
  )

target_include_directories(common_codepage_tests PRIVATE . ${CMAKE_SOURCE_DIR}/src/common)

add_catch_test(common_codepage_tests)
