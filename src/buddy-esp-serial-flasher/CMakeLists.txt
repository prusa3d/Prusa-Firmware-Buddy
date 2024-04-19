set(ESP_SERIAL_FLASHER_SOURCE_DIR ${CMAKE_SOURCE_DIR}/lib/esp-serial-flasher)

add_library(
  buddy-esp-serial-flasher OBJECT
  ${ESP_SERIAL_FLASHER_SOURCE_DIR}/src/esp_loader.c
  ${ESP_SERIAL_FLASHER_SOURCE_DIR}/src/esp_targets.c
  ${ESP_SERIAL_FLASHER_SOURCE_DIR}/src/protocol_common.c
  ${ESP_SERIAL_FLASHER_SOURCE_DIR}/src/protocol_uart.c
  ${ESP_SERIAL_FLASHER_SOURCE_DIR}/src/slip.c
  buddy_port.cpp
  )

target_link_libraries(buddy-esp-serial-flasher PRIVATE STM32F4::HAL FreeRTOS::FreeRTOS)
target_include_directories(
  buddy-esp-serial-flasher PRIVATE ${ESP_SERIAL_FLASHER_SOURCE_DIR}/include
                                   ${ESP_SERIAL_FLASHER_SOURCE_DIR}/private_include
  )

target_compile_definitions(
  buddy-esp-serial-flasher PUBLIC SERIAL_FLASHER_INTERFACE_UART
                                  SERIAL_FLASHER_WRITE_BLOCK_RETRIES=3
  )

target_compile_definitions(
  buddy-esp-serial-flasher PUBLIC SERIAL_FLASHER_INTERFACE_UART
                                  SERIAL_FLASHER_WRITE_BLOCK_RETRIES=3
  )

target_include_directories(
  buddy-esp-serial-flasher PUBLIC ${ESP_SERIAL_FLASHER_SOURCE_DIR}/include
                                  ${CMAKE_CURRENT_SOURCE_DIR}
  )

if(PRINTER STREQUAL "XL")
  target_compile_definitions(buddy-esp-serial-flasher PUBLIC MD5_ENABLED=1)
  target_sources(buddy-esp-serial-flasher PRIVATE ${ESP_SERIAL_FLASHER_SOURCE_DIR}/src/md5_hash.c)
else()
  target_compile_definitions(buddy-esp-serial-flasher PUBLIC MD5_ENABLED=0)
endif()
