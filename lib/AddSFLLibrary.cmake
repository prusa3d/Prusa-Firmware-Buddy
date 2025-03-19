add_library(sfl-library INTERFACE)

target_compile_definitions(sfl-library INTERFACE -DSFL_NO_EXCEPTIONS)
target_include_directories(sfl-library INTERFACE ${CMAKE_SOURCE_DIR}/lib/sfl-library/include)
