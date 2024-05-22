target_sources(firmware PRIVATE M115.cpp M604.cpp)

if(NOT BOARD MATCHES "XL_DEV_KIT_XLB")
  target_sources(firmware PRIVATE M46.cpp)
endif()
