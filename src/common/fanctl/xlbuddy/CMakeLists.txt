target_sources(firmware PRIVATE CFanCtlPuppy.cpp cpu_fan_controller.cpp fanctl_instances.cpp)

target_include_directories(BuddyHeaders INTERFACE .)
