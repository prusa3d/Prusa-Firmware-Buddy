#
# TEXT FIT TESTS
#

# Common dependencies
link_libraries(SG14 buddy_utils font_data)

# Extract texts from error yaml
file(GLOB ERROR_TEXT_EXTRACTOR ${PROJECT_ROOT_DIR}/utils/extract_error_texts.py)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/errors.hpp
  DEPENDS ${ERROR_TEXT_EXTRACTOR}
          ${PROJECT_ROOT_DIR}/lib/Prusa-Error-Codes/yaml/buddy-error-codes.yaml
          ${PROJECT_ROOT_DIR}/lib/Prusa-Error-Codes/yaml/mmu-error-codes.yaml
  COMMAND
    ${Python3_EXECUTABLE} ${ERROR_TEXT_EXTRACTOR}
    ${PROJECT_ROOT_DIR}/lib/Prusa-Error-Codes/yaml/buddy-error-codes.yaml
    ${PROJECT_ROOT_DIR}/lib/Prusa-Error-Codes/yaml/mmu-error-codes.yaml
  COMMENT "Extracting error texts for unit testing"
  )

add_custom_target(error_texts_extraction DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/errors.hpp)

# Generate MO files read by the FILE translation provider at test runtime
add_custom_target(text_fit_mo_files)
foreach(
  LANG
  cs
  de
  es
  fr
  it
  ja
  pl
  uk
  )
  set(PO_FILE "${CMAKE_SOURCE_DIR}/src/lang/po/${LANG}/Prusa-Firmware-Buddy_${LANG}.po")
  set(MO_FILE "${CMAKE_CURRENT_BINARY_DIR}/MO/${LANG}.mo")
  add_custom_command(
    OUTPUT ${MO_FILE}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/MO
    COMMAND msgfmt ${PO_FILE} -o ${MO_FILE}
    DEPENDS ${PO_FILE}
    COMMENT "Generating ${MO_FILE} from ${PO_FILE}"
    VERBATIM
    )
  add_custom_target(text_fit_generate_mo_${LANG} DEPENDS ${MO_FILE})
  add_dependencies(text_fit_mo_files text_fit_generate_mo_${LANG})
endforeach()

# Mini display text fit test
add_executable(
  text_fit_tests_mini
  ${CMAKE_CURRENT_SOURCE_DIR}/mini_display_tests.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/text_fit_test.cpp
  ${CMAKE_SOURCE_DIR}/src/common/str_utils.cpp
  ${CMAKE_SOURCE_DIR}/src/common/utils/string_builder.cpp
  ${CMAKE_SOURCE_DIR}/src/lang/string_view_utf8.cpp
  ${CMAKE_SOURCE_DIR}/src/gui/fonts.cpp
  ${CMAKE_SOURCE_DIR}/src/guiapi/src/Rect16.cpp
  ${CMAKE_SOURCE_DIR}/src/lang/translator.cpp
  ${CMAKE_SOURCE_DIR}/src/lang/translation_provider_FILE.cpp
  ${CMAKE_SOURCE_DIR}/src/lang/gettext_string_hash.cpp
  )

target_include_directories(
  text_fit_tests_mini
  PRIVATE .
          ${CMAKE_SOURCE_DIR}/src
          ${CMAKE_SOURCE_DIR}/src/common
          ${CMAKE_SOURCE_DIR}/src/common/utils
          ${CMAKE_SOURCE_DIR}/src/lang
          ${CMAKE_SOURCE_DIR}/src/gui
          ${CMAKE_SOURCE_DIR}/src/gui/dialogs
          ${CMAKE_SOURCE_DIR}/src/guiapi/include
          ${PROJECT_ROOT_DIR}/tests/unit/lang/translator
          # Have to be before ${CMAKE_BINARY_DIR}/include otherwise translations options will be not
          # overwritten
          ${CMAKE_SOURCE_DIR}/include
          ${CMAKE_BINARY_DIR}/include
          ${CMAKE_CURRENT_SOURCE_DIR}
          ${CMAKE_CURRENT_BINARY_DIR}
  )

target_compile_definitions(text_fit_tests_mini PUBLIC BOARD=BOARD_BUDDY)

add_dependencies(text_fit_tests_mini text_fit_mo_files)
add_dependencies(text_fit_tests_mini error_texts_extraction)

add_catch_test(text_fit_tests_mini)

# Large display text fit test
add_executable(
  text_fit_tests_large
  ${CMAKE_CURRENT_SOURCE_DIR}/large_display_tests.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/text_fit_test.cpp
  ${CMAKE_SOURCE_DIR}/src/common/str_utils.cpp
  ${CMAKE_SOURCE_DIR}/src/common/utils/string_builder.cpp
  ${CMAKE_SOURCE_DIR}/src/lang/string_view_utf8.cpp
  ${CMAKE_SOURCE_DIR}/src/gui/fonts.cpp
  ${CMAKE_SOURCE_DIR}/src/guiapi/src/Rect16.cpp
  ${CMAKE_SOURCE_DIR}/src/lang/translator.cpp
  ${CMAKE_SOURCE_DIR}/src/lang/translation_provider_FILE.cpp
  ${CMAKE_SOURCE_DIR}/src/lang/gettext_string_hash.cpp
  )

target_include_directories(
  text_fit_tests_large
  PRIVATE .
          ${CMAKE_SOURCE_DIR}/src
          ${CMAKE_SOURCE_DIR}/src/common
          ${CMAKE_SOURCE_DIR}/src/common/utils
          ${CMAKE_SOURCE_DIR}/src/lang
          ${CMAKE_SOURCE_DIR}/src/gui
          ${CMAKE_SOURCE_DIR}/src/gui/dialogs
          ${CMAKE_SOURCE_DIR}/src/guiapi/include
          ${CMAKE_SOURCE_DIR}/include
          ${PROJECT_ROOT_DIR}/tests/unit/lang/translator
          # Have to be before ${CMAKE_BINARY_DIR}/include otherwise translations options will be not
          # overwritten
          ${CMAKE_BINARY_DIR}/include
          ${CMAKE_CURRENT_SOURCE_DIR}
          ${CMAKE_CURRENT_BINARY_DIR}
  )

target_compile_definitions(text_fit_tests_large PUBLIC BOARD=BOARD_XBUDDY)

add_dependencies(text_fit_tests_large text_fit_mo_files)
add_dependencies(text_fit_tests_large error_texts_extraction)

add_catch_test(text_fit_tests_large)
