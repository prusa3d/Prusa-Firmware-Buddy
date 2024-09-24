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
    "${CMAKE_SOURCE_DIR}/include/option/option_boolean.h.in"
    "${OPTIONS_INCLUDE_DIR}/option/${OPTION_NAME_lowered}.h"
    )
endfunction()

function(to_pascal_case out_var str)
  execute_process(
    COMMAND
      "${Python3_EXECUTABLE}" "-c"
      "import string, sys; print(string.capwords(sys.argv[1].replace('_', ' ')).replace(' ', ''))"
      "${str}"
    OUTPUT_VARIABLE result
    OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ERROR_IS_FATAL ANY
    )
  set(${out_var}
      "${result}"
      PARENT_SCOPE
      )
endfunction()

function(define_enum_option)
  set(one_value_args NAME VALUE)
  set(multi_value_args ALL_VALUES)
  cmake_parse_arguments(ARG "" "${one_value_args}" "${multi_value_args}" ${ARGN})

  set(option_name "${ARG_NAME}")
  set(option_value "${ARG_VALUE}")
  string(TOLOWER "${option_name}" option_name_lower)
  string(TOUPPER "${option_name}" option_name_upper)
  set(option_values "${ARG_ALL_VALUES}")

  # assign numbers to values
  set(current_number "1")
  foreach(value ${option_values})
    set(value_${value}_number "${current_number}")
    math(EXPR current_number "${current_number} + 1")
  endforeach()

  # create file with C header
  set(input_file_prefix "${OPTIONS_INCLUDE_DIR}/option/${option_name_lower}")
  set(input_file "${input_file_prefix}.in")
  set(output_file "${input_file_prefix}.h")
  file(WRITE "${input_file}" "#pragma once\n\n")

  # list all values (C defines)
  foreach(value ${option_values})
    file(APPEND "${input_file}" "#define ${option_name_upper}_${value} ${value_${value}_number}\n")
  endforeach()
  file(APPEND "${input_file}" "\n")

  # create main option getter: #define OPTION_NAME() <value>
  file(APPEND "${input_file}" "#define ${option_name_upper}() ${value_${option_value}_number}\n")
  file(APPEND "${input_file}" "\n")

  # define easy checks: #define OPTION_IS_X() 0
  foreach(value ${option_values})
    if(value STREQUAL option_value)
      set(value_is_x "1")
    else()
      set(value_is_x "0")
    endif()
    file(APPEND "${input_file}" "#define ${option_name_upper}_IS_${value}() ${value_is_x}\n")
  endforeach()
  file(APPEND "${input_file}" "\n")

  file(APPEND "${input_file}" "#ifdef __cplusplus\n")
  file(APPEND "${input_file}" "namespace option {\n\n")

  # add c++ enum class
  to_pascal_case(option_name_pascal "${option_name_lower}")
  file(APPEND "${input_file}" "enum class ${option_name_pascal} {\n")
  foreach(value ${option_values})
    string(TOLOWER "${value}" value_lower)
    file(APPEND "${input_file}" "    ${value_lower} = ${value_${value}_number},\n")
  endforeach()
  file(APPEND "${input_file}" "};\n\n")

  # add C++ constexpr value
  string(TOLOWER "${option_value}" option_value_lower)
  file(
    APPEND "${input_file}"
    "inline constexpr ${option_name_pascal} ${option_name_lower} = ${option_name_pascal}::${option_value_lower};\n\n"
    )

  file(APPEND "${input_file}" "};\n")
  file(APPEND "${input_file}" "#endif // __cplusplus\n")

  configure_file(${input_file} ${output_file} COPYONLY)
endfunction()

add_library(options INTERFACE)
target_include_directories(options INTERFACE ${OPTIONS_INCLUDE_DIR})
