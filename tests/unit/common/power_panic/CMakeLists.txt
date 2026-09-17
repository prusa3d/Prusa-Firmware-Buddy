add_executable(
  power_panic_storage_tests ${CMAKE_SOURCE_DIR}/src/common/power_panic_storage_flash.cpp
                            power_panic_storage_test.cpp
  )
target_include_directories(
  power_panic_storage_tests
  PRIVATE # shadows the Marlin-heavy power_panic_storage.hpp
          ${CMAKE_CURRENT_SOURCE_DIR}/mock ${CMAKE_SOURCE_DIR}/src/version/include
  )
target_link_libraries(power_panic_storage_tests logging-mock w25x-mock)
add_catch_test(power_panic_storage_tests)
