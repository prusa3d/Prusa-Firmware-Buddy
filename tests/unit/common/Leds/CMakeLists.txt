add_executable(
  leds_tests
  ${CMAKE_SOURCE_DIR}/tests/unit/test_main.cpp
  ${CMAKE_SOURCE_DIR}/src/gui/led_animations/animation.cpp
  ${CMAKE_SOURCE_DIR}/src/gui/led_animations/animator.cpp
  ${CMAKE_SOURCE_DIR}/tests/stubs/malloc_stub.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/led_animation_tests.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/timing_dummy.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/leds_dummy.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/freertos_mutex.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/leds_eeprom_access.cpp
  )
target_include_directories(
  leds_tests
  PUBLIC ${CMAKE_SOURCE_DIR}/tests/stubs ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/common
         ${CMAKE_SOURCE_DIR}/src/gui/led_animations ${CMAKE_SOURCE_DIR}/src/guiapi/include
         ${CMAKE_SOURCE_DIR}/src/gui/dialogs
  )
