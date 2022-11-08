add_library(
  CoreBuddy
  cores/arduino/HardwareTimer.cpp
  cores/arduino/Print.cpp
  cores/arduino/Stream.cpp
  cores/arduino/USBSerial.cpp
  cores/arduino/Wire.cpp
  cores/arduino/wiring_time.c
  cores/arduino/WMath.cpp
  )

target_include_directories(
  CoreBuddy PUBLIC variants/BUDDY_F407VET6_2209 cores/arduino/stm32 cores/arduino lib/IWatchdog
  )

target_compile_definitions(CoreBuddy PUBLIC ARDUINO=187 HAVE_HWSERIAL2 USBD_USE_CDC USBCON)

target_link_libraries(
  CoreBuddy PUBLIC STM32::USBHost FreeRTOS::FreeRTOS tinyusb::tinyusb CoreBuddy_Config
  )

add_library(Arduino::CoreBuddy ALIAS CoreBuddy)
add_library(Arduino::Core ALIAS CoreBuddy)
