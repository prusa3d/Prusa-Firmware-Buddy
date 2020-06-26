set(ARDUINO_CORE_VARIANTS "2209" "2130")

if(NOT ARDUINO_CORE_TARGET)
  message(FATAL_ERROR "ARDUINO_CORE_TARGET: not set")
endif()
if(NOT ARDUINO_CORE_VARIANT IN_LIST ARDUINO_CORE_VARIANTS)
  message(FATAL_ERROR "ARDUINO_CORE_VARIANT: invalid value ${ARDUINO_CORE_VARIANT}")
endif()

add_library(
  CoreA3ides
  $<$<STREQUAL:${ARDUINO_CORE_VARIANT},2130>:${CMAKE_CURRENT_SOURCE_DIR}/variants/A3IDES_F407VET6_2130/variant.cpp>
  $<$<STREQUAL:${ARDUINO_CORE_VARIANT},2209>:${CMAKE_CURRENT_SOURCE_DIR}/variants/A3IDES_F407VET6_2209/variant.cpp>
  cores/arduino/HardwareSerial.cpp
  cores/arduino/Print.cpp
  cores/arduino/Stream.cpp
  cores/arduino/USBSerial.cpp
  cores/arduino/Wire.cpp
  cores/arduino/wiring_time.c
  cores/arduino/WMath.cpp
  )

target_include_directories(
  CoreA3ides
  PUBLIC
    $<$<STREQUAL:${ARDUINO_CORE_VARIANT},2209>:${CMAKE_CURRENT_SOURCE_DIR}/variants/A3IDES_F407VET6_2209>
    $<$<STREQUAL:${ARDUINO_CORE_VARIANT},2130>:${CMAKE_CURRENT_SOURCE_DIR}/variants/A3IDES_F407VET6_2130>
    cores/arduino/stm32
    cores/arduino
  )

target_compile_definitions(
  CoreA3ides
  PUBLIC
  ARDUINO=187
  $<$<STREQUAL:${ARDUINO_CORE_TARGET},STM32F4xx>:STM32F4xx>
  HAVE_HWSERIAL2
  USBD_USE_CDC
  USBCON
  )

target_link_libraries(
  CoreA3ides PUBLIC STM32F4::HAL STM32::USBDevice STM32::USBHost FreeRTOS::FreeRTOS
  )

add_library(Arduino::CoreA3ides ALIAS CoreA3ides)
add_library(Arduino::Core ALIAS CoreA3ides)
