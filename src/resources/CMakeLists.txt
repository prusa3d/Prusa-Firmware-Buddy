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
  add_custom_target(resources-header-${revision_name} DEPENDS "${header_hash_file}")
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

add_lfs_image(resources-image BLOCK_SIZE 4096 BLOCK_COUNT 256)

# /esp directory
add_resource("esp/uart_wifi.bin" "/esp/uart_wifi.bin")
add_resource("esp/bootloader.bin" "/esp/bootloader.bin")
add_resource("esp/partition-table.bin" "/esp/partition-table.bin")

# /web directory
add_gzip_resource("web/favicon.ico" "/web/favicon.ico")
add_gzip_resource("web/index.html" "/web/index.html")
add_gzip_resource("web/main.2db555ec7e6e55de710c.js" "/web/main.2db555ec7e6e55de710c.js")
add_gzip_resource("web/main.112d6e1a4344fea4f149.css" "/web/main.112d6e1a4344fea4f149.css")

# /png directory
if(PRINTER STREQUAL "MINI")
  add_resource("png/prusa_mini_logo_153x40.png" "/png/prusa_printer_logo.png")
  add_resource("png/prusa_mini_splash_207x47.png" "/png/prusa_printer_splash.png")
endif()

add_resource("png/pepa_64px.png" "/png/pepa_64px.png")
add_resource("png/pepa_140px.png" "/png/pepa_140px.png")
add_resource("png/pepa_original_140px.png" "/png/pepa_original_140px.png")
add_resource("png/serial_printing.png" "/png/serial_printing.png")
add_resource("png/home_shape_16px.png" "/png/home_shape_16px.png")
add_resource("png/print_16px.png" "/png/print_16px.png")
add_resource("png/hand_154x65.png" "/png/hand_154x65.png")
add_resource("png/nozzle_34x32.png" "/png/nozzle_34x32.png")
add_resource("png/usb_16px.png" "/png/usb_16px.png")
add_resource("png/info_16px.png" "/png/info_16px.png")
add_resource("png/warning_16px.png" "/png/warning_16px.png")
add_resource("png/question_16px.png" "/png/question_16px.png")
add_resource("png/arrow_up_16px.png" "/png/arrow_up_16px.png")
add_resource("png/arrow_down_16px.png" "/png/arrow_down_16px.png")
add_resource("png/arrow_left_16px.png" "/png/arrow_left_16px.png")
add_resource("png/hand_qr.png" "/png/hand_qr.png")
add_resource("png/turn_knob.png" "/png/turn_knob.png")

add_resource("png/marlin_logo_76x61.png" "/png/marlin_logo_76x61.png")

add_resource("png/arrow_right_10x16.png" "/png/arrow_right_10x16.png")

add_resource("png/arrow_up_12x12.png" "/png/arrow_up_12x12.png")
add_resource("png/arrow_up_orange_12x12.png" "/png/arrow_up_orange_12x12.png")
add_resource("png/arrow_down_12x12.png" "/png/arrow_down_12x12.png")
add_resource("png/arrow_down_orange_12x12.png" "/png/arrow_down_orange_12x12.png")

add_resource("png/flag_cs_16x11.png" "/png/flag_cs_16x11.png")
add_resource("png/flag_de_16x11.png" "/png/flag_de_16x11.png")
add_resource("png/flag_en_16x11.png" "/png/flag_en_16x11.png")
add_resource("png/flag_es_16x11.png" "/png/flag_es_16x11.png")
add_resource("png/flag_fr_16x11.png" "/png/flag_fr_16x11.png")
add_resource("png/flag_it_16x11.png" "/png/flag_it_16x11.png")
add_resource("png/flag_pl_16x11.png" "/png/flag_pl_16x11.png")

add_resource("png/sheets_profile_16x14.png" "/png/sheets_profile_16x14.png")

