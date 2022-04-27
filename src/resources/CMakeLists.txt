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
add_gzip_resource("web/main.446d4b169b9b725c49ef.js" "/web/main.446d4b169b9b725c49ef.js")
add_gzip_resource("web/main.554eee61c4aa8b635cf4.css" "/web/main.554eee61c4aa8b635cf4.css")

# generate header with hash
set(RESOURCES_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/include")
set(RESOURCES_HEADER_HASH_FILE "${RESOURCES_INCLUDE_DIR}/resources/required_revision.hpp")
lfs_image_generate_hash_header_file(
  resources-image "${RESOURCES_HEADER_HASH_FILE}" "buddy::resources::revision" "required_revision"
  )

# generate binary file with hash TODO: Investigate PARENT_SCOPE
set(RESOURCES_BINARY_HASH_FILE "${CMAKE_CURRENT_BINARY_DIR}/resources_hash.bin")
set(RESOURCES_BINARY_HASH_FILE
    "${CMAKE_CURRENT_BINARY_DIR}/resources_hash.bin"
    PARENT_SCOPE
    )
lfs_image_generate_hash_bin_file(resources-image "${RESOURCES_BINARY_HASH_FILE}")

add_custom_target(
  resources-hash-files DEPENDS "${RESOURCES_BINARY_HASH_FILE}" "${RESOURCES_HEADER_HASH_FILE}"
  )

add_dependencies(firmware resources-hash-files)

target_sources(firmware PUBLIC bootstrap.cpp revision.cpp)

target_include_directories(firmware PUBLIC "${RESOURCES_INCLUDE_DIR}")
