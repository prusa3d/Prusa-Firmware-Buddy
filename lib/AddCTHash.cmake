add_library(cthash INTERFACE)
target_include_directories(cthash INTERFACE cthash/include)

add_library(cthash::cthash ALIAS cthash)
