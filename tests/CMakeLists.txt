set_directory_properties(PROPERTIES EXCLUDE_FROM_ALL OFF)

# enable libstdc++ assertions
add_compile_definitions(_GLIBCXX_ASSERTIONS)

add_subdirectory(unit)
add_subdirectory(stubs/FreeRTOS)
add_subdirectory(manual)

# Some kind of manual test, outdated and doesn't compile. Leaving for now for future inspiration.
# add_subdirectory(module/Connect)
