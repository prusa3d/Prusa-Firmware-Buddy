set(LibBGCode_BUILD_TESTS
    "OFF"
    CACHE BOOL "Compile tests component of libbgcode"
    )
set(LibBGCode_BUILD_COMPONENT_Convert
    "OFF"
    CACHE BOOL "Compile bgcode_convert component of libbgcode"
    )
set(LibBGCode_BUILD_COMPONENT_Binarize
    "OFF"
    CACHE BOOL "Compile bgcode_binarize component of libbgcode"
    )

add_subdirectory(libbgcode)
set_property(TARGET bgcode_core PROPERTY CXX_STANDARD ${CMAKE_CXX_STANDARD})
