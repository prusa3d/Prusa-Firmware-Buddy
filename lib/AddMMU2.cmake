add_library(MMU2 Prusa-Firmware-MMU/src/modules/protocol.cpp)
target_compile_features(MMU2 PUBLIC cxx_std_17)
add_library(MMU2::MMU2 ALIAS MMU2)
