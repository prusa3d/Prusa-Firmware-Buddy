target_sources(
  firmware
  PRIVATE mmu2_error_converter.cpp
          mmu2_fsensor.cpp
          mmu2_power.cpp
          mmu2_reporting.cpp
          mmu2_serial.cpp
          tmp_error_converter.cpp
          tmp_progress_converter.cpp
  )
