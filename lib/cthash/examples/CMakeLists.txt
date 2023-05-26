add_executable(sha3 sha3.cpp)
target_link_libraries(sha3 cthash)

add_executable(shake128 shake128.cpp)
target_link_libraries(shake128 cthash)
