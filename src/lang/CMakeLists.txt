#
# First, create a target converting .po files to C++ files
#

add_library(translations INTERFACE)
target_include_directories(translations INTERFACE "${CMAKE_CURRENT_BINARY_DIR}")
add_custom_command(
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/hash_table_buckets.ipp"
         "${CMAKE_CURRENT_BINARY_DIR}/hash_table_buckets_count.ipp"
  COMMAND "${Python3_EXECUTABLE}" "${PROJECT_ROOT_DIR}/utils/lang.py" "generate-hash-tables"
          "${CMAKE_CURRENT_SOURCE_DIR}/po" "${CMAKE_CURRENT_BINARY_DIR}"
  COMMENT "Generating C++ files out of .po files."
  VERBATIM
  )
add_custom_target(
  translations-genfiles DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/hash_table_buckets.ipp"
  )
add_custom_command(
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/non-ascii-chars.txt"
         "${CMAKE_CURRENT_BINARY_DIR}/non-ascii-chars.raw"
  COMMAND "${Python3_EXECUTABLE}" "${PROJECT_ROOT_DIR}/utils/lang.py" "generate-nonascii-chars"
          "${CMAKE_CURRENT_SOURCE_DIR}/po" "${CMAKE_CURRENT_BINARY_DIR}"
  COMMENT "Generating used non-ascii chars out of .po files."
  VERBATIM
  )
add_custom_target(
  translations-nonascii-chars DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/non-ascii-chars.txt"
  )
add_dependencies(translations translations-genfiles translations-nonascii-chars)

#
# And create a Buddy::Lang library itself
#

add_library(
  lang OBJECT translator.cpp translation_provider_CPUFLASH.cpp unaccent.cpp
              format_print_will_end.cpp translation_provider_FILE.cpp gettext_string_hash.cpp
  )
target_link_libraries(lang translations sysbase::sysbase)
target_include_directories(lang PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(Buddy::Lang ALIAS lang)
add_library(Buddy::Lang::Translations ALIAS translations)
