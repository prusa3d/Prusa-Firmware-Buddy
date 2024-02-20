function(add_generated_error_codes_header file printer_code printer_id is_mmu)
  if(is_mmu)
    set(suffix "_mmu")
  else()
    set(suffix "")
  endif()

  add_library(error_codes${suffix} INTERFACE)
  target_include_directories(
    error_codes${suffix} INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/error_codes"
    )

  set(generate_error_codes_py
      "${CMAKE_CURRENT_SOURCE_DIR}/Prusa-Error-Codes/generate_buddy_headers.py"
      )

  set(error_codes_header "${CMAKE_CURRENT_BINARY_DIR}/error_codes/error_codes${suffix}.hpp")
  add_custom_command(
    OUTPUT "${error_codes_header}"
    COMMAND "${Python3_EXECUTABLE}" "${generate_error_codes_py}" "${file}" "${error_codes_header}"
            "${printer_id}" "${printer_code}" $<$<BOOL:${is_mmu}>:--mmu>
    DEPENDS "${file}" "${generate_error_codes_py}"
    VERBATIM COMMAND_EXPAND_LISTS
    )

  set(error_list_header "${CMAKE_CURRENT_BINARY_DIR}/error_codes/error_list${suffix}.hpp")
  add_custom_command(
    OUTPUT "${error_list_header}"
    COMMAND
      "${Python3_EXECUTABLE}" "${generate_error_codes_py}" "${file}" "${error_list_header}"
      "${printer_id}" "${printer_code}" $<$<BOOL:${is_mmu}>:--mmu> --list --include
      "error_codes${suffix}.hpp"
    DEPENDS "${file}" "${generate_error_codes_py}"
    VERBATIM COMMAND_EXPAND_LISTS
    )

  add_custom_target(error_codes${suffix}_tgt DEPENDS "${error_codes_header}" "${error_list_header}")
  add_dependencies(error_codes${suffix} error_codes${suffix}_tgt)
  target_include_directories(error_codes${suffix} INTERFACE "${error_codes_dir}/include")
endfunction()

set(error_codes_dir "${CMAKE_CURRENT_SOURCE_DIR}/Prusa-Error-Codes")

if(PRINTER STREQUAL "XL_DEV_KIT")
  # use XL error codes for XL_DEV_KIT
  add_generated_error_codes_header("${error_codes_dir}/yaml/buddy-error-codes.yaml" 17 XL FALSE)
else()
  add_generated_error_codes_header(
    "${error_codes_dir}/yaml/buddy-error-codes.yaml" ${PRINTER_CODE} ${PRINTER} FALSE
    )
endif()

# TODO temporarily build the mmu header, not easy to separate the mmu code, needs refactor
# if(HAS_MMU2)
add_generated_error_codes_header("${error_codes_dir}/yaml/mmu-error-codes.yaml" 04 MMU TRUE)
# endif()
