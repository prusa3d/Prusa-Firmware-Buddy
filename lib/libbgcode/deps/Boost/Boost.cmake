add_cmake_project(Boost
    URL "https://github.com/boostorg/boost/releases/download/boost-1.82.0/boost-1.82.0.zip"
    URL_HASH SHA256=200f9292b5ef957ab551a648834239f502df165cb7bff18432702fb7ae98accb
    LIST_SEPARATOR |
    CMAKE_ARGS
        -DBOOST_INCLUDE_LIBRARIES:STRING=beast|nowide
        -DBUILD_TESTING:BOOL=OFF
)