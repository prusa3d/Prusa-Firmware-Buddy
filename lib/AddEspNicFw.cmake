# AddEspNicFw.cmake
#
# Handles ESP NIC firmware for printers with HAS_ESP_FLASH_TASK.
#
# By default the pre-built binaries checked into src/resources/<chip> are used. Set
# ESP_FW_BINARY_DIR to empty to build the firmware from source via Docker.
#
# The Docker image is rebuilt only when the Dockerfile changes; the firmware is rebuilt only when
# the ESP NIC source files change.

if(NOT HAS_ESP_FLASH_TASK)
  return()
endif()

# Derive chip and in-container SDK path from the hardware feature flags.
if(HAS_EMBEDDED_ESP32)
  set(ESP_CHIP "esp32")
  set(ESP_SDK_EXPORT_SH "/esp-idf/export.sh")
else()
  set(ESP_CHIP "esp8266")
  set(ESP_SDK_EXPORT_SH "/ESP8266_RTOS_SDK/export.sh")
endif()

set(ESP_FW_BINARY_DIR
    "${CMAKE_SOURCE_DIR}/src/resources/${ESP_CHIP}"
    CACHE PATH "Directory with ESP NIC firmware binaries (uart_wifi.bin, bootloader.bin, \
partition-table.bin). Defaults to the pre-built binaries in src/resources. \
Set to empty to build from source via Docker."
    )

if(NOT ESP_FW_BINARY_DIR)
  set(ESP_FW_SOURCE_DIR
      "${CMAKE_SOURCE_DIR}/lib/${ESP_CHIP}-nic"
      CACHE PATH "Source directory for the ESP NIC firmware."
      )
  set(ESP_FW_BUILD_DIR
      "${CMAKE_BINARY_DIR}/esp-fw-build"
      CACHE PATH "Build directory for ESP NIC firmware built from source."
      )
endif()

# Source build — only active when ESP_FW_BINARY_DIR is empty and this is a cross-compiled buddy
# board build.
if(NOT ESP_FW_BINARY_DIR AND BOARD IN_LIST BUDDY_BOARDS)
  get_filename_component(
    ESP_FW_BUILD_DIR "${ESP_FW_BUILD_DIR}" REALPATH BASE_DIR "${CMAKE_SOURCE_DIR}"
    )
  get_filename_component(
    ESP_FW_SOURCE_DIR "${ESP_FW_SOURCE_DIR}" REALPATH BASE_DIR "${CMAKE_SOURCE_DIR}"
    )

  set(_esp_output_dir "${ESP_FW_BUILD_DIR}/output")
  set(ESP_FW_BINARY_DIR "${_esp_output_dir}")
  set(_esp_docker_image "buddy-${ESP_CHIP}-nic-build")

  execute_process(
    COMMAND id -u
    OUTPUT_VARIABLE _esp_uid
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  execute_process(
    COMMAND id -g
    OUTPUT_VARIABLE _esp_gid
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )

  # Rebuild the Docker image only when its Dockerfile changes.
  set(_esp_docker_stamp "${CMAKE_BINARY_DIR}/esp-${ESP_CHIP}-docker.stamp")
  add_custom_command(
    OUTPUT "${_esp_docker_stamp}"
    COMMAND docker build -t "${_esp_docker_image}" "${ESP_FW_SOURCE_DIR}"
    COMMAND ${CMAKE_COMMAND} -E touch "${_esp_docker_stamp}"
    DEPENDS "${ESP_FW_SOURCE_DIR}/Dockerfile"
    COMMENT "Building ${ESP_CHIP} NIC Docker image"
    VERBATIM
    )

  # Collect ESP NIC source files for dependency tracking.
  file(GLOB_RECURSE _esp_sources "${ESP_FW_SOURCE_DIR}/main/*.c" "${ESP_FW_SOURCE_DIR}/main/*.h"
       "${ESP_FW_SOURCE_DIR}/sdkconfig"
       )
  list(APPEND _esp_sources "${ESP_FW_SOURCE_DIR}/CMakeLists.txt")

  # For ESP8266 the flasher stub comes from a separate pre-built esptool repo; copy it into the
  # output directory so ESP_FW_BINARY_DIR is self-contained.
  if(NOT HAS_EMBEDDED_ESP32)
    set(_esp_stub_dir "${CMAKE_SOURCE_DIR}/src/resources/esp8266")
    set(_esp_copy_stubs
        COMMAND
        ${CMAKE_COMMAND}
        -E
        copy_if_different
        "${_esp_stub_dir}/stub_text.bin"
        "${_esp_output_dir}/stub_text.bin"
        COMMAND
        ${CMAKE_COMMAND}
        -E
        copy_if_different
        "${_esp_stub_dir}/stub_data.bin"
        "${_esp_output_dir}/stub_data.bin"
        )
    set(_esp_stub_outputs "${_esp_output_dir}/stub_text.bin" "${_esp_output_dir}/stub_data.bin")
  else()
    set(_esp_copy_stubs "")
    set(_esp_stub_outputs "")
  endif()

  # Build ESP NIC firmware via Docker, invoking cmake directly. Rebuilds only when source files or
  # the Dockerfile change.
  add_custom_command(
    OUTPUT "${_esp_output_dir}/uart_wifi.bin" "${_esp_output_dir}/bootloader.bin"
           "${_esp_output_dir}/partition-table.bin" ${_esp_stub_outputs}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${ESP_FW_BUILD_DIR}"
    COMMAND
      docker run --rm --user "${_esp_uid}:${_esp_gid}" -v "${ESP_FW_SOURCE_DIR}:/project" -v
      "${ESP_FW_BUILD_DIR}:/build" "${_esp_docker_image}" bash -c
      "source ${ESP_SDK_EXPORT_SH} && cmake -DIDF_TARGET=${ESP_CHIP} -B /build -S /project && cmake --build /build"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${_esp_output_dir}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${ESP_FW_BUILD_DIR}/uart_wifi.bin"
            "${_esp_output_dir}/uart_wifi.bin"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${ESP_FW_BUILD_DIR}/bootloader/bootloader.bin"
            "${_esp_output_dir}/bootloader.bin"
    COMMAND
      ${CMAKE_COMMAND} -E copy_if_different
      "${ESP_FW_BUILD_DIR}/partition_table/partition-table.bin"
      "${_esp_output_dir}/partition-table.bin" ${_esp_copy_stubs}
    DEPENDS ${_esp_sources} "${_esp_docker_stamp}"
    COMMENT "Building ${ESP_CHIP} NIC firmware"
    USES_TERMINAL VERBATIM
    )

  add_custom_target(
    esp-nic-fw ALL DEPENDS "${_esp_output_dir}/uart_wifi.bin" "${_esp_output_dir}/bootloader.bin"
                           "${_esp_output_dir}/partition-table.bin" ${_esp_stub_outputs}
    )
endif()
