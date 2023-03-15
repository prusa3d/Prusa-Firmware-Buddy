add_subdirectory(startup)

if(BOARD MATCHES "MODULARBED")
  target_sources(firmware PRIVATE cmsis.c sysview.c)
else()
  target_sources(firmware PRIVATE cmsis.c core_init.cpp hal_msp.c peripherals.cpp sysview.c)
endif()
