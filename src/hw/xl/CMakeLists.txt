target_include_directories(firmware PRIVATE . include)

target_sources(firmware PRIVATE hw_configuration.cpp modular_bed_fan.cpp xl_enclosure.cpp)
