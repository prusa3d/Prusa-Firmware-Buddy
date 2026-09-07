add_library(i2c_base INTERFACE)
target_include_directories(i2c_base INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(i2c_base INTERFACE byte_utils)
