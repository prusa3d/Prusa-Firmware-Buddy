#
# First, create a target converting .po files to C++ files
#

add_library(translations INTERFACE)
target_include_directories(translations INTERFACE "${CMAKE_CURRENT_BINARY_DIR}")
add_custom_command(
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/hash_table_buckets.ipp"
  COMMAND "${Python3_EXECUTABLE}" "${PROJECT_ROOT_DIR}/utils/lang.py" "generate-hash-tables"
          "${CMAKE_CURRENT_SOURCE_DIR}/po" "${CMAKE_CURRENT_BINARY_DIR}"
  COMMENT "Generating C++ files out of .po files."
  VERBATIM
  )
add_custom_target(
  translations-genfiles DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/hash_table_buckets.ipp"
  )
add_dependencies(translations translations-genfiles)

#
# And create a Buddy::Lang library itself
#

add_library(
  lang OBJECT translator.cpp translation_provider_CPUFLASH.cpp unaccent.cpp
              format_print_will_end.cpp
  )
target_link_libraries(lang translations)
target_include_directories(lang PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
add_library(Buddy::Lang ALIAS lang)
