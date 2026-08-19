# A library that defines interface for creating BSODs
add_library(bsod STATIC bsod.cpp)

target_include_directories(bsod PUBLIC .)
