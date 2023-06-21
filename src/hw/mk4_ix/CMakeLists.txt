target_include_directories(firmware PRIVATE . include)

target_sources(firmware PRIVATE calibrated_loveboard.cpp hw_configuration.cpp)
