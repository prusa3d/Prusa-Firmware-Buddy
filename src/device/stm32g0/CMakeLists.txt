add_subdirectory(startup)

if(BOARD STREQUAL "MODULARBED")
  target_sources(firmware PRIVATE cmsis.cpp sysview.cpp)
else()
  target_sources(firmware PRIVATE cmsis.cpp core_init.cpp hal_msp.cpp peripherals.cpp sysview.cpp)
endif()
