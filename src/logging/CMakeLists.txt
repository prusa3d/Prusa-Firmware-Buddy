target_sources(
  firmware
  PRIVATE log.c
          log_dest_rtt.c
          log_dest_shared.c
          log_dest_swo.c
          log_dest_syslog.c
          log_dest_usb.c
          log_platform.c
  )
