set(png_source_dir "${CMAKE_SOURCE_DIR}/src/gui/res/png")
set(png_generator_py "${CMAKE_SOURCE_DIR}/utils/png_packer.py")
set(png_data_file
    "${CMAKE_CURRENT_BINARY_DIR}/resources.pngs"
    CACHE INTERNAL ""
    )
set(png_resources_file "${CMAKE_BINARY_DIR}/src/gui/res/png_resources.gen")

add_custom_command(
  OUTPUT "${png_data_file}" "${png_resources_file}"
  COMMAND "${Python3_EXECUTABLE}" "${png_generator_py}" "${png_source_dir}" "${png_resources_file}"
          "${png_data_file}"
  DEPENDS "${png_source_dir}" "${png_generator_py}"
  VERBATIM
  )

add_custom_target(png_generator DEPENDS "${png_data_file}" "${png_resources_file}")

add_dependencies(firmware png_generator)

target_include_directories(firmware PUBLIC "${CMAKE_BINARY_DIR}/src/gui/res")
