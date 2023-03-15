target_include_directories(firmware PUBLIC modbus .)

add_subdirectory(hal)
add_subdirectory(modbus)
add_subdirectory(startup)
