option(PNG2FONT_ENABLE "Enable building of png2font" ON)

if(PNG2FONT_ENABLE)
  add_subdirectory(translations_and_fonts/png2font)
endif()
