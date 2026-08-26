add_executable(${_libname}_wasm bgcode_wa.cpp index.html)
set_target_properties(${_libname}_wasm PROPERTIES OUTPUT_NAME ${_libname})

target_link_libraries(${_libname}_wasm bgcode_convert embind)
#set_target_properties(bgcode_wa PROPERTIES SUFFIX)
target_link_options(${_libname}_wasm PUBLIC
    -s TOTAL_MEMORY=512MB
#    -sEXPORTED_RUNTIME_METHODS=stringToNewUTF8,FS
#    -sEXPORTED_FUNCTIONS=_free
)

configure_file(index.html ${CMAKE_CURRENT_BINARY_DIR}/${_libname}.html)

install(TARGETS ${_libname}_wasm
    EXPORT ${PROJECT_NAME}ConvertTargets
    RUNTIME DESTINATION lib/wasm
    INCLUDES DESTINATION include/${PROJECT_NAME}
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${_libname}.wasm
    ${CMAKE_CURRENT_BINARY_DIR}/${_libname}.html
    DESTINATION lib/wasm  # or wherever you want these files to be installed
)
