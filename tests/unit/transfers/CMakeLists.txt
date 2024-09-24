add_executable(
  transfers_tests
  ${CMAKE_SOURCE_DIR}/src/common/crc32.cpp
  ${CMAKE_SOURCE_DIR}/src/common/filename_type.cpp
  ${CMAKE_SOURCE_DIR}/src/common/filepath_operation.cpp
  ${CMAKE_SOURCE_DIR}/src/common/random_sw.cpp
  ${CMAKE_SOURCE_DIR}/src/transfers/decrypt.cpp
  ${CMAKE_SOURCE_DIR}/src/transfers/monitor.cpp
  ${CMAKE_SOURCE_DIR}/src/transfers/changed_path.cpp
  ${CMAKE_SOURCE_DIR}/src/transfers/transfer_recovery.cpp
  ${CMAKE_SOURCE_DIR}/tests/stubs/strlcpy.c
  ${CMAKE_SOURCE_DIR}/tests/unit/mock/bsod.cpp
  decrypt.cpp
  monitor.cpp
  missing_functions.cpp
  changed_path_tests.cpp
  transfer_tests.cpp
  partial_file_mock.cpp
  ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/mbedtls/library/aes.c
  ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/mbedtls/library/aesni.c
  )
target_include_directories(
  transfers_tests
  PRIVATE .
          ${CMAKE_CURRENT_SOURCE_DIR}/mock
          ${CMAKE_SOURCE_DIR}/src
          ${CMAKE_SOURCE_DIR}/include
          ${CMAKE_SOURCE_DIR}/src/common
          ${CMAKE_SOURCE_DIR}/lib/Middlewares/Third_Party/mbedtls/include
          ${CMAKE_SOURCE_DIR}/tests/stubs
          ${CMAKE_SOURCE_DIR}/include/usb_host
  )
add_definitions(-DDATA_PATH="${CMAKE_CURRENT_SOURCE_DIR}")

file(GLOB AUTOMATA_GENERATORS ${PROJECT_ROOT_DIR}/utils/gen-automata/*.py)

target_link_libraries(transfers_tests SG14 freertos_tests)

add_catch_test(transfers_tests)
