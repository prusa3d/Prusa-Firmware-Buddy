add_library(AIS2IH INTERFACE)
target_include_directories(AIS2IH INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(AIS2IH INTERFACE spi_base byte_utils)
