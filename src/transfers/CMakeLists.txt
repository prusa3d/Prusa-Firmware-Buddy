target_sources(
  firmware
  PRIVATE changed_path.cpp
          decrypt.cpp
          download.cpp
          files.cpp
          logging.cpp
          monitor.cpp
          partial_file.cpp
          transfer.cpp
          transfer_file_check.cpp
          transfer_recovery.cpp
  )
