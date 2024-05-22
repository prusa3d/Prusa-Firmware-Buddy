cmake_minimum_required(VERSION 3.15)

project(
  Buddy
  LANGUAGES C ASM
  VERSION 1.0.0
  )

add_library(flasher INTERFACE)

target_sources(
  flasher INTERFACE prusa/src/serial-flasher/buddy_port.c prusa/src/serial-flasher/serial_comm.c
                    serial-flasher/src/esp_loader.c serial-flasher/src/esp_targets.c
  )

target_include_directories(
  flasher INTERFACE serial-flasher/include serial-flasher/port serial-flasher/private_include
  )
