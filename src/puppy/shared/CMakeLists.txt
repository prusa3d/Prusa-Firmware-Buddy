target_sources(firmware PRIVATE crash_catcher_integration.cpp trigger_crash_dump.cpp)

target_include_directories(firmware PUBLIC modbus .)

add_subdirectory(hal)
add_subdirectory(modbus)
add_subdirectory(startup)
