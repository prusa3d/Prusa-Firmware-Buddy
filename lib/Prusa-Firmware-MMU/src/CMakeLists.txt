target_sources(firmware PRIVATE application.cpp debug.cpp main.cpp registers.cpp version.hpp)

target_link_libraries(firmware LUFA)

add_subdirectory(hal)
add_subdirectory(logic)
add_subdirectory(modules)
