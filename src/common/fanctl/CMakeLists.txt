if(BOARD MATCHES "XLBUDDY")
  target_sources(
    firmware PRIVATE CFanCtlCommon.cpp on_puppy/CFanCtlOnPuppy.cpp on_puppy/fanctl_instances.cpp
    )
elseif(NOT BOARD MATCHES "MODULARBED")
  target_sources(
    firmware PRIVATE CFanCtlCommon.cpp local/CFanCtlLocal.cpp local/fanctl_instances.cpp
    )
endif()

target_include_directories(BuddyHeaders INTERFACE .)
