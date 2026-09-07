add_library(cobs STATIC cobs.cpp)

target_include_directories(cobs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(cobs PUBLIC SG14 bsod byte_utils)
