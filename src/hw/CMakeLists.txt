target_sources(firmware PRIVATE cpu_utils.cpp)

if(BOARD MATCHES ".*BUDDY")
  target_sources(firmware PRIVATE AT21CSxx.cpp buffered_serial.cpp SparkFunLIS3DH.cpp)
endif()

if((BOARD STREQUAL "XBUDDY" AND BOARD_VERSION VERSION_GREATER_EQUAL "0.2.0") OR BOARD STREQUAL
                                                                                "XLBUDDY"
   )
  target_sources(firmware PRIVATE FUSB302B.cpp fusb303.cpp)
endif()

if(HAS_XLCD_TOUCH_DRIVER)
  target_sources(firmware PRIVATE GT911.cpp)
endif()
