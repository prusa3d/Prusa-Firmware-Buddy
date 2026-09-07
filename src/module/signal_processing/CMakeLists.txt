add_library(signal_processing STATIC src/signal_processing.cpp)

target_include_directories(signal_processing PUBLIC include)
target_link_libraries(signal_processing PUBLIC CMSISDSP bsod)
