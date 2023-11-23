add_cmake_project(Catch2
    URL "https://github.com/catchorg/Catch2/archive/refs/tags/v2.13.10.zip"
    URL_HASH SHA256=121e7488912c2ce887bfe4699ebfb983d0f2e0d68bcd60434cdfd6bb0cf78b43
    CMAKE_ARGS
        -DCATCH_BUILD_TESTING:BOOL=OFF
)