get_filename_component(PROJECT_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" DIRECTORY)
get_filename_component(PROJECT_ROOT_DIR "${PROJECT_CMAKE_DIR}" DIRECTORY)

find_package(Python3 COMPONENTS Interpreter)
if(NOT Python3_FOUND)
  message(FATAL_ERROR "Python3 not found.")
endif()

function(get_recommended_gcc_version var)
  execute_process(
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_ROOT_DIR}/utils/bootstrap.py"
            "--print-dependency-version" "gcc-arm-none-eabi"
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

function(pack_firmware target)
  # parse arguments
  set(one_value_args FW_VERSION BUILD_NUMBER PRINTER_TYPE SIGNING_KEY RESOURCES_IMAGE_TARGET
                     RESOURCES_IMAGE_HASH_FILE
      )
  cmake_parse_arguments(ARG "" "${one_value_args}" "" ${ARGN})

  # calculate path for binary image of the firmware
  set(bin_firmware_path "${CMAKE_CURRENT_BINARY_DIR}/${target}.bin")

  # signing options
  if(ARG_SIGNING_KEY)
    set(sign_opts "--key" "${ARG_SIGNING_KEY}")
  else()
    set(sign_opts "--no-sign")
  endif()

  # resources options
  if(ARG_RESOURCES_IMAGE_TARGET)
    get_target_property(block_size ${ARG_RESOURCES_IMAGE_TARGET} LFS_IMAGE_BLOCK_SIZE)
    get_target_property(block_count ${ARG_RESOURCES_IMAGE_TARGET} LFS_IMAGE_BLOCK_COUNT)
    set(block_size_file "${CMAKE_CURRENT_BINARY_DIR}/block_size.bin")
    set(block_count_file "${CMAKE_CURRENT_BINARY_DIR}/block_count.bin")
    create_file_with_value("${block_size_file}" "<I" "${block_size}")
    create_file_with_value("${block_count_file}" "<I" "${block_count}")
    add_custom_target(block-describing-files DEPENDS "${block_size_file}" "${block_count_file}")
    add_dependencies(${target} block-describing-files)

    set(resources_opts
        "--tlv"
        "RESOURCES_IMAGE:$<TARGET_PROPERTY:${ARG_RESOURCES_IMAGE_TARGET},LFS_IMAGE_LOCATION>"
        "RESOURCES_IMAGE_BLOCK_SIZE:${block_size_file}"
        "RESOURCES_IMAGE_BLOCK_COUNT:${block_count_file}"
        "RESOURCES_IMAGE_HASH:${ARG_RESOURCES_IMAGE_HASH_FILE}"
        )
  endif()

  add_custom_command(
    TARGET ${target} POST_BUILD
    # generate .bin file
    COMMAND
      "${CMAKE_OBJCOPY}" -O binary -S "$<TARGET_FILE:${target}>" "${bin_firmware_path}"


      # visually separate the output
    COMMAND echo ""
            # generate .bbf file
    COMMAND
      "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/utils/pack_fw.py" --version="${ARG_FW_VERSION}"
      --printer-type "${ARG_PRINTER_TYPE}" --printer-version "1" --build-number
      "${ARG_BUILD_NUMBER}" ${sign_opts} ${resources_opts} -- "${bin_firmware_path}"
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

function(create_file_with_value file_path format value)
  set(PYTHON_CODE "import sys" "import struct" "f = open(sys.argv[1], 'wb')"
                  "f.write(struct.pack(sys.argv[2], int(sys.argv[3])))" "f.close()"
      )
  add_custom_command(
    OUTPUT ${file_path}
    COMMAND ${Python3_EXECUTABLE} "-c" "${PYTHON_CODE}" "${file_path}" "${format}" "${value}"
    VERBATIM
    )
endfunction()

function(gzip_file input_file output_file)
  set(PYTHON_CODE
      "import sys" "import gzip" "input_file = open(sys.argv[1], 'rb')"
      "output_file = open(sys.argv[2], 'wb')" "output_file.write(gzip.compress(input_file.read()))"
      )
  add_custom_command(
    OUTPUT "${output_file}"
    DEPENDS "${input_file}"
    COMMAND ${Python3_EXECUTABLE} "-c" "${PYTHON_CODE}" "${input_file}" "${output_file}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    VERBATIM
    )
endfunction()
