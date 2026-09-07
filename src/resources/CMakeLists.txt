include(QoiGenerator.cmake)
#
# Common Functions
#
set(resources_include_dir "${CMAKE_CURRENT_BINARY_DIR}/include")

function(add_resource source_path install_path)
  tar_image_add_file(resources-tarball "${source_path}" "${install_path}")
endfunction()

function(add_gzip_resource source_path install_path)
  set(gzip_file_path "${CMAKE_CURRENT_BINARY_DIR}/gzipped${install_path}")
  gzip_file("${source_path}" "${gzip_file_path}")
  add_resource("${gzip_file_path}" "${install_path}")
endfunction()

#
# General
#

target_sources(firmware PUBLIC bootstrap.cpp revision.cpp tarball.cpp)
target_include_directories(firmware PUBLIC "${resources_include_dir}")

#
# Standard Resources Image
#

add_tar_image(resources-tarball)

if(HAS_ESP)
  set(ESP_PARTS ${CMAKE_CURRENT_BINARY_DIR}/include/esp_parts.gen)
  if(HAS_EMBEDDED_ESP32)
    add_resource("esp32/uart_wifi.bin" "/esp/uart_wifi.bin")
    add_resource("esp32/bootloader.bin" "/esp/bootloader.bin")
    add_resource("esp32/partition-table.bin" "/esp/partition-table.bin")
    add_custom_command(
      OUTPUT ${ESP_PARTS}
      COMMAND
        "${Python3_EXECUTABLE}" ${CMAKE_SOURCE_DIR}/utils/gen_esp_parts.py ARGS --output
        ${ESP_PARTS} --basedir ${CMAKE_CURRENT_SOURCE_DIR}/esp32/ --flash
        partition-table.bin:0x08000 --flash bootloader.bin:0x01000 --flash uart_wifi.bin:0x10000
      DEPENDS esp32/uart_wifi.bin esp32/bootloader.bin esp32/partition-table.bin
              ${CMAKE_SOURCE_DIR}/utils/gen_esp_parts.py
      )
  else()
    add_resource("esp8266/uart_wifi.bin" "/esp/uart_wifi.bin")
    add_resource("esp8266/bootloader.bin" "/esp/bootloader.bin")
    add_resource("esp8266/partition-table.bin" "/esp/partition-table.bin")
    add_resource("esp8266/stub_text.bin" "/esp/stub_text.bin")
    add_resource("esp8266/stub_data.bin" "/esp/stub_data.bin")
    add_custom_command(
      OUTPUT ${ESP_PARTS}
      COMMAND
        "${Python3_EXECUTABLE}" ${CMAKE_SOURCE_DIR}/utils/gen_esp_parts.py ARGS --output
        ${ESP_PARTS} --basedir ${CMAKE_CURRENT_SOURCE_DIR}/esp8266/ --flash
        partition-table.bin:0x08000 --flash bootloader.bin:0x00000 --flash uart_wifi.bin:0x10000
        --memory stub_text.bin:0x4010d000 --memory stub_data.bin:0x3fff01b4
      DEPENDS esp8266/uart_wifi.bin esp8266/bootloader.bin esp8266/partition-table.bin
              esp8266/stub_text.bin esp8266/stub_data.bin
              ${CMAKE_SOURCE_DIR}/utils/gen_esp_parts.py
      )
  endif()
  add_custom_target(esp-parts DEPENDS ${ESP_PARTS})
endif()

if(HAS_AC_CONTROLLER)
  string(TOLOWER ${PRINTER} printer_low)
  get_dependency_directory("firmware-${printer_low}_ac_controller" firmware_dir)
  set(AC_CONTROLLER_BINARY_PATH "${firmware_dir}/firmware.bin")
endif()

if(PUPPY_FLASH_FW)
  if(HAS_DWARF)
    add_resource("${DWARF_BINARY_PATH}" "/puppies/fw-dwarf.bin")
  endif()
  if(HAS_PUPPY_MODULARBED)
    add_resource("${MODULARBED_BINARY_PATH}" "/puppies/fw-modularbed.bin")
  endif()
  if(HAS_XBUDDY_EXTENSION)
    add_resource("${XBUDDY_EXTENSION_BINARY_PATH}" "/puppies/fw-xbuddy-extension.bin")
  endif()
  if(HAS_XL_CAN)
    add_resource("${XL_CAN_BINARY_PATH}" "/puppies/fw-xl_can.bin")
  endif()
  if(HAS_AC_CONTROLLER)
    add_resource("${AC_CONTROLLER_BINARY_PATH}" "/puppies/fw-ac_controller.bin")
  endif()
  if(HAS_ANFC)
    add_resource("${ANFC_BINARY_PATH}" "/puppies/fw-anfc.bin")
  endif()
  if(HAS_TOOL_OFFSET_SENSOR)
    add_resource("${TOOL_OFFSET_SENSOR_BINARY_PATH}" "/puppies/fw-tool_offset_sensor.bin")
  endif()
  if(HAS_INDX_HEAD)
    add_resource("${INDX_HEAD_BINARY_PATH}" "/puppies/fw-indx_head.bin")
  endif()
endif()

