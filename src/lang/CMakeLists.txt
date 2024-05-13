#
# First, create a target converting .po files to C++ files
#
file(GLOB_RECURSE POFILES ${CMAKE_SOURCE_DIR}/src/lang/po/*/*.po)
add_library(translations INTERFACE)
target_include_directories(translations INTERFACE "${CMAKE_CURRENT_BINARY_DIR}")
add_custom_command(
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/hash_table_buckets.ipp"
         "${CMAKE_CURRENT_BINARY_DIR}/hash_table_buckets_count.ipp"
  COMMAND "${Python3_EXECUTABLE}" "${PROJECT_ROOT_DIR}/utils/translations_and_fonts/lang.py"
          "generate-hash-tables" "${CMAKE_CURRENT_SOURCE_DIR}/po" "${CMAKE_CURRENT_BINARY_DIR}"
  DEPENDS "${PROJECT_ROOT_DIR}/utils/translations_and_fonts/lang.py" ${POFILES}
  COMMENT "Generating C++ files out of .po files."
  VERBATIM
  )
add_custom_target(
  translations-genfiles DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/hash_table_buckets.ipp"
  )
add_custom_command(
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/full-chars.txt"
         "${CMAKE_CURRENT_BINARY_DIR}/standard-chars.txt"
         "${CMAKE_CURRENT_BINARY_DIR}/digits-chars.txt"
  COMMAND "${Python3_EXECUTABLE}" "${PROJECT_ROOT_DIR}/utils/translations_and_fonts/lang.py"
          "generate-required-chars" "${CMAKE_CURRENT_SOURCE_DIR}/po" "${CMAKE_CURRENT_BINARY_DIR}"
  DEPENDS "${PROJECT_ROOT_DIR}/utils/translations_and_fonts/lang.py" ${POFILES}
  COMMENT "Generating used required chars out of .po files."
  VERBATIM
  )
add_custom_target(translations-required-chars DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/full-chars.txt")
add_dependencies(translations translations-genfiles translations-required-chars)

#
# And create a Buddy::Lang library itself
#
add_library(
  lang OBJECT translator.cpp translation_provider_FILE.cpp translation_provider_CPUFLASH.cpp
              gettext_string_hash.cpp format_print_will_end.cpp
  )

target_link_libraries(lang translations sysbase::sysbase)
target_include_directories(lang PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/include")

add_library(Buddy::Lang ALIAS lang)
add_library(Buddy::Lang::Translations ALIAS translations)
