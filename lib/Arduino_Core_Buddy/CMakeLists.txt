add_library(
  CoreBuddy cores/arduino/Print.cpp cores/arduino/SPI.cpp cores/arduino/Stream.cpp
            cores/arduino/Wire.cpp cores/arduino/wiring_time.c cores/arduino/WMath.cpp
  )

target_include_directories(
  CoreBuddy PUBLIC variants/BUDDY_F407VET6_2209 cores/arduino/stm32 cores/arduino
  )

target_compile_definitions(CoreBuddy PUBLIC ARDUINO=187 HAVE_HWSERIAL2 USBD_USE_CDC USBCON)

target_link_libraries(CoreBuddy PUBLIC FreeRTOS::FreeRTOS BuddyHeaders)

if(BOARD IN_LIST BOARDS_WITH_ARDUINO_USB_SERIAL)
  target_sources(CoreBuddy PRIVATE cores/arduino/USBSerial.cpp)
  target_link_libraries(CoreBuddy PUBLIC STM32::USBHost tinyusb::tinyusb)
endif()

add_library(Arduino::CoreBuddy ALIAS CoreBuddy)
add_library(Arduino::Core ALIAS CoreBuddy)