add_resource("png/spinner0_16x16.png" "/png/spinner0_16x16.png")
add_resource("png/spinner1_16x16.png" "/png/spinner1_16x16.png")
add_resource("png/spinner2_16x16.png" "/png/spinner2_16x16.png")
add_resource("png/spinner3_16x16.png" "/png/spinner3_16x16.png")
add_resource("png/nozzle_16px.png" "/png/nozzle_16px.png")
add_resource("png/heatbed_16px.png" "/png/heatbed_16px.png")
add_resource("png/speed_16px.png" "/png/speed_16px.png")
add_resource("png/spool_16px.png" "/png/spool_16px.png")
add_resource("png/z_axis_16px.png" "/png/z_axis_16px.png")
add_resource("png/home_shape_16px.png" "/png/home_shape_16px.png")
add_resource("png/print_16px.png" "/png/print_16px.png")
add_resource("png/x_axis_16x16.png" "/png/x_axis_16x16.png")
add_resource("png/y_axis_16x16.png" "/png/y_axis_16x16.png")
add_resource("png/z_axis_16x16.png" "/png/z_axis_16x16.png")
add_resource("png/turbine_16x16.png" "/png/turbine_16x16.png")
add_resource("png/fan_16x16.png" "/png/fan_16x16.png")
add_resource("png/selftest_16x16.png" "/png/selftest_16x16.png")
add_resource("png/wizard_16x16.png" "/png/wizard_16x16.png")
add_resource("png/filament_sensor_17x16.png" "/png/filament_sensor_17x16.png")
add_resource("png/wizard_16x16.png" "/png/nozzle_empty_16px.png") # TODO missing resource
add_resource("png/folder_full_16px.png" "/png/folder_full_16px.png")
add_resource("png/wifi_16x16.png" "/png/wifi_16x16.png")
add_resource("png/lan_16x16.png" "/png/lan_16x16.png")
add_resource("png/home_full_16x16.png" "/png/home_full_16x16.png")
add_resource("png/folder_up_16x16.png" "/png/folder_up_16x16.png")
add_resource("png/error_16x16.png" "/png/error_16x16.png")

add_resource("png/dash_18x18.png" "/png/dash_18x18.png")
add_resource("png/ok_color_18x18.png" "/png/ok_color_18x18.png")
add_resource("png/nok_color_18x18.png" "/png/nok_color_18x18.png")

add_resource("png/hourglass_26x39.png" "/png/hourglass_26x39.png")

add_resource("png/back_32x32.png" "/png/back_32x32.png")

add_resource("png/question_48x48.png" "/png/question_48x48.png")
add_resource("png/warning_48px.png" "/png/warning_48px.png")
add_resource("png/error_white_48px.png" "/png/error_white_48px.png")
add_resource("png/info_48px.png" "/png/info_48px.png")
add_resource("png/nozzle_shape_48px.png" "/png/nozzle_shape_48px.png")
add_resource("png/disconnect_48x48.png" "/png/disconnect_48x48.png")
add_resource("png/resume_48x48.png" "/png/resume_48x48.png")
add_resource("png/reprint_48x48.png" "/png/reprint_48x48.png")
add_resource("png/fan_error_48x48.png" "/png/fan_error_48x48.png")
add_resource("png/exposure_times_48x48.png" "/png/exposure_times_48x48.png")
add_resource("png/usb_error_48x48.png" "/png/usb_error_48x48.png")

add_resource("png/print_58x58.png" "/png/print_58x58.png")
add_resource("png/preheat_58x58.png" "/png/preheat_58x58.png")
add_resource("png/spool_58x58.png" "/png/spool_58x58.png")
add_resource("png/calibrate_58x58.png" "/png/calibrate_58x58.png")
add_resource("png/settings_58x58.png" "/png/settings_58x58.png")
add_resource("png/info_58x58.png" "/png/info_58x58.png")
add_resource("png/pause_58x58.png" "/png/pause_58x58.png")
add_resource("png/stop_58x58.png" "/png/stop_58x58.png")
add_resource("png/stop_58x58.png" "/png/home_58x58.png") # TODO missing resource

add_resource("png/wifi_64x64.png" "/png/wifi_64x64.png")

add_resource("png/nozzle_crash_101x64.png" "/png/nozzle_crash_101x64.png")

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
