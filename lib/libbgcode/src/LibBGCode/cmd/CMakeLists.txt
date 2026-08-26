add_executable(${_libname}_cmd  main.cpp )

find_package(Boost ${Boost_VER} REQUIRED COMPONENTS nowide)

if (MSVC)
    set_target_properties(Boost::nowide PROPERTIES MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release)
endif ()

set_target_properties(${_libname}_cmd PROPERTIES OUTPUT_NAME ${_libname})
target_link_libraries(${_libname}_cmd ${_libname}_convert Boost::nowide)


if (NOT EMSCRIPTEN)
    install(TARGETS ${_libname}_cmd
        EXPORT ${PROJECT_NAME}ConvertTargets
        RUNTIME  DESTINATION bin
        INCLUDES DESTINATION include/${PROJECT_NAME}
    )
else ()
    target_link_libraries(${_libname}_cmd nodefs.js noderawfs.js)
endif ()
