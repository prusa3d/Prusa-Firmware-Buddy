# Cmake for puppies (Dwarf + Modular bed)

target_sources(
  firmware
  PUBLIC fifo_decoder.cpp
         modbus.cpp
         power_panic_mutex.cpp
         puppy_crash_dump.cpp
         puppy_task.cpp
         PuppyBootstrap.cpp
         PuppyBus.cpp
         PuppyModbus.cpp
         time_sync.cpp
  )

target_link_libraries(BuddyHeaders INTERFACE lightmodbus)

if(HAS_DWARF)
  target_sources(firmware PUBLIC Dwarf.cpp)
endif()

if(HAS_MODULARBED)
  target_sources(firmware PUBLIC modular_bed.cpp)
endif()

if(HAS_PUPPIES_BOOTLOADER)
  target_sources(firmware PUBLIC BootloaderProtocol.cpp)
endif()

if(PUPPY_FLASH_FW)
  # if bootloader in puppy boards is enabled, also compile flashing support

  if(NOT HAS_PUPPIES_BOOTLOADER)
    message(FATAL_ERROR "Need HAS_PUPPIES_BOOTLOADER when PUPPY_FLASH_FW is enabled.")
  endif()

  if(NOT RESOURCES)
    message(FATAL_ERROR "Need resources when PUPPY_FLASH_FW is enabled.")
  endif()
endif()
