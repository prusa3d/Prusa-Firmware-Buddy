add_library(crc INTERFACE)

target_include_directories(crc INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(crc INTERFACE byte_utils)
