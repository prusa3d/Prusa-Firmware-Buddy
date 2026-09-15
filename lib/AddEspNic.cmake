if(HAS_EMBEDDED_ESP32)
  set(variant esp32)
  get_dependency_directory("esp-idf" sdk_dir)
else()
  set(variant esp8266)
  get_dependency_directory("esp8266-rtos-sdk" sdk_dir)
endif()

get_filename_component(sdk_dir "${sdk_dir}" REALPATH)

set(project_dir "${CMAKE_CURRENT_SOURCE_DIR}/esp-nic")
set(build_dir "${CMAKE_BINARY_DIR}/${variant}-nic-build")
set(images "${build_dir}/uart_wifi.bin" "${build_dir}/bootloader/bootloader.bin"
           "${build_dir}/partition_table/partition-table.bin"
    )

file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${project_dir}/main/*")
list(APPEND sources "${project_dir}/CMakeLists.txt" "${project_dir}/${variant}.sdkconfig"
     "${project_dir}/build.py"
     )
# dependency on SDK version is defined implicitly by sdk_dir variable in add_custom_command

file(STRINGS "${sdk_dir}/missing-components.txt" missing_components)

get_filename_component(cmake_bin_dir "${CMAKE_COMMAND}" DIRECTORY)
get_filename_component(ninja_bin_dir "${CMAKE_MAKE_PROGRAM}" DIRECTORY)
get_filename_component(python_bin_dir "${Python3_EXECUTABLE}" DIRECTORY)

# cmake-format: off
add_custom_command(
  OUTPUT ${images}
  COMMAND
    ${CMAKE_COMMAND} -E env "IDF_PATH=${sdk_dir}" "IDF_TOOLS_PATH=${sdk_dir}/.idf-tools"
    "EXCLUDE_COMPONENTS=${missing_components}"
    "PATH=${cmake_bin_dir}:${ninja_bin_dir}:${python_bin_dir}:$ENV{PATH}" "${Python3_EXECUTABLE}"
    "${project_dir}/build.py" ${variant} --build-dir "${build_dir}"
  DEPENDS ${sources}
  VERBATIM
  )
# cmake-format: on

execute_process(COMMAND "${Python3_EXECUTABLE}" "${project_dir}/gen_clangd.py")

# images are consumed from another directory
add_custom_target(esp-firmware DEPENDS ${images})
set(ESP_NIC_IMAGES
    ${images}
    PARENT_SCOPE
    )
