get_filename_component(PROJECT_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" DIRECTORY)
get_filename_component(PROJECT_ROOT_DIR "${PROJECT_CMAKE_DIR}" DIRECTORY)

find_package(Python3 COMPONENTS Interpreter)
if(NOT Python3_FOUND)
  message(FATAL_ERROR "Python3 not found.")
endif()

function(get_recommended_gcc_version var)
  execute_process(
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_ROOT_DIR}/utils/bootstrap.py"
            "--print-dependency-version" "avr-gcc"
    OUTPUT_VARIABLE RECOMMENDED_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE RETVAL
    )

  if(NOT "${RETVAL}" STREQUAL "0")
    message(FATAL_ERROR "Failed to obtain recommended gcc version from utils/bootstrap.py")
  endif()

  set(${var}
      ${RECOMMENDED_VERSION}
      PARENT_SCOPE
      )
endfunction()

function(get_dependency_directory dependency var)
  execute_process(
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_ROOT_DIR}/utils/bootstrap.py"
            "--print-dependency-directory" "${dependency}"
    OUTPUT_VARIABLE DEPENDENCY_DIRECTORY
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE RETVAL
    )

  if(NOT "${RETVAL}" STREQUAL "0")
    message(FATAL_ERROR "Failed to find directory with ${dependency}")
  endif()

  set(${var}
      ${DEPENDENCY_DIRECTORY}
      PARENT_SCOPE
      )
endfunction()

function(objcopy target format suffix)
  add_custom_command(
    TARGET ${target} POST_BUILD
    COMMAND "${CMAKE_OBJCOPY}" -O ${format} -S "$<TARGET_FILE:${target}>"
            "${CMAKE_CURRENT_BINARY_DIR}/${target}${suffix}"
    COMMENT "Generating ${format} from ${target}..."
    )
endfunction()

function(report_size target)
  add_custom_command(
    TARGET ${target} POST_BUILD
    COMMAND echo "" # visually separate the output
    COMMAND "${CMAKE_SIZE_UTIL}" -B "$<TARGET_FILE:${target}>"
    USES_TERMINAL
    )
endfunction()

function(pack_firmware target fw_version build_number printer_type signing_key)
  set(bin_firmware_path "${CMAKE_CURRENT_BINARY_DIR}/${target}.bin")
  if(SIGNING_KEY)
    set(sign_opts "--key" "${signing_key}")
  else()
    set(sign_opts "--no-sign")
  endif()
  add_custom_command(
    TARGET ${target} POST_BUILD
    COMMAND "${CMAKE_OBJCOPY}" -O binary -S "$<TARGET_FILE:${target}>" "${bin_firmware_path}"
    COMMAND echo "" # visually separate the output
    COMMAND
      "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/utils/pack_fw.py" --version="${fw_version}"
      --printer-type "${printer_type}" --printer-version "1" ${sign_opts} "${bin_firmware_path}"
      --build-number "${build_number}"
    )
endfunction()

function(create_dfu)
  set(options)
  set(one_value_args OUTPUT TARGET)
  set(multi_value_args INPUT)
  cmake_parse_arguments(CREATE_DFU "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

  add_custom_command(
    TARGET "${CREATE_DFU_TARGET}" POST_BUILD
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/utils/dfu.py" create ${CREATE_DFU_INPUT}
            "${CREATE_DFU_OUTPUT}"
    )
endfunction()

function(add_link_dependency target file_path)
  get_target_property(link_deps ${target} LINK_DEPENDS)
  if(link_deps STREQUAL "link_deps-NOTFOUND")
    set(link_deps "")
  endif()
  list(APPEND link_deps "${file_path}")
  set_target_properties(${target} PROPERTIES LINK_DEPENDS "${link_deps}")
endfunction()

function(rfc1123_datetime var)
  set(cmd
      "from email.utils import formatdate; print(formatdate(timeval=None, localtime=False, usegmt=True))"
      )
  execute_process(
    COMMAND "${Python3_EXECUTABLE}" -c "${cmd}"
    OUTPUT_VARIABLE RFC1123_DATETIME
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE RETVAL
    )
  if(NOT "${RETVAL}" STREQUAL "0")
    message(FATAL_ERROR "Failed to obtain rfc1123 date time from Python")
  endif()
  set(${var}
      ${RFC1123_DATETIME}
      PARENT_SCOPE
      )
endfunction()
