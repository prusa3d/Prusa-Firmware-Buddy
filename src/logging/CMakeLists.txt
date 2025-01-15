add_library(logging OBJECT log.cpp log_dest_rtt.cpp log_dest_shared.cpp log_platform.cpp)

if(BOARD MATCHES ".*BUDDY")
  target_sources(logging PRIVATE log_buddy.cpp log_dest_syslog.cpp log_dest_usb.cpp log_task.cpp)
  target_link_libraries(
    logging PRIVATE LwIP # log_dest_syslog.cpp
                    tinyusb::tinyusb # log_dest_usb.cpp
    )
  if(HAS_FILE_LOG)
    target_sources(logging PRIVATE log_dest_file.cpp)
  endif()
endif()

if(BOARD STREQUAL "DWARF" OR BOARD STREQUAL "XL_DEV_KIT_XLB")
  target_sources(logging PRIVATE log_dest_bufflog.cpp log_puppy.cpp)
endif()

# FIXME: One day, there will be proper modules instead of this
target_link_libraries(
  logging
  PRIVATE BuddyHeaders # pretty much everything
          FreeRTOS::FreeRTOS # pretty much everything
          freertos
          Arduino::Core # config store 🤦
          SG14 # config store 🤦
          error_codes # config store 🤦
  )

target_include_directories(
  logging
  PRIVATE ${CMAKE_SOURCE_DIR}/include
          ${CMAKE_SOURCE_DIR}/lib
          ${CMAKE_SOURCE_DIR}/lib/Segger
          ${CMAKE_SOURCE_DIR}/lib/Marlin/Marlin/src # config store 🤦
          ${CMAKE_SOURCE_DIR}/lib/Marlin # config store 🤦
          ${CMAKE_SOURCE_DIR}/src
          ${CMAKE_SOURCE_DIR}/src/gui # filename_defs.h 🤦
  )

target_include_directories(logging PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
