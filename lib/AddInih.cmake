add_library(inih inih/ini.c)

target_include_directories(inih PUBLIC inih)

add_library(inih::inih ALIAS inih)
