add_executable(
  mmu_tests ${CMAKE_SOURCE_DIR}/src/mmu2/fail_bucket.cpp config_store/store_instance.cpp
            test_bucket.cpp test_mmu_coroutines.cpp
  )
target_include_directories(mmu_tests PRIVATE . ${CMAKE_SOURCE_DIR}/src)

add_catch_test(mmu_tests)
