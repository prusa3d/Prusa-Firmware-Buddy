find_package(PNG REQUIRED)

add_executable(png2font)

target_sources(png2font PRIVATE src/main.c src/pngutils.c)

target_include_directories(png2font PRIVATE src)

target_link_libraries(png2font PNG::PNG)
