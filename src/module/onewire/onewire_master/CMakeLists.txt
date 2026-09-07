add_library(onewire_master STATIC src/onewire_master.cpp)
target_include_directories(onewire_master PUBLIC include)
target_link_libraries(onewire_master PUBLIC bsod byte_utils)
