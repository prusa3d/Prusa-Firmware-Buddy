option(PNG2FONT_ENABLE "Enable building of png2font" ON)
option(HEX2DFU_ENABLE "Enable building of hex2dfu" ON)
option(MAKEFSDATA_ENABLE "Enable building of makefsdata" OFF)

if(PNG2FONT_ENABLE)
  add_subdirectory(translations_and_fonts/png2font)
endif()

if(HEX2DFU_ENABLE)
  add_subdirectory(hex2dfu)
endif()

if(MAKEFSDATA_ENABLE)
  add_subdirectory(makefsdata)
endif()
