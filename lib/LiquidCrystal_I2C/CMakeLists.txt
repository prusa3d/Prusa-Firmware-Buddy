add_library(LiquidCrystal LiquidCrystal_I2C.cpp)

target_include_directories(LiquidCrystal PUBLIC .)

target_link_libraries(LiquidCrystal PUBLIC Arduino::Core)

add_library(Arduino::LiquidCrystal ALIAS LiquidCrystal)
