include(ExternalProject)

function(add_puppy_firmware)
  cmake_parse_arguments(ARG "" "PUPPY_NAME;OUT_BINARY_PATH" "" ${ARGN})

  string(TOLOWER "${PRINTER}-${ARG_PUPPY_NAME}_${CMAKE_BUILD_TYPE}_boot" preset)
  set(binary_dir "${CMAKE_BINARY_DIR}/${ARG_PUPPY_NAME}-build")
  set(binary_path "${binary_dir}/firmware.bin")

  # Pass extra arguments to the build command of puppies, sadly the ExternalProject_Add command
  # creates a new CMake environment, so we need to manually pass the variables
  set(extra_args)
  foreach(variable CMAKE_TOOLCHAIN_FILE ARM_TOOLCHAIN_DIR NO_CCACHE DISABLE_LTO)
    if(NOT "${${variable}}" STREQUAL "")
      list(APPEND extra_args "-D${variable}=${${variable}}")
    endif()
  endforeach()

  ExternalProject_Add(
    ${ARG_PUPPY_NAME}_firmware
    SOURCE_DIR "${CMAKE_SOURCE_DIR}"
    BINARY_DIR "${binary_dir}"
    CMAKE_ARGS
      --preset ${preset} -B "${binary_dir}" ${extra_args}
      $<$<BOOL:${CUSTOM_COMPILE_OPTIONS}>:-DCUSTOM_COMPILE_OPTIONS=${CUSTOM_COMPILE_OPTIONS}>
    BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --target firmware
    INSTALL_COMMAND ${CMAKE_COMMAND} -E echo "Skipping install step"
    BUILD_BYPRODUCTS "${binary_path}" "${binary_dir}/firmware"
    STEP_TARGETS build
    USES_TERMINAL_BUILD TRUE
    USES_TERMINAL_INSTALL FALSE
    BUILD_ALWAYS TRUE
    EXCLUDE_FROM_ALL TRUE
    )

  set(${ARG_OUT_BINARY_PATH}
      "${binary_path}"
      PARENT_SCOPE
      )
endfunction()
