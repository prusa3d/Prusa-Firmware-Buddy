#pragma once

#include <tuple>
#include <string.h>
#include <common/array_extensions.hpp>

namespace no_backend {

template <typename DataT, auto DefaultVal>
class CurrentItem {
public:
    static constexpr DataT default_val { DefaultVal };
    using value_type = DataT;

    DataT get() const {
        return data;
    }

    void set(const DataT &in) {
        data = in;
    }

    /**
     * @brief Set overload when DataT is holding a string, ie std::array<char, ...>.
     */
    void set(const char *in_buff, size_t max_length = std::tuple_size_v<DataT>)
        requires is_std_array_v<DataT> && std::same_as<char, typename DataT::value_type>
    {
        DataT arr_buff;
        strlcpy(arr_buff.data(), in_buff, std::min(arr_buff.size(), max_length));
        set(arr_buff);
    }

    /**
     * @brief Get overload when DataT is holding a string, ie std::array<char, ...>.
     */
    const char *get_c_str() const
        requires is_std_array_v<DataT> && std::same_as<char, typename DataT::value_type>
    {
        return data.data();
    }

    CurrentItem() = default;
    CurrentItem(const CurrentItem &other) = delete;
    CurrentItem(CurrentItem &&other) = delete;
    CurrentItem &operator=(const CurrentItem &other) = delete;
    CurrentItem &operator=(CurrentItem &&other) = delete;

private:
    DataT data { DefaultVal };
};

template <typename DataT, DataT DefaultVal>
class DeprecatedItem {
public:
    static constexpr DataT default_val { DefaultVal };
    using value_type = DataT;
};

} // namespace no_backend
