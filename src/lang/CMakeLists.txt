add_library(
  lang OBJECT translator.cpp translation_provider_FILE.cpp gettext_string_hash.cpp
              format_print_will_end.cpp string_view_utf8.cpp
  )

target_link_libraries(lang sysbase::sysbase bsod)
target_include_directories(lang PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/include")

add_library(Buddy::Lang ALIAS lang)
