function(create_generate_hashes_target)
  set(options)
  set(one_value_args TARGET_NAME OUTPUT_FILE_PATH GENERATOR_SCRIPT_PATH)
  set(multi_value_args INPUT_FILES)
  cmake_parse_arguments(
    GENERATE_HASHES "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN}
    )

  add_custom_command(
    OUTPUT "${GENERATE_HASHES_OUTPUT_FILE_PATH}"
    COMMAND "${Python3_EXECUTABLE}" "${GENERATE_HASHES_GENERATOR_SCRIPT_PATH}"
            "${GENERATE_HASHES_OUTPUT_FILE_PATH}" "${GENERATE_HASHES_INPUT_FILES}"
    DEPENDS "${GENERATE_HASHES_GENERATOR_SCRIPT_PATH}" "${GENERATE_HASHES_INPUT_FILES}"
    VERBATIM
    )

  add_custom_target("${GENERATE_HASHES_TARGET_NAME}" DEPENDS "${GENERATE_HASHES_OUTPUT_FILE_PATH}")
endfunction()
