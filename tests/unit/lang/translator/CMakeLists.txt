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
  ${CMAKE_SOURCE_DIR}/src/lang/string_view_utf8.cpp
  ${CMAKE_SOURCE_DIR}/src/lang/translator.cpp
  ${CMAKE_SOURCE_DIR}/src/lang/translation_provider_FILE.cpp
  ${CMAKE_SOURCE_DIR}/tests/unit/test_main.cpp
  ${CMAKE_SOURCE_DIR}/src/lang/gettext_string_hash.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/hash.cpp
  # we need to add mo file generation to test files
  ${CMAKE_CURRENT_SOURCE_DIR}/providerFILE.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/provider.cpp
  ${CMAKE_SOURCE_DIR}/tests/stubs/strlcpy.c
  )

add_dependencies(translator_tests translations_pofiles_dump)

target_link_libraries(translator_tests font_data)

# define required search paths
target_include_directories(
  translator_tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src/lang
                          ${CMAKE_CURRENT_BINARY_DIR}
  )

# add files for testing FILE provider
foreach(LANG ${LANGUAGES_AVAILABLE})
  string(TOLOWER ${LANG} LANG)
  set(PO_FILE "${CMAKE_SOURCE_DIR}/src/lang/po/${LANG}/Prusa-Firmware-Buddy_${LANG}.po")
  set(MO_FILE "${CMAKE_CURRENT_BINARY_DIR}/MO/${LANG}.mo")
  add_custom_command(
    OUTPUT ${MO_FILE}
    COMMAND msgfmt ${PO_FILE} -o ${MO_FILE}
    DEPENDS ${PO_FILE}
    COMMENT "Generating ${MO_FILE} from ${PO_FILE}"
    VERBATIM
    )
  add_custom_target(generate_mo_${LANG} ALL DEPENDS ${MO_FILE})
  add_dependencies(translator_tests generate_mo_${LANG})
endforeach()
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

# tell build system about the test case
add_catch_test(translator_tests)
