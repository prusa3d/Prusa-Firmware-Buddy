idf_component_register(SRCS "uart_nic.cpp" INCLUDE_DIRS ".")
target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu++23)
