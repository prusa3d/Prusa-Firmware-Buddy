add_executable(makefsdata)

target_include_directories(makefsdata PRIVATE src)
target_sources(makefsdata PRIVATE src/makefsdata.c)
