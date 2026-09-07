# Common utilities directory, with all the nice things that one might want to use elsewhere

add_library(buddy_utils OBJECT)

target_link_libraries(buddy_utils PUBLIC SG14 random byte_utils)
target_include_directories(buddy_utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(filters)
add_subdirectory(utils)
