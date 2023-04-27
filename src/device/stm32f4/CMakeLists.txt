add_subdirectory(startup)

target_sources(firmware PRIVATE core_init.cpp hal_msp.c interrupts.cpp peripherals.cpp)

if(BOOTLOADER)
  target_sources(firmware PRIVATE cmsis_boot.c)
else()
  target_sources(firmware PRIVATE cmsis.c)
endif()
