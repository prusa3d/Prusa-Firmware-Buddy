add_subdirectory(startup)

target_sources(firmware PRIVATE core_init.cpp hal_msp.cpp interrupts.cpp peripherals.cpp)

if(BOOTLOADER)
  target_sources(firmware PRIVATE cmsis_boot.cpp)
else()
  target_sources(firmware PRIVATE cmsis.cpp)
endif()
