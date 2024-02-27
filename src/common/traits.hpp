#pragma once

#include <functional>
#include <type_traits>

/**
 * Function trait extracts argument & return type information
 **/
template <typename T>
struct function_traits;

// Function pointer specialization
template <typename ReturnType, typename... Args>
struct function_traits<ReturnType (*)(Args...)> {
    using return_type = ReturnType;
    using argument_types = std::tuple<Args...>;
};

// Member function pointer specialization
template <typename ClassType, typename ReturnType, typename... Args>
struct function_traits<ReturnType (ClassType::*)(Args...)> {
    using return_type = ReturnType;
    using argument_types = std::tuple<Args...>;
};

// Const member function pointer specialization
template <typename ClassType, typename ReturnType, typename... Args>
struct function_traits<ReturnType (ClassType::*)(Args...) const> {
    using return_type = ReturnType;
    using argument_types = std::tuple<Args...>;
};

// Lambda specialization
template <typename Lambda>
struct function_traits : public function_traits<decltype(&Lambda::operator())> {};
