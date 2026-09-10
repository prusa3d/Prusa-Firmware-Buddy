set(sources)

if(IDF_TARGET STREQUAL "esp8266")
  list(APPEND sources "uart_nic_esp8266.cpp" "uart0_driver.cpp")
elseif(IDF_TARGET STREQUAL "esp32")
  list(APPEND sources "uart_nic_esp32.cpp")
else()
  message(FATAL_ERROR "unsupported target")
endif()

idf_component_register(SRCS ${sources} INCLUDE_DIRS ".")
target_compile_options(${COMPONENT_LIB} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++17>)
