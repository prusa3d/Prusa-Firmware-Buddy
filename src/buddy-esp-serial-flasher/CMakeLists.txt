set(ESP_SERIAL_FLASHER_SOURCE_DIR ${CMAKE_SOURCE_DIR}/lib/esp-serial-flasher)

add_library(
  buddy-esp-serial-flasher OBJECT
  ${ESP_SERIAL_FLASHER_SOURCE_DIR}/src/esp_loader.c
  ${ESP_SERIAL_FLASHER_SOURCE_DIR}/src/esp_targets.c
  ${ESP_SERIAL_FLASHER_SOURCE_DIR}/src/md5_hash.c
  ${ESP_SERIAL_FLASHER_SOURCE_DIR}/src/protocol_common.c
  ${ESP_SERIAL_FLASHER_SOURCE_DIR}/src/protocol_uart.c
  ${ESP_SERIAL_FLASHER_SOURCE_DIR}/src/slip.c
  buddy_port.cpp
  esp_flash.cpp
  )

add_dependencies(buddy-esp-serial-flasher esp-parts)

target_link_libraries(buddy-esp-serial-flasher PRIVATE STM32F4::HAL BuddyHeaders freertos logging)

target_include_directories(
  buddy-esp-serial-flasher
  PRIVATE ${ESP_SERIAL_FLASHER_SOURCE_DIR}/include ${ESP_SERIAL_FLASHER_SOURCE_DIR}/private_include
          ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/src
          ${CMAKE_BINARY_DIR}/src/resources/include
  )

target_compile_definitions(
  buddy-esp-serial-flasher PRIVATE SERIAL_FLASHER_INTERFACE_UART
                                   SERIAL_FLASHER_WRITE_BLOCK_RETRIES=3 MD5_ENABLED=1
  )

target_include_directories(buddy-esp-serial-flasher PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
