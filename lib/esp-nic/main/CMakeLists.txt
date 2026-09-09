set(sources)

if(IDF_TARGET STREQUAL "esp8266")
  list(APPEND sources "uart_nic_esp8266.c" "uart0_driver.c")
elseif(IDF_TARGET STREQUAL "esp32")
  list(APPEND sources "uart_nic_esp32.c")
else()
  message(FATAL_ERROR "unsupported target")
endif()

idf_component_register(SRCS ${sources} INCLUDE_DIRS ".")
