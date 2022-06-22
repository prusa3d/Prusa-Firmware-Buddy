if(CONNECT)
  if(NOT WUI)
    message(FATAL_ERROR "Connect needs WUI!")
  endif()
  add_subdirectory(Connect)
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

add_subdirectory(lang)
add_subdirectory(common)
add_subdirectory(hw)
add_subdirectory(marlin_stubs)
add_subdirectory(segger)
add_subdirectory(syslog)
add_subdirectory(logging)

target_sources(
  firmware
  PRIVATE fatfs.c
          filesystem.c
          filesystem_fatfs.c
          filesystem_littlefs.c
          filesystem_littlefs_internal.c
          filesystem_littlefs_bbf.cpp
          filesystem_root.c
          freertos.c
          freertos_openocd.c
          libsysbase_syscalls.c
          littlefs_bbf.cpp
          littlefs_internal.c
          logging.c
          lwip.c
          main.c
          main.cpp
          peripherals.cpp
          stm32f4xx_hal_msp.c
          stm32f4xx_it.c
          usb_device.c
          usb_device_cdc.cpp
          usb_device_msc.c
          usb_host.c
          usbh_conf.c
          usbh_diskio.c
  )

if(BOOTLOADER STREQUAL EMPTY OR BOOTLOADER)
  target_sources(firmware PRIVATE system_stm32f4xx_boot.c)
else()
  target_sources(firmware PRIVATE system_stm32f4xx.c)
endif()

set_property(
  SOURCE main.cpp
  APPEND
  PROPERTY COMPILE_DEFINITIONS FILAMENT_SENSOR=$<BOOL:${FILAMENT_SENSOR}>
  )
