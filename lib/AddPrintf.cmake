add_library(printf printf/printf.c)
target_include_directories(printf PUBLIC printf)
add_library(printf::printf ALIAS printf)
