add_library(ADS131M02 INTERFACE)
target_include_directories(ADS131M02 INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(ADS131M02 INTERFACE spi_base byte_utils)
