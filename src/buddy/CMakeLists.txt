target_sources(
  firmware
  PRIVATE bootstrap_state.cpp
          cxa_guard.cpp
          digest.cpp
          fatfs.cpp
          filesystem.cpp
          filesystem_fatfs.cpp
          filesystem_littlefs.cpp
          filesystem_littlefs_internal.cpp
          filesystem_root.cpp
          filesystem_semihosting.cpp
          libsysbase_syscalls.cpp
          littlefs_internal.cpp
          logging.cpp
          lwip.cpp
          main.cpp
          timing_sys.cpp
          usb_device.cpp
          usb_device_cdc.cpp
          usb_host.cpp
          usbh_conf.cpp
          usbh_diskio.cpp
  )

if(HAS_ESP)
  target_sources(firmware PRIVATE esp_flash_task.cpp)
endif()

if(HAS_DOOR_SENSOR)
  target_sources(firmware PRIVATE door_sensor.cpp)
endif()

if(HAS_MMU2)
  target_sources(firmware PRIVATE mmu_port.cpp)
endif()

set_property(
  SOURCE src/main.cpp
  APPEND
  PROPERTY COMPILE_DEFINITIONS HAS_MMU2=$<BOOL:${HAS_MMU2}>
  )

if(RTT_METRICS_ENABLED)
  target_link_libraries(firmware PRIVATE rtt_metrics_task)
endif()
