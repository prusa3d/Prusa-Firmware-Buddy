target_sources(
  firmware
  PRIVATE changed_path.cpp
          download.cpp
          files.cpp
          logging.cpp
          monitor.cpp
          partial_file.cpp
          partial_file_posix.cpp
          partial_file_usb.cpp
          transfer.cpp
          transfer_file_check.cpp
          transfer_recovery.cpp
  )

if(NOT WEBSOCKET)
  target_sources(firmware PRIVATE decrypt.cpp)
endif()
