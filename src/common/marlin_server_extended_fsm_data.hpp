#pragma once

#include <common/fsm_base_types.hpp>
#include "log.h"
#include "marlin_vars.hpp"
#include <common/no_rtti_type_id.hpp>

/// @brief Parent of all extended FSM data
class FSMExtendedData {
public:
    FSMExtendedData() = default;
};

/// concept used to check if item is subclass of FSMExtendedData
template <class T>
concept FSMExtendedDataSubclass = std::is_base_of<FSMExtendedData, T>::value;

LOG_COMPONENT_REF(Marlin);

/**
 * @brief Class used to send extended (bigger then 31bits) data between marlin server and marlin client
 *        Only one instance of this data is currently supported, so it will not work with nested FSMs etc.
 *
 * @note Usage:
 *  Define shared type:
 *      class DataType_t : public FSMExtendedData
 *      {
 *          int value;
 *      }
 *
 * Send data from server:
 *     DataType_t data
 *     data.value = 123;
 *     FSM_CHANGE_WITH_EXTENDED_DATA__LOGGING(phase, data);
 *
 *  Receive data in client:
 *      DataType_t dt;
 *      if (FSMExtendedDataManager::get(dt))
 *      {
 *          cout << "Hello world, value: " << dt.value;
 *      }
 *
 */
class FSMExtendedDataManager {
public:
    /// @brief Store extended data from server, use via FSM_CHANGE_WITH_EXTENDED_DATA__LOGGING
    /// @tparam T type with data
    /// @param data
    /// @return id
    template <FSMExtendedDataSubclass T>
    static bool store(T &data) {
        // do simple check that get_identifier works as expected and provides diffent value for different types
        assert(get_identifier<T>() != get_identifier<FSMExtendedData>());

        // all operations are done under lock
        auto guard = MarlinVarsLockGuard();

        static_assert(sizeof(data) <= sizeof(extended_data_buffer)); // check if data type fits into buffer

        size_t new_identifier = get_identifier<T>();

        // check if data inside extended_data_buffer already matches what we are tring to store, if yes skip just return false and don't update the data
        if (identifier == new_identifier && (*reinterpret_cast<T *>(&extended_data_buffer)) == data) {
            return false;
        }

        new (&extended_data_buffer) T(data);
        identifier = new_identifier;

        return true;
    }

    /// @brief  Get extended data from client, only when true is returned, data is actually available
    /// @tparam T
    /// @param result
    /// @return
    template <FSMExtendedDataSubclass T>
    static bool get(T &result) {
        // all operations are done under lock
        auto guard = MarlinVarsLockGuard();

        size_t expected_identifier = get_identifier<T>();

        if (expected_identifier == identifier) {
            // stored identifier matches, reinterpret buffer to instance of data type, and use assignment operator to return it
            result = *reinterpret_cast<T *>(&extended_data_buffer);
            return true;
        } else {
            log_info(Marlin, "FSM extended data get fail");
            return false;
        }
    }

private:
    static constexpr size_t buffer_size = 32; //< size of buffer that is used to exchange extended data between server->client
    static size_t identifier; //< contains identifier of type, that is currently stored
    static uint8_t extended_data_buffer[buffer_size];

    /// @brief Obtain type identifier of stored data
    /// @tparam T subclass of FSMExtendedData
    /// @return identifier
    template <FSMExtendedDataSubclass T>
    static constexpr uintptr_t get_identifier() {
        return no_rtti_hash_code<T>();
    }
};
