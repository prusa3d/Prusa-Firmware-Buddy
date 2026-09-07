add_library(coroutines STATIC "${CMAKE_CURRENT_SOURCE_DIR}/src/coroutines.cpp")

target_include_directories(coroutines PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
