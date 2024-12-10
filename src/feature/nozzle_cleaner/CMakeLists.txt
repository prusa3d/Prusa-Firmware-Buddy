target_include_directories(firmware PRIVATE . include)

target_sources(firmware PRIVATE nozzle_cleaner.cpp)
