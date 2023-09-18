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
          transfer_recovery.cpp
  )
if(USE_ASYNCIO)
  target_sources(firmware PRIVATE async_io.cpp)
  target_compile_definitions(firmware INTERFACE USE_ASYNCIO)
endif()
