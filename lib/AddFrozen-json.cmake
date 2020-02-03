add_library(frozen-json frozen-json/frozen.c)

target_include_directories(frozen-json PUBLIC frozen-json)

add_library(frozen-json::frozen-json ALIAS frozen-json)
