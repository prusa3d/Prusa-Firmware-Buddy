target_sources(
  firmware
  PRIVATE fatfs.c
          filesystem.c
          filesystem_fatfs.c
          filesystem_littlefs.c
          filesystem_littlefs_bbf.cpp
          filesystem_littlefs_internal.c
          filesystem_root.c
          filesystem_semihosting.cpp
          libsysbase_syscalls.c
          littlefs_bbf.cpp
          littlefs_internal.c
          logging.c
          lwip.c
          main.c
          main.cpp
          timing_sys.c
          usb_device.c
          usb_device_cdc.cpp
          usb_host.c
          usbh_conf.c
          usbh_diskio.c
  )

set_property(
  SOURCE src/main.cpp
  APPEND
  PROPERTY COMPILE_DEFINITIONS HAS_MMU2=$<BOOL:${HAS_MMU2}>
  )
