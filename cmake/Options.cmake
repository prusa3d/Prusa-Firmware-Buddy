set(OPTIONS_INCLUDE_DIR "${CMAKE_BINARY_DIR}/include")

function(define_boolean_option name value)
  set(${name}
      ${value}
      PARENT_SCOPE
      )
  set(OPTION_NAME ${name})
  string(TOLOWER ${OPTION_NAME} OPTION_NAME_lowered)
  if(value)
    set(OPTION_ENABLED "1")
    message(STATUS "Option ${name}: Enabled")
  else()
    set(OPTION_ENABLED "0")
    message(STATUS "Option ${name}: Disabled")
  endif()
  configure_file(
    "${CMAKE_SOURCE_DIR}/include/option/option.h.in"
    "${OPTIONS_INCLUDE_DIR}/option/${OPTION_NAME_lowered}.h"
    )
endfunction()

function(define_enum_option)

endfunction()
