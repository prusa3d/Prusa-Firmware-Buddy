if(BOARD STREQUAL "XLBUDDY")
  add_subdirectory(xlbuddy)
  target_sources(firmware PRIVATE CFanCtlCommon.cpp CFanCtlEnclosure.cpp)
elseif((BOARD STREQUAL "XBUDDY") AND (PRINTER STREQUAL "MK3.5"))
  target_sources(firmware PRIVATE CFanCtl3Wire.cpp CFanCtlCommon.cpp mk3.5/fanctl_instances.cpp)
elseif((BOARD STREQUAL "XBUDDY") AND (PRINTER STREQUAL "iX"))
  target_sources(firmware PRIVATE CFanCtl3Wire.cpp CFanCtlCommon.cpp ix/fanctl_instances.cpp)
elseif(BOARD STREQUAL "XBUDDY")
  target_sources(
    firmware PRIVATE CFanCtl3Wire.cpp CFanCtlCommon.cpp xbuddy/CFanCtl3WireDynamic.cpp
                     xbuddy/fanctl_instances.cpp
    )
  target_include_directories(BuddyHeaders INTERFACE xbudy)
elseif(BOARD IN_LIST BUDDY_BOARDS)
  target_sources(firmware PRIVATE buddy/fanctl_instances.cpp CFanCtl3Wire.cpp CFanCtlCommon.cpp)
elseif(BOARD STREQUAL "DWARF")
  target_sources(firmware PRIVATE CFanCtl3Wire.cpp CFanCtlCommon.cpp dwarf/fanctl_instances.cpp)
elseif(BOARD STREQUAL "XL_DEV_KIT_XLB")
  target_sources(firmware PRIVATE CFanCtlCommon.cpp)
  add_subdirectory(xlbuddy)
endif()

target_include_directories(BuddyHeaders INTERFACE .)
