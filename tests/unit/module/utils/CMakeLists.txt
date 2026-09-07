# timer_event_period_tracker_tests
add_executable(
  timer_event_period_tracker_tests
  ${CMAKE_CURRENT_SOURCE_DIR}/timer_event_period_tracker_tests.cpp
  ${CMAKE_SOURCE_DIR}/src/module/utils/buddy_utils/utils/timing/timer_event_period_tracker.cpp
  )

target_include_directories(
  timer_event_period_tracker_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/module/utils/buddy_utils
  )

add_catch_test(timer_event_period_tracker_tests)

# atomic_circular_queue_test
add_executable(
  atomic_circular_queue_test ${CMAKE_CURRENT_SOURCE_DIR}/atomic_circular_queue_test.cpp
  )

target_include_directories(
  atomic_circular_queue_test PUBLIC ${CMAKE_SOURCE_DIR}/src/module/utils/buddy_utils
  )

add_catch_test(atomic_circular_queue_test)

# rate_limiter_tests
add_executable(rate_limiter_tests ${CMAKE_CURRENT_SOURCE_DIR}/rate_limiter_tests.cpp)

target_include_directories(
  rate_limiter_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/module/utils/buddy_utils
  )

add_catch_test(rate_limiter_tests)

# combo_counter_tests
add_executable(combo_counter_tests ${CMAKE_CURRENT_SOURCE_DIR}/combo_counter_tests.cpp)

target_include_directories(
  combo_counter_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/module/utils/buddy_utils
  )

add_catch_test(combo_counter_tests)

# timer_tests
add_executable(timer_tests ${CMAKE_CURRENT_SOURCE_DIR}/timer_tests.cpp)
target_include_directories(timer_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/module/utils/buddy_utils)
add_catch_test(timer_tests)

add_executable(latching_debouncer_tests ${CMAKE_CURRENT_SOURCE_DIR}/latching_debouncer_tests.cpp)
target_include_directories(
  latching_debouncer_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/module/utils/buddy_utils
  )
add_catch_test(latching_debouncer_tests)

# leaky_bucket_tests
add_executable(leaky_bucket_tests leaky_bucket.cpp)
target_include_directories(
  leaky_bucket_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/module/utils/buddy_utils
  )
add_catch_test(leaky_bucket_tests)

# value_pack_tests
add_executable(value_pack_tests value_pack_tests.cpp)
target_link_libraries(value_pack_tests buddy_utils)
add_catch_test(value_pack_tests)

# single_linked_list_tests
add_executable(single_linked_list_tests single_linked_list_tests.cpp)
target_include_directories(
  single_linked_list_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/module/utils/buddy_utils
  )
add_catch_test(single_linked_list_tests)

# compact_optional_tests
add_executable(compact_optional_tests compact_optional_tests.cpp)
target_include_directories(
  compact_optional_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/module/utils/buddy_utils
  )
add_catch_test(compact_optional_tests)

# debouncer_tests
add_executable(debouncer_tests debouncer_tests.cpp)
target_include_directories(debouncer_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/module/utils/buddy_utils)
add_catch_test(debouncer_tests)

# bitset_utils_tests
add_executable(bitset_utils_tests bitset_utils_tests.cpp)
target_include_directories(
  bitset_utils_tests PUBLIC ${CMAKE_SOURCE_DIR}/src/module/utils/buddy_utils
  )
add_catch_test(bitset_utils_tests)
