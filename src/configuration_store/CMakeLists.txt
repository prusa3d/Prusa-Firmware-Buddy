target_sources(firmware PRIVATE configuration_store.cpp)
add_subdirectory(journal)
target_include_directories(firmware PRIVATE .)
