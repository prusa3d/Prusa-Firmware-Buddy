target_include_directories(firmware PRIVATE . include)
target_include_directories(BuddyHeaders INTERFACE .)

target_sources(firmware PRIVATE calibrated_loveboard.cpp hw_configuration.cpp)
