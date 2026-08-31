add_library(otp STATIC src/types.cpp src/utils.cpp)
target_include_directories(otp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(otp PUBLIC bsod byte_utils buddy_utils)
