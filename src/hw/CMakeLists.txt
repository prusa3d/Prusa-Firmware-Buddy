target_sources(firmware PRIVATE cpu_utils.cpp)

if(HAS_XLCD_TOUCH_DRIVER)
  add_subdirectory(touchscreen)
endif()

if(BOARD_IS_MASTER_BOARD)
  target_sources(firmware PRIVATE buffered_serial.cpp)
endif()

if((BOARD STREQUAL "XBUDDY")
   OR BOARD STREQUAL "XLBUDDY"
   OR BOARD STREQUAL "XL_DEV_KIT_XLB"
   )
  target_sources(firmware PRIVATE FUSB302B.cpp)
  add_subdirectory(xbuddy_xlbuddy)
  if(BOOTLOADER STREQUAL "NO")
    target_sources(firmware PRIVATE at21csxx.cpp)
  endif()
endif()

if(HAS_ACCELEROMETER)
  if(BOARD STREQUAL "DWARF")
    target_link_libraries(firmware PRIVATE LIS2DH12)
  endif()
endif()

if(BOARD_IS_MASTER_BOARD)
  if((PRINTER STREQUAL "MK4")
     OR (PRINTER STREQUAL "iX")
     OR (PRINTER STREQUAL "COREONE")
     )
    add_subdirectory(mk4_ix)
  endif()

  if(PRINTER STREQUAL "MK3.5")
    add_subdirectory(mk3.5)
  endif()

  if(PRINTER STREQUAL "XL")
    add_subdirectory(xl)
  endif()

  if(PRINTER STREQUAL "XL_DEV_KIT")
    add_subdirectory(xl_dev_kit)
  endif()
endif()

if(HAS_I2C_EXPANDER)
  target_sources(firmware PRIVATE TCA6408A.cpp)
endif()
