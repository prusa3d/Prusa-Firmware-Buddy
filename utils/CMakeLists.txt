option(BIN2CC_ENABLE "Enable building of bin2cc." ON)
option(PNG2FONT_ENABLE "Enable building of png2font" ON)
option(HEX2DFU_ENABLE "Enable building of hex2dfu" ON)
option(MAKEFSDATA_ENABLE "Enable building of makefsdata" OFF)

if(BIN2CC_ENABLE)
  add_subdirectory(bin2cc)
endif()

if(PNG2FONT_ENABLE)
  add_subdirectory(png2font)
endif()

if(HEX2DFU_ENABLE)
  add_subdirectory(hex2dfu)
endif()

if(MAKEFSDATA_ENABLE)
  add_subdirectory(makefsdata)
endif()
