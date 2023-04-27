add_executable(
  transfers_tests
  ${CMAKE_SOURCE_DIR}/src/transfers/decrypt.cpp
  ${CMAKE_SOURCE_DIR}/src/transfers/monitor.cpp
  ${CMAKE_SOURCE_DIR}/src/transfers/changed_path.cpp
  ${CMAKE_SOURCE_DIR}/tests/stubs/strlcpy.c
  ${CMAKE_SOURCE_DIR}/tests/stubs/fake_freertos_mutex.cpp
  decrypt.cpp
  monitor.cpp
  missing_functions.cpp
  changed_path_tests.cpp
  ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/mbedtls/library/aes.c
  ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/mbedtls/library/aesni.c
  )
target_include_directories(
  transfers_tests
  PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/common
          ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/mbedtls/include
          ${CMAKE_SOURCE_DIR}/tests/stubs
  )
add_definitions(-DUNITTESTS -DDATA_PATH="${CMAKE_CURRENT_SOURCE_DIR}")

file(GLOB AUTOMATA_GENERATORS ${PROJECT_ROOT_DIR}/utils/gen-automata/*.py)

add_catch_test(transfers_tests)