# /web directory
add_gzip_resource("web/favicon.ico" "/web/favicon.ico")
add_gzip_resource("web/index.html" "/web/index.html")
add_gzip_resource("web/main.b41402ed4d6d019ca007.js" "/web/main.b41402ed4d6d019ca007.js")
add_gzip_resource("web/main.71c989a4a290e5c10892.css" "/web/main.71c989a4a290e5c10892.css")

# add languages
set(compile_mo_py "${CMAKE_SOURCE_DIR}/utils/translations_and_fonts/compile_mo.py")
if(${TRANSLATIONS_ENABLED})
  foreach(LANG ${LANGUAGES_AVAILABLE})
    if(${ENABLE_TRANSLATION_${LANG}})
      string(TOLOWER ${LANG} LANG)
      set(PO_FILE "${CMAKE_SOURCE_DIR}/src/lang/po/${LANG}/Prusa-Firmware-Buddy_${LANG}.po")
      set(MO_FILE "${CMAKE_CURRENT_BINARY_DIR}/${LANG}.mo")
      add_custom_command(
        OUTPUT ${MO_FILE}
        COMMAND "${Python3_EXECUTABLE}" "${compile_mo_py}" --input ${PO_FILE} --output ${MO_FILE}
                --binary "${firmware_without_digest_bin}"
        DEPENDS ${PO_FILE} "${compile_mo_py}" "${firmware_without_digest_bin}"
        VERBATIM
        )
      add_custom_target(generate_mo_${LANG} ALL DEPENDS ${MO_FILE})
      add_resource("${MO_FILE}" "/lang/${LANG}.mo")
    endif()
  endforeach()
endif()

# Quite OK images
add_resource("${qoi_data_file}" "/qoi.data")

#
# Bootloader Resources Image
#
if(BOOTLOADER_UPDATE)

  get_dependency_directory("bootloader-${BOOTLOADER_VARIANT}" bootloader_dir)
  get_dependency_version("bootloader-${BOOTLOADER_VARIANT}" bootloader_version_str)
  set(bootloader_bin "${bootloader_dir}/bootloader.bin")

  add_tar_image(bootloader-tarball)
  tar_image_add_file(bootloader-tarball "${bootloader_bin}" "/bootloader.bin")
  tar_image_build(bootloader-tarball)

  # parse bootloader version
  string(REGEX MATCH "([0-9]+)\.([0-9]+)\.([0-9]+)" result "${bootloader_version_str}")
  if(NOT result)
    message(FATAL_ERROR "Failed to parse bootloader version ${bootloader_version_str}")
  endif()
  set(bootloader_version ${CMAKE_MATCH_0})

  # generate header file with bootloader version
  configure_file(
    required_bootloader_version.hpp.in "${resources_include_dir}/bootloader/required_version.hpp"
    )

endif()

# MMU Firmware resource
if(HAS_MMU2)
  # Returns directory with a single MMU hex FW - version set in bootstrap.py
  get_dependency_directory("firmware-mmu" mmu_fw_dir)
  get_dependency_version("firmware-mmu" mmu_fw_version)

  file(GLOB mmu_fw_hex_file "${mmu_fw_dir}/*.hex")
  if(NOT EXISTS "${mmu_fw_hex_file}")
    message(
      FATAL_ERROR
        "Failed to locate MMU FW in .dependencies. You probably need to rerun bootstrap.py"
      )
  endif()

  set(mmu_fw_file_base "${CMAKE_CURRENT_BINARY_DIR}/mmu_fw_${mmu_fw_version}")
  set(mmu_fw_adj_hex_file "${mmu_fw_file_base}.hex")
  set(mmu_fw_bin_file "${mmu_fw_file_base}.bin")

  # We need to adjust the hex file and strip the comments and empty lines, otherwise the objcopy
  # will not get it
  add_custom_command(
    OUTPUT "${mmu_fw_adj_hex_file}"
    COMMAND "${Python3_EXECUTABLE}" "${PROJECT_ROOT_DIR}/utils/mmu_ihex_cleanup.py"
            "${mmu_fw_hex_file}" "${mmu_fw_adj_hex_file}"
    VERBATIM
    )

  add_custom_command(
    OUTPUT "${mmu_fw_bin_file}"
    COMMAND "${RECOMMENDED_TOOLCHAIN_BINUTILS}/arm-none-eabi-objcopy" -I ihex -O binary
            "${mmu_fw_adj_hex_file}" "${mmu_fw_bin_file}"
    DEPENDS "${mmu_fw_adj_hex_file}"
    )

  # Parse the FW version - used in the configure_file call
  string(REGEX MATCH "([0-9]+)\.([0-9]+)\.([0-9]+)" result "${mmu_fw_version}")
  if(NOT result)
    message(FATAL_ERROR "Failed to parse MMU FW version ${mmu_fw_version}")
  endif()

  # generate header file with MMU FW version
  configure_file(
    mmu_fw_attached_version.hpp.in "${resources_include_dir}/mmu2/mmu_fw_attached_version.hpp"
    )

  add_resource("${mmu_fw_bin_file}" "/mmu/fw.bin")
endif()

tar_image_build(resources-tarball)
