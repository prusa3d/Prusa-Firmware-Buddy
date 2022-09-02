set(configuration_store_include_dir "${CMAKE_CURRENT_BINARY_DIR}")
set(configuration_store_structure_file
    ${configuration_store_include_dir}/configuration_store_structure.hpp
    )
set(item_updater_file ${configuration_store_include_dir}/item_updater.cpp)
set(declarations_file ${configuration_store_include_dir}/declarations.cpp)
set(structure_generator_py "${CMAKE_SOURCE_DIR}/utils/configuration_store/structure_generator.py")

function(generate_config_store_structure target source_file)
  add_custom_command(
    OUTPUT "${configuration_store_structure_file}" "${item_updater_file}" "${declarations_file}"
    COMMAND "${Python3_EXECUTABLE}" "${structure_generator_py}" "${source_file}"
            "${configuration_store_structure_file}" "${item_updater_file}" "${declarations_file}"
    DEPENDS "${source_file}" "${structure_generator_py}"
    VERBATIM
    )

  add_custom_target(store-structure_${target} DEPENDS "${configuration_store_structure_file}")

  add_dependencies(${target} store-structure_${target})
  add_dependencies(Marlin store-structure_${target})

  target_sources(${target} PRIVATE "${declarations_file}" "${item_updater_file}")

  target_include_directories(${target} PUBLIC "${configuration_store_include_dir}")
endfunction()
