# First, create a target converting .po files to txt files easily parseable by tests
file(GLOB_RECURSE POFILES ${CMAKE_SOURCE_DIR}/src/lang/po/*/*.po)
add_custom_command(
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/keys.txt"
  COMMAND "${Python3_EXECUTABLE}" "${PROJECT_ROOT_DIR}/utils/translations_and_fonts/lang.py"
          "dump-pofiles" "${CMAKE_SOURCE_DIR}/src/lang/po" "${CMAKE_CURRENT_BINARY_DIR}"
  DEPENDS ${POFILES}
  COMMENT "Generating text files out of .po files."
  VERBATIM
  )
add_custom_target(translations_pofiles_dump DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/keys.txt")

# define the test executable
add_executable(
  translator_tests
  ${CMAKE_SOURCE_DIR}/src/lang/translator.cpp
  ${CMAKE_SOURCE_DIR}/src/lang/translation_provider_CPUFLASH.cpp
  ${CMAKE_SOURCE_DIR}/src/lang/translation_provider_FILE.cpp
  ${CMAKE_SOURCE_DIR}/tests/unit/test_main.cpp
  ${CMAKE_SOURCE_DIR}/src/lang/gettext_string_hash.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/providerCPUFLASH.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/hash.cpp
  # we need to add mo file generation to test files
  ${CMAKE_CURRENT_SOURCE_DIR}/providerFILE.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/provider.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/fnt-indices.cpp
  ${CMAKE_SOURCE_DIR}/tests/stubs/strlcpy.c
  )

add_dependencies(translator_tests translations_pofiles_dump)

# target_link_libraries(translator_tests Buddy::Lang)

target_link_libraries(translator_tests Buddy::Lang::Translations)

# define required search paths
target_include_directories(
  translator_tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src/lang
                          ${CMAKE_CURRENT_BINARY_DIR}
  )

# add files for testing FILE provider
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/MOfilesTesting/keys.txt ${CMAKE_CURRENT_BINARY_DIR}/MO/keys.txt
  COPYONLY
  )
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/MOfilesTesting/cs.txt ${CMAKE_CURRENT_BINARY_DIR}/MO/cs.txt COPYONLY
  )
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/MOfilesTesting/cs.mo ${CMAKE_CURRENT_BINARY_DIR}/MO/cs.mo COPYONLY
  )
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/MOfilesTesting/de.txt ${CMAKE_CURRENT_BINARY_DIR}/MO/de.txt COPYONLY
  )
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/MOfilesTesting/de.mo ${CMAKE_CURRENT_BINARY_DIR}/MO/de.mo COPYONLY
  )
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/MOfilesTesting/en.txt ${CMAKE_CURRENT_BINARY_DIR}/MO/en.txt COPYONLY
  )
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/MOfilesTesting/en.mo ${CMAKE_CURRENT_BINARY_DIR}/MO/en.mo COPYONLY
  )
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/MOfilesTesting/es.txt ${CMAKE_CURRENT_BINARY_DIR}/MO/es.txt COPYONLY
  )
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/MOfilesTesting/es.mo ${CMAKE_CURRENT_BINARY_DIR}/MO/es.mo COPYONLY
  )
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/MOfilesTesting/fr.mo ${CMAKE_CURRENT_BINARY_DIR}/MO/fr.mo COPYONLY
  )
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/MOfilesTesting/fr.txt ${CMAKE_CURRENT_BINARY_DIR}/MO/fr.txt COPYONLY
  )
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/MOfilesTesting/it.mo ${CMAKE_CURRENT_BINARY_DIR}/MO/it.mo COPYONLY
  )
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/MOfilesTesting/it.txt ${CMAKE_CURRENT_BINARY_DIR}/MO/it.txt COPYONLY
  )
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/MOfilesTesting/pl.mo ${CMAKE_CURRENT_BINARY_DIR}/MO/pl.mo COPYONLY
  )
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/MOfilesTesting/pl.txt ${CMAKE_CURRENT_BINARY_DIR}/MO/pl.txt COPYONLY
  )

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/MOfilesTesting/magic.mo ${CMAKE_CURRENT_BINARY_DIR}/MO/magic.mo
  COPYONLY
  )
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/MOfilesTesting/short.mo ${CMAKE_CURRENT_BINARY_DIR}/MO/short.mo
  COPYONLY
  )
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/MOfilesTesting/bigEnd.mo ${CMAKE_CURRENT_BINARY_DIR}/MO/bigEnd.mo
  COPYONLY
  )

# define custom macros
target_compile_definitions(translator_tests PUBLIC TRANSLATIONS_UNITTEST)

# tell build system about the test case
add_catch_test(translator_tests)
