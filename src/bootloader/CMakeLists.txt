target_sources(firmware PRIVATE bootloader.cpp)

if(BOOTLOADER_UPDATE)
  target_sources(firmware PRIVATE bootloader_update.cpp)
endif()
