target_sources(firmware PRIVATE configuration.cpp)
target_include_directories(firmware PRIVATE . include)
target_include_directories(BuddyHeaders INTERFACE .)
