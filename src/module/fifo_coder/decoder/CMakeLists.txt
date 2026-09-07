add_library(fifo_decoder STATIC src/fifo_decoder.cpp)
target_include_directories(fifo_decoder PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(fifo_decoder PUBLIC fifo_coder_base bsod)
