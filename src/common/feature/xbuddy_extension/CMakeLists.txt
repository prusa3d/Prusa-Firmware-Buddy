target_link_libraries(firmware PUBLIC XBuddyExtensionShared)

target_sources(firmware PRIVATE cooling.cpp xbuddy_extension.cpp)
