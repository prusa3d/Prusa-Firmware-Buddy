get_filename_component(PROJECT_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" DIRECTORY)
get_filename_component(PROJECT_ROOT_DIR "${PROJECT_CMAKE_DIR}" DIRECTORY)

# cache BUDDY_NO_VIRTUALENV across builds
set(BUDDY_NO_VIRTUALENV
    $<BOOL:$ENV{BUDDY_NO_VIRTUALENV}>
    CACHE BOOL "Disable python virtualenv management"
    )
if(NOT Python3_EXECUTABLE)
  if(NOT ${BUDDY_NO_VIRTUALENV})
    set(Python3_ROOT_DIR "${CMAKE_SOURCE_DIR}/.venv")
    set(Python3_FIND_VIRTUALENV "ONLY")
  endif()
  find_package(Python3 COMPONENTS Interpreter)
  if(NOT Python3_FOUND)
    message(FATAL_ERROR "Python3 not found.")
  else()
    message(STATUS "Python3: ${Python3_EXECUTABLE}")
  endif()
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

function(get_dependency_version dependency var)
  execute_process(
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_ROOT_DIR}/utils/bootstrap.py"
            "--print-dependency-version" "${dependency}"
    OUTPUT_VARIABLE DEPENDENCY_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE RETVAL
    )

  if(NOT "${RETVAL}" STREQUAL "0")
    message(FATAL_ERROR "Failed to find directory with ${dependency}")
  endif()

  set(${var}
      ${DEPENDENCY_VERSION}
      PARENT_SCOPE
      )
endfunction()

function(objcopy target format suffix)
  set(bin_filename "${CMAKE_CURRENT_BINARY_DIR}/${target}${suffix}")
  add_custom_command(
    TARGET ${target}
    POST_BUILD
    COMMAND "${CMAKE_OBJCOPY}" -O ${format} -S "$<TARGET_FILE:${target}>" "${bin_filename}"
    COMMENT "Generating ${format} from ${target}..."
    BYPRODUCTS "${bin_filename}"
    )
endfunction()

function(pack_firmware target)
  # parse arguments
  set(one_value_args FW_VERSION BUILD_NUMBER PRINTER_TYPE PRINTER_VERSION PRINTER_SUBVERSION
                     SIGNING_KEY
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

  set(bbf_firmware_path "${CMAKE_CURRENT_BINARY_DIR}/${target}.bbf")

  set(resources_opts)
  set(objcopy_opts)
  set(digest_deps)
  set(bbf_deps)

  if(RESOURCES)
    get_target_property(resources_tar resources-tarball TAR_IMAGE_LOCATION)
    set(resources_digest "${resources_tar}.sha256")
    sha256_file("${resources_tar}" "${resources_digest}")
    list(APPEND resources_opts "--tlv" "RESOURCES_TARBALL:${resources_tar}"
         "RESOURCES_TARBALL_DIGEST:${resources_digest}"
         )
    list(APPEND objcopy_opts "--update-section" ".resources_tarball_digest=${resources_digest}")
    list(APPEND digest_deps "${resources_digest}")
    list(APPEND bbf_deps "${resources_tar}" "${resources_digest}")
  endif()

  if(BOOTLOADER_UPDATE)
    get_target_property(bootloader_tar bootloader-tarball TAR_IMAGE_LOCATION)
    set(bootloader_digest "${bootloader_tar}.sha256")
    sha256_file("${bootloader_tar}" "${bootloader_digest}")
    list(APPEND resources_opts "--tlv" "BOOTLOADER_TARBALL:${bootloader_tar}"
         "BOOTLOADER_TARBALL_DIGEST:${bootloader_digest}"
         )
    list(APPEND objcopy_opts "--update-section" ".bootloader_tarball_digest=${bootloader_digest}")
    list(APPEND digest_deps "${bootloader_digest}")
    list(APPEND bbf_deps "${bootloader_tar}" "${bootloader_digest}")
  endif()

  add_custom_command(
    OUTPUT "${bin_firmware_path}"
    COMMAND "${CMAKE_OBJCOPY}" ${objcopy_opts} "$<TARGET_FILE:${target}>" "$<TARGET_FILE:${target}>"
    COMMAND "${CMAKE_OBJCOPY}" -O binary -S "$<TARGET_FILE:${target}>" "${bin_firmware_path}"
    DEPENDS ${target} ${digest_deps}
    )

  add_custom_command(
    OUTPUT "${bbf_firmware_path}"
    COMMAND
      "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/utils/pack_fw.py" --version="${ARG_FW_VERSION}"
      --printer-type "${ARG_PRINTER_TYPE}" --printer-version "${ARG_PRINTER_VERSION}"
      --printer-subversion "${ARG_PRINTER_SUBVERSION}" --build-number "${ARG_BUILD_NUMBER}"
      ${sign_opts} ${resources_opts} -- "${bin_firmware_path}"
    DEPENDS "${bin_firmware_path}" ${bbf_deps}
    )

  add_custom_target(${target}-bbf ALL DEPENDS "${bbf_firmware_path}")
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
      "import sys"
      "import io"
      "import gzip"
      "input_file = open(sys.argv[1], 'rb')"
      "bytes_io = io.BytesIO()"
      "gzip_file = gzip.GzipFile(fileobj=bytes_io, mode='wb', mtime=0)"
      "gzip_file.write(input_file.read())"
      "gzip_file.close()"
      "bytes_io.seek(0)"
      "output_file = open(sys.argv[2], 'wb')"
      "output_file.write(bytes_io.read())"
      )
  add_custom_command(
    OUTPUT "${output_file}"
    DEPENDS "${input_file}"
    COMMAND ${Python3_EXECUTABLE} "-c" "${PYTHON_CODE}" "${input_file}" "${output_file}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    VERBATIM
    )
endfunction()

function(target_set_linker_script target linker_script)
  target_link_options(${target} PRIVATE "-Wl,-T,${linker_script}")
  add_custom_target("${target}_linker_script" DEPENDS "${linker_script}")
  add_dependencies(${target} "${target}_linker_script")
endfunction()
