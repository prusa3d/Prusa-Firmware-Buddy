# Cmake for puppies (Dwarf + Modular bed)

target_sources(firmware PUBLIC modbus.cpp puppy_task.cpp PuppyBus.cpp PuppyModbus.cpp time_sync.cpp)

target_link_libraries(firmware PRIVATE crc modbus)

target_link_libraries(BuddyHeaders INTERFACE lightmodbus)

if(HAS_PUPPY_BOOTSTRAP)
  target_sources(firmware PUBLIC puppy_crash_dump.cpp PuppyBootstrap.cpp)
endif()

if(HAS_DWARF)
  target_sources(firmware PUBLIC Dwarf.cpp)
endif()

if(HAS_INDX)
  target_sources(firmware PUBLIC INDX.cpp)
  target_link_libraries(firmware PUBLIC indx_head)
  target_link_libraries(BuddyHeaders INTERFACE indx_head)
endif()

if(HAS_PUPPY_MODULARBED)
  target_sources(firmware PUBLIC modular_bed.cpp)
endif()

if(HAS_XBUDDY_EXTENSION)
  target_sources(
    firmware PUBLIC cyphal_bridge.cpp out_of_band_bootstrap.cpp xbuddy_extension.cpp
                    xbuddy_extension_bootstrap.cpp
    )
endif()

if(HAS_XL_CAN)
  target_sources(
    firmware PUBLIC cyphal_bridge.cpp out_of_band_bootstrap.cpp xl_can.cpp xl_can_bootstrap.cpp
    )
  # Bridge fw is the xl_can variant of xBuddy Extension, so its Modbus map (and shared headers) come
  # from the xbuddy_extension lib; a bridge-specific reduced layout is a possible future
  # optimization.
  target_link_libraries(firmware PUBLIC xbuddy_extension)
endif()

if(HAS_XBUDDY_EXTENSION OR HAS_XL_CAN)
  target_sources(firmware PUBLIC cyphal_flash_files.cpp cyphal_flash_host.cpp)
endif()

if(HAS_AC_CONTROLLER)
  target_sources(firmware PUBLIC ac_controller.cpp)
  target_link_libraries(firmware PUBLIC ac_controller)
endif()

if(HAS_TOOL_OFFSET_SENSOR)
  target_sources(firmware PUBLIC tool_offset_sensor.cpp)
  target_link_libraries(firmware PUBLIC tool_offset_sensor)
endif()

if(HAS_MMU2)
  target_sources(firmware PUBLIC mmu.cpp)
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
