# st25r39xxb OPTBackend implementation for OPTReader

add_library(openprinttag_nfcv STATIC)
target_sources(openprinttag_nfcv PRIVATE openprinttag_nfcv/opt_nfcv.cpp)
target_include_directories(openprinttag_nfcv PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(openprinttag_nfcv PUBLIC openprinttag nfcv SG14 buddy_utils raii bsod)
