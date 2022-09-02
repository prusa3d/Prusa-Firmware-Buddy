target_include_directories(firmware PRIVATE .)

add_subdirectory(device)
add_subdirectory(segger)
add_subdirectory(hw)
add_subdirectory(logging)
add_subdirectory(common)
add_subdirectory(configuration_store)

if(CONNECT)
  if(NOT WUI)
    message(FATAL_ERROR "Connect needs WUI!")
  endif()
  add_subdirectory(connect)
endif()

if(RESOURCES)
  add_subdirectory(resources)
endif()

if(BOOTLOADER)
  add_subdirectory(bootloader)
endif()

if(GUI)
  add_subdirectory(gui)
  add_subdirectory(guiapi)
endif()

if(BOARD MATCHES "BUDDY")
  add_subdirectory(buddy)
  add_subdirectory(lang)
  add_subdirectory(marlin_stubs)
  add_subdirectory(syslog)
  add_subdirectory(semihosting)
endif()

target_sources(firmware PRIVATE freertos.c freertos_openocd.c)
