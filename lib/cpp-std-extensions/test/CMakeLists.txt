function(add_tests)
    foreach(name ${ARGN})
        add_unit_test(
            "${name}_test"
            CATCH2
            FILES
            "${name}.cpp"
            LIBRARIES
            warnings
            stdx)
    endforeach()
endfunction()

add_tests(
    always_false
    bit
    bitset
    callable
    concepts
    conditional
    ct_conversions
    cx_map
    cx_multimap
    cx_queue
    cx_set
    cx_vector
    default_panic
    for_each_n_args
    function_traits
    intrusive_list
    is_constant_evaluated
    overload
    panic
    priority
    remove_cvref
    to_underlying
    type_traits
    with_result_of
    udls)

if(${CMAKE_CXX_STANDARD} GREATER_EQUAL 20)
    add_tests(ct_string indexed_tuple tuple tuple_algorithms)
endif()
