add_library(
  nanocbor ${CMAKE_CURRENT_SOURCE_DIR}/nanocbor/src/decoder.c
           ${CMAKE_CURRENT_SOURCE_DIR}/nanocbor/src/encoder.c
  )

target_include_directories(nanocbor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/nanocbor_stub_include/)
target_include_directories(nanocbor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/nanocbor/include/)
