file(STRINGS ${CMAKE_CURRENT_LIST_DIR}/../pyproject.toml _tmp REGEX "version")
string(REGEX REPLACE "version = \"([0-9]+\\.[0-9]+\\.[0-9]+)\"" "\\1" LibBGCode_VERSION ${_tmp} )
