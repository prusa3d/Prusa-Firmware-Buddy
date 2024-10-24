include(QoiGenerator.cmake)
#
# Common Functions
#
set(generate_hash_file_py "${CMAKE_SOURCE_DIR}/utils/resources/generate_hash_file.py")
set(resources_include_dir "${CMAKE_CURRENT_BINARY_DIR}/include")

function(add_resource source_path install_path)
  set(one_value_args IMAGE_NAME)
  cmake_parse_arguments(ARG "" "${one_value_args}" "" ${ARGN})
  if(NOT ARG_IMAGE_NAME)
    set(ARG_IMAGE_NAME resources-image)
  endif()
  lfs_image_add_file(${ARG_IMAGE_NAME} "${source_path}" "${install_path}")
endfunction()

function(add_gzip_resource source_path install_path)
  set(gzip_file_path "${CMAKE_CURRENT_BINARY_DIR}/gzipped${install_path}")
  gzip_file("${source_path}" "${gzip_file_path}")
  add_resource("${gzip_file_path}" "${install_path}" ${ARGN})
endfunction()

function(add_header_with_revision revision_name image_name)
  # generate hash (to a file)
  set(binary_hash_file "${CMAKE_CURRENT_BINARY_DIR}/resources_${revision_name}_hash.bin")
  lfs_image_generate_hash_bin_file(${image_name} "${binary_hash_file}")

  # generate header with the hash
  set(header_hash_file "${resources_include_dir}/resources/revision_${revision_name}.hpp")
  add_custom_command(
    OUTPUT "${header_hash_file}"
    COMMAND
      "${Python3_EXECUTABLE}" "${generate_hash_file_py}" "--header-variable-name" "${revision_name}"
      "--header-namespace-name" "buddy::resources::revision" "${binary_hash_file}"
      "${header_hash_file}"
    DEPENDS "${binary_hash_file}" "${generate_hash_file_py}"
    VERBATIM
    )

  # set dependency on the main target
  add_custom_target(
    resources-header-${revision_name} DEPENDS "${header_hash_file}" qoi_generated_files
    )
  add_dependencies(firmware resources-header-${revision_name})
endfunction()

#
# General
#

target_sources(firmware PUBLIC bootstrap.cpp hash.cpp revision.cpp)
target_include_directories(firmware PUBLIC "${resources_include_dir}")

#
# Standard Resources Image
#

if(PRINTER STREQUAL "MINI")
  # Some revisions of mini has 1MB flash, so limit resource size to remaining size
  add_lfs_image(resources-image BLOCK_SIZE 4096 BLOCK_COUNT 205)
else()
  add_lfs_image(resources-image BLOCK_SIZE 4096 BLOCK_COUNT 512)
endif()

if(${PRINTER} IN_LIST PRINTERS_WITH_ESP_FLASH_TASK)
  set(ESP_PARTS ${CMAKE_CURRENT_BINARY_DIR}/include/esp_parts.gen)
  if(${PRINTER} IN_LIST PRINTERS_WITH_EMBEDDED_ESP32)
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

if(PUPPY_FLASH_FW)
  if(HAS_DWARF)
    add_resource("${DWARF_BINARY_PATH}" "/puppies/fw-dwarf.bin")
  endif()
  if(HAS_MODULARBED)
    add_resource("${MODULARBED_BINARY_PATH}" "/puppies/fw-modularbed.bin")
  endif()
  if(HAS_XBUDDY_EXTENSION)
    add_resource("${XBUDDY_EXTENSION_BINARY_PATH}" "/puppies/fw-xbuddy-extension.bin")
  endif()
endif()

# /web directory
add_gzip_resource("web/favicon.ico" "/web/favicon.ico")
add_gzip_resource("web/index.html" "/web/index.html")
add_gzip_resource("web/main.3fd7d8a85463914cd803.js" "/web/main.3fd7d8a85463914cd803.js")
add_gzip_resource("web/main.71c989a4a290e5c10892.css" "/web/main.71c989a4a290e5c10892.css")

# add languages
if(${TRANSLATIONS_IN_EXTFLASH})
  foreach(LANG ${LANGUAGES_AVAILABLE})
    if(${ENABLE_TRANSLATION_${LANG}})
      string(TOLOWER ${LANG} LANG)
      add_resource(
        "${CMAKE_SOURCE_DIR}/src/lang/po/${LANG}/Prusa-Firmware-Buddy_${LANG}.mo"
        "/lang/${LANG}.mo"
        )
    endif()
  endforeach()
endif()

# non-packed images, set the list of images to process
set(NONPACKED_PNG_SOURCE_DIR "${CMAKE_SOURCE_DIR}/src/resources")

# convert non-packed PNG->QOI, do all of them, even though not all will be used
set(NONPACKED_PNGS
    "pre_bootstrap_pngs/marlin_logo_79x61.png" "pre_bootstrap_pngs/prusa_mini_splash_207x47.png"
    "pre_bootstrap_pngs/prusa_ix_splash_158x52.png" "pre_bootstrap_pngs/prusa_xl_splash_172x52.png"
    )

foreach(NONPACKED_PNG ${NONPACKED_PNGS})
  get_filename_component(NONPACKED_NAME ${NONPACKED_PNG} NAME_WE)
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${NONPACKED_NAME}.qoi
    DEPENDS ${NONPACKED_PNG}
    COMMAND
      "${Python3_EXECUTABLE}" "${qoi_generator_py}" "${NONPACKED_PNG_SOURCE_DIR}/${NONPACKED_PNG}"
      "${CMAKE_CURRENT_BINARY_DIR}/nonpacked.res"
      "${CMAKE_CURRENT_BINARY_DIR}/${NONPACKED_NAME}.qoi"
    )
endforeach()

add_resource("${CMAKE_CURRENT_BINARY_DIR}/marlin_logo_79x61.qoi" "marlin_logo_79x61.qoi")
if(PRINTER STREQUAL "MINI")
  add_resource("${CMAKE_CURRENT_BINARY_DIR}/prusa_mini_splash_207x47.qoi" "printer_logo.qoi")
endif()

# Quite OK images
add_resource("${qoi_data_file}" "qoi.data")

# generate header with hash
add_header_with_revision(standard resources-image)

#
# Bootloader Resources Image
#
if(BOOTLOADER_UPDATE)

  add_lfs_image(resources-bootloader-image BLOCK_SIZE 4096 BLOCK_COUNT 64)

  get_dependency_directory("bootloader-${BOOTLOADER_VARIANT}" bootloader_dir)
  get_dependency_version("bootloader-${BOOTLOADER_VARIANT}" bootloader_version_str)
  set(bootloader_bin "${bootloader_dir}/bootloader.bin")

  # add the bootloader.bin to the image
  add_resource("${bootloader_bin}" "/bootloader.bin" IMAGE_NAME resources-bootloader-image)

  # generate header with hash
  add_header_with_revision(bootloader resources-bootloader-image)

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
