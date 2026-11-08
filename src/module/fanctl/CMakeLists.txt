add_library(fanctl STATIC src/CFanCtl3Wire.cpp src/CFanCtlCommon.cpp)
target_include_directories(fanctl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(
  fanctl
  PUBLIC SG14 # CFanCtl3Wire.hpp includes inplace_function.hpp
  PRIVATE random random_sw
  )
