set(FONT_PY "${CMAKE_CURRENT_SOURCE_DIR}/font.py")
set(FONTS_GEN "${CMAKE_CURRENT_BINARY_DIR}/fonts.gen")

add_library(font_data STATIC font_data.cpp)
target_link_libraries(font_data PRIVATE bsod)
target_include_directories(font_data PUBLIC include)
target_include_directories(font_data PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

add_custom_command(
  OUTPUT "${FONTS_GEN}"
  COMMAND "${Python3_EXECUTABLE}" "${FONT_PY}" --po-dir "${PROJECT_ROOT_DIR}/src/lang/po" --png-dir
          "${CMAKE_CURRENT_SOURCE_DIR}/png" --output "${FONTS_GEN}" --depfile "${FONTS_GEN}.d"
  DEPENDS "${FONT_PY}"
  DEPFILE "${FONTS_GEN}.d"
  VERBATIM
  )

target_sources(font_data PRIVATE "${FONTS_GEN}")
