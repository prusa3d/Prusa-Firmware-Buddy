set(generate_hash_file_py "${CMAKE_SOURCE_DIR}/utils/resources/generate_hash_file.py")

add_lfs_image(resources-image BLOCK_SIZE 256 BLOCK_COUNT 3000)

function(add_resource source_path install_path)
  lfs_image_add_file(resources-image "${source_path}" "${install_path}")
endfunction()

function(add_gzip_resource source_path install_path)
  set(gzip_file_path "${CMAKE_CURRENT_BINARY_DIR}/gzipped${install_path}")
  gzip_file("${source_path}" "${gzip_file_path}")
  add_resource("${gzip_file_path}" "${install_path}")
endfunction()

add_resource("esp/uart_wifi.bin" "/esp/uart_wifi.bin")
add_resource("esp/bootloader.bin" "/esp/bootloader.bin")
add_resource("esp/partition-table.bin" "/esp/partition-table.bin")

add_gzip_resource(
  "web/e5ae0f11eb9a619ec903488e77be89b8.svg" "/web/e5ae0f11eb9a619ec903488e77be89b8.svg"
  )
add_gzip_resource("web/favicon.ico" "/web/favicon.ico")
add_gzip_resource("web/index.html" "/web/index.html")
add_gzip_resource("web/main.eb501c83e567a43e721c.js" "/web/main.eb501c83e567a43e721c.js")
add_gzip_resource("web/main.2db251e25b37de8a6876.css" "/web/main.2db251e25b37de8a6876.css")

# generate binary file with hash TODO: Investigate PARENT_SCOPE
set(RESOURCES_BINARY_HASH_FILE "${CMAKE_CURRENT_BINARY_DIR}/resources_hash.bin")
set(RESOURCES_BINARY_HASH_FILE
    "${CMAKE_CURRENT_BINARY_DIR}/resources_hash.bin"
    PARENT_SCOPE
    )
lfs_image_generate_hash_bin_file(resources-image "${RESOURCES_BINARY_HASH_FILE}")

# generate header with hash
set(RESOURCES_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/include")
set(RESOURCES_HEADER_HASH_FILE "${RESOURCES_INCLUDE_DIR}/resources/required_revision.hpp")
add_custom_command(
  OUTPUT "${RESOURCES_HEADER_HASH_FILE}"
  COMMAND
    "${Python3_EXECUTABLE}" "${generate_hash_file_py}" "--header-variable-name" "required_revision"
    "--header-namespace-name" "buddy::resources::revision" "${RESOURCES_BINARY_HASH_FILE}"
    "${RESOURCES_HEADER_HASH_FILE}"
  DEPENDS "${RESOURCES_BINARY_HASH_FILE}" "${generate_hash_file_py}"
  VERBATIM
  )

add_custom_target(
  resources-hash-files DEPENDS "${RESOURCES_BINARY_HASH_FILE}" "${RESOURCES_HEADER_HASH_FILE}"
  )

add_dependencies(firmware resources-hash-files)

target_sources(firmware PUBLIC bootstrap.cpp revision.cpp hash.cpp)

target_include_directories(firmware PUBLIC "${RESOURCES_INCLUDE_DIR}")
