# define the test executable
add_executable(
  language_eeprom_tests ../../../../src/common/language_eeprom.cpp
                        ../../../../src/lang/translator.cpp ../../test_main.cpp tests.cpp
  )

# define required search paths
target_include_directories(
  language_eeprom_tests PUBLIC . ${CMAKE_SOURCE_DIR}/src/lang ${CMAKE_SOURCE_DIR}/src/common
                               ${CMAKE_SOURCE_DIR}/src/gui ${CMAKE_SOURCE_DIR}/include
  )

# define custom macros
target_compile_definitions(language_eeprom_tests PUBLIC LANGEEPROM_UNITTEST)

# tell build system about the test case
add_catch_test(language_eeprom_tests)
