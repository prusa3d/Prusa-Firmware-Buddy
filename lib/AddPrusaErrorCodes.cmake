function(add_generated_error_codes_header dir is_mmu)
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
    COMMAND "${Python3_EXECUTABLE}" "${generate_error_codes_py}" "${dir}/error-codes.yaml"
            "${error_codes_header}" $<$<BOOL:${is_mmu}>:--mmu>
    DEPENDS "${dir}/error-codes.yaml" "${generate_error_codes_py}"
    VERBATIM COMMAND_EXPAND_LISTS
    )

  add_custom_target(error_codes${suffix}_tgt DEPENDS "${error_codes_header}")
  add_dependencies(error_codes${suffix} error_codes${suffix}_tgt)
  target_include_directories(error_codes${suffix} INTERFACE ${dir})
endfunction()

set(error_codes_dir "${CMAKE_CURRENT_SOURCE_DIR}/Prusa-Error-Codes")

if(EXISTS "${error_codes_dir}/${PRINTER_CODE}_${PRINTER}/error-codes.yaml")
  add_generated_error_codes_header("${error_codes_dir}/${PRINTER_CODE}_${PRINTER}" FALSE)
else()
  # temporary: use MINI codes as a fallback if not present for the printer
  add_generated_error_codes_header("${error_codes_dir}/12_MINI" FALSE)
endif()

if(HAS_MMU2)
  add_generated_error_codes_header("${error_codes_dir}/04_MMU" TRUE)
endif()
