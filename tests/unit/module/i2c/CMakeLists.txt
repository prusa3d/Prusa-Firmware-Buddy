add_library(i2c_mock INTERFACE)

target_include_directories(i2c_mock INTERFACE .)
target_link_libraries(i2c_mock INTERFACE i2c_base bsod byte_utils)
