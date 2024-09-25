idf_component_register(SRCS "uart_nic.cpp" "uart0_driver.cpp" INCLUDE_DIRS ".")
target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu++2a)
