set(ARDUINO_CORE_VARIANTS "2209")

if(NOT ARDUINO_CORE_TARGET)
  message(FATAL_ERROR "ARDUINO_CORE_TARGET: not set")
endif()
if(NOT ARDUINO_CORE_VARIANT IN_LIST ARDUINO_CORE_VARIANTS)
  message(FATAL_ERROR "ARDUINO_CORE_VARIANT: invalid value ${ARDUINO_CORE_VARIANT}")
endif()

add_library(
  CoreBuddy cores/arduino/Print.cpp cores/arduino/Stream.cpp cores/arduino/USBSerial.cpp
            cores/arduino/Wire.cpp cores/arduino/wiring_time.c cores/arduino/WMath.cpp
  )

target_include_directories(
  CoreBuddy
  PUBLIC
    $<$<STREQUAL:${ARDUINO_CORE_VARIANT},2209>:${CMAKE_CURRENT_SOURCE_DIR}/variants/BUDDY_F407VET6_2209>
    cores/arduino/stm32
    cores/arduino
  )

target_compile_definitions(
  CoreBuddy
  PUBLIC
  ARDUINO=187
  $<$<STREQUAL:${ARDUINO_CORE_TARGET},STM32F4xx>:STM32F4xx>
  HAVE_HWSERIAL2
  USBD_USE_CDC
  USBCON
  )

target_link_libraries(
  CoreBuddy PUBLIC STM32F4::HAL STM32::USBHost FreeRTOS::FreeRTOS tinyusb::tinyusb
  )

add_library(Arduino::CoreBuddy ALIAS CoreBuddy)
add_library(Arduino::Core ALIAS CoreBuddy)
