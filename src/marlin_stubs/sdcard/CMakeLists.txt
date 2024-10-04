if(NOT BOARD STREQUAL "XL_DEV_KIT_XLB")
  target_sources(firmware PRIVATE M20-M30_M32-M34.cpp)
endif()
