add_library(modbus STATIC src/modbus/modbus.cpp src/modbus/server_address.cpp src/modbus/traits.cpp)
target_include_directories(modbus PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(modbus PUBLIC crc bsod byte_utils)
