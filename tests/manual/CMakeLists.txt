# Manual test binaries have no Catch2/bsod mock; give them a print+abort _bsod.
add_library(manual_bsod_stub STATIC bsod_stub.cpp)
target_link_libraries(manual_bsod_stub PUBLIC bsod)
link_libraries(manual_bsod_stub)

add_subdirectory(signal_processing)
add_subdirectory(signal2step)
