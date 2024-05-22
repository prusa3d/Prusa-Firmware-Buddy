define_property(
  TARGET
  PROPERTY LFS_IMAGE_LOCATION
  BRIEF_DOCS "Target location of the .lfs binary image."
  FULL_DOCS "The path to an image file created by a target made by add_lfs_image"
  )

define_property(
  TARGET
  PROPERTY LFS_IMAGE_BLOCK_SIZE
  BRIEF_DOCS "Block size of a littlefs image"
  FULL_DOCS "Block size of a littlefs image"
  )

define_property(
  TARGET
  PROPERTY LFS_IMAGE_BLOCK_COUNT
  BRIEF_DOCS "Block count of a littlefs image"
  FULL_DOCS "Block count of a littlefs image"
  )

set(mklittlefs "${CMAKE_SOURCE_DIR}/utils/mklittlefs.py")

function(add_lfs_image image_name)
  set(one_value_args BLOCK_SIZE BLOCK_COUNT)
  cmake_parse_arguments(arg "" "${one_value_args}" "" ${ARGN})
  set(image_location "${CMAKE_CURRENT_BINARY_DIR}/${image_name}.lfs")
  add_custom_target(
    ${image_name}
    DEPENDS ${image_location}
    COMMENT "Generating littlefs image"
    )
  set_target_properties(${image_name} PROPERTIES LFS_IMAGE_LOCATION ${image_location})
  set_target_properties(${image_name} PROPERTIES LFS_IMAGE_BLOCK_COUNT ${arg_BLOCK_COUNT})
  set_target_properties(${image_name} PROPERTIES LFS_IMAGE_BLOCK_SIZE ${arg_BLOCK_SIZE})
  add_custom_command(
    OUTPUT ${image_location}
    COMMAND "${Python3_EXECUTABLE}" "${mklittlefs}" "--block-size" "${arg_BLOCK_SIZE}"
            "--block-count" "${arg_BLOCK_COUNT}" "create-image" "${image_location}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    VERBATIM
    )
endfunction()

function(lfs_image_add_file image_name file target)
  get_target_property(image_location ${image_name} LFS_IMAGE_LOCATION)
  get_target_property(block_count ${image_name} LFS_IMAGE_BLOCK_COUNT)
  get_target_property(block_size ${image_name} LFS_IMAGE_BLOCK_SIZE)
  add_custom_command(
    OUTPUT ${image_location}
    COMMAND "${Python3_EXECUTABLE}" "${mklittlefs}" "--block-size" "${block_size}" "--block-count"
            "${block_count}" "add-file" "${image_location}" "${file}" "${target}"
    DEPENDS "${file}"
    APPEND VERBATIM
    )
endfunction()

function(lfs_image_generate_hash_bin_file image_name file)
  get_target_property(image_location ${image_name} LFS_IMAGE_LOCATION)
  get_target_property(block_count ${image_name} LFS_IMAGE_BLOCK_COUNT)
  get_target_property(block_size ${image_name} LFS_IMAGE_BLOCK_SIZE)
  add_custom_command(
    OUTPUT "${file}"
    COMMAND "${Python3_EXECUTABLE}" "${mklittlefs}" "--block-size" "${block_size}" "--block-count"
            "${block_count}" "get-content-hash" "${image_location}" "${file}"
    DEPENDS "${image_location}" "${mklittlefs}"
    VERBATIM
    )
endfunction()
