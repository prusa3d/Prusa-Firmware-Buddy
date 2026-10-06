add_library(log_interface INTERFACE)
target_include_directories(log_interface INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_library(logging OBJECT log.cpp log_dest_shared.cpp log_platform.cpp)

# Only these boards register the RTT sink
if(BOARD_IS_MASTER_BOARD OR BOARD STREQUAL "DWARF")
  target_sources(logging PRIVATE log_dest_rtt.cpp)
  target_link_libraries(logging PRIVATE Segger)
endif()

if(BOARD IN_LIST BUDDY_BOARDS)
  target_sources(
    logging PRIVATE log_buddy.cpp log_dest_file.cpp log_dest_syslog.cpp log_dest_usb.cpp
                    log_task.cpp
    )
  target_link_libraries(
    logging
    PRIVATE LwIP # log_dest_syslog.cpp
            tinyusb::tinyusb # log_dest_usb.cpp
            Arduino::Core # config store, via log_dest_syslog.cpp
            error_codes # config store, via log_dest_syslog.cpp
    )
else()
  target_sources(logging PRIVATE log_puppy.cpp)
endif()

if(BOARD STREQUAL "DWARF" OR BOARD STREQUAL "XL_DEV_KIT_XLB")
  target_sources(logging PRIVATE log_dest_bufflog.cpp)
endif()

# FIXME: One day, there will be proper modules instead of this
target_link_libraries(
  logging
  PRIVATE BuddyHeaders # pretty much everything
          FreeRTOS::FreeRTOS # pretty much everything
          freertos
          bsod
          SG14 # config store 🤦
          buddy_utils # config store 🤦
          uuid # config store 🤦
          otp
          raii
  PUBLIC log_interface
  )

target_include_directories(
  logging
  PRIVATE ${CMAKE_SOURCE_DIR}/include
          ${CMAKE_SOURCE_DIR}/lib
          ${CMAKE_SOURCE_DIR}/lib/Marlin/Marlin/src # config store 🤦
          ${CMAKE_SOURCE_DIR}/lib/Marlin # config store 🤦
          ${CMAKE_SOURCE_DIR}/src
          ${CMAKE_SOURCE_DIR}/src/gui # filename_defs.h 🤦
  )
