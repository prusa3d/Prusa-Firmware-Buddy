set(sources "common.cpp")

if(IDF_TARGET STREQUAL "esp8266")
  list(APPEND sources "uart_nic_esp8266.cpp" "uart0_driver.cpp")
elseif(IDF_TARGET STREQUAL "esp32")
  list(APPEND sources "uart_nic_esp32.cpp")
else()
  message(FATAL_ERROR "unsupported target")
endif()

idf_component_register(SRCS ${sources} INCLUDE_DIRS ".")
target_compile_options(${COMPONENT_LIB} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++17>)

# cmake-format: off
target_compile_options(
  ${COMPONENT_LIB}
  PRIVATE -Wall
          -Wfloat-conversion
          -Wdouble-promotion
          -Wno-deprecated-declarations
          -Wunused-parameter
          $<$<COMPILE_LANGUAGE:CXX>:-Wextra>
          $<$<COMPILE_LANGUAGE:CXX>:-Werror=delete-non-virtual-dtor>
          $<$<COMPILE_LANGUAGE:CXX>:-Werror=switch>
          $<$<COMPILE_LANGUAGE:CXX>:-Wduplicated-cond>
          $<$<COMPILE_LANGUAGE:CXX>:-Wlogical-op>
          $<$<COMPILE_LANGUAGE:CXX>:-Wnull-dereference>
          $<$<COMPILE_LANGUAGE:CXX>:-Wshadow=compatible-local>
          $<$<COMPILE_LANGUAGE:CXX>:-Wvla>
          $<$<COMPILE_LANGUAGE:CXX>:-Wno-expansion-to-defined>
          $<$<COMPILE_LANGUAGE:CXX>:-Wno-format-zero-length>
          $<$<COMPILE_LANGUAGE:CXX>:-Wno-non-virtual-dtor>
          $<$<COMPILE_LANGUAGE:CXX>:-Wno-psabi>
          $<$<COMPILE_LANGUAGE:CXX>:-Wno-register>
  )
# cmake-format: on
