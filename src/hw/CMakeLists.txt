target_sources(firmware PRIVATE cpu_utils.cpp)

if(BOARD MATCHES ".*BUDDY")
  target_sources(firmware PRIVATE buffered_serial.cpp)
endif()

if(BOARD STREQUAL "XBUDDY")
  target_sources(firmware PRIVATE AT21CSxx.cpp)
endif()

if((BOARD STREQUAL "XBUDDY") OR BOARD STREQUAL "XLBUDDY")
  target_sources(firmware PRIVATE FUSB302B.cpp fusb303.cpp)
endif()

if(HAS_XLCD_TOUCH_DRIVER)
  target_sources(firmware PRIVATE GT911.cpp)
endif()

if(HAS_ACCELEROMETER)
  target_sources(firmware PRIVATE SparkFunLIS2DH.cpp)
endif()

if(BOARD MATCHES ".*BUDDY")
  if((PRINTER STREQUAL "MK4") OR (PRINTER STREQUAL "iX"))
    add_subdirectory(mk4_ix)
  endif()

  if(PRINTER STREQUAL "MK3.5")
    add_subdirectory(mk3.5)
  endif()
endif()
