add_library(spi_base INTERFACE)
target_include_directories(spi_base INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(spi_base INTERFACE byte_utils)
