#pragma once

#include <freertos/mutex.hpp>

#include <cstddef>
#include <array>
#include <optional>
#include <mutex>
#include <cstring>

namespace crash_dump {

// Class for registration of an memory, that needs to be zeroed out before crash dump,
// used for e2ee encryption keys etc. Use the Secret class below for any private info
// not to be included in the dump.
class PrivacyProtection {
public:
    void reg(void *ptr, size_t size);
    void unreg(void *ptr);
    void clean_up();

private:
    struct PrivacyRecord {
        void *ptr;
        size_t size;
    };

    freertos::Mutex mutex;
    // The 10 is kind of a magic constant, not sure if there is a good way to determine it,
    // right now it fills up to 8 at the highest point. Needs to be enlarged if some
    // other Secret info is added
    std::array<std::optional<PrivacyRecord>, 10> to_delete;
};

extern PrivacyProtection privacy_protection;

// Used for secrets in a local scope, that cannot fit into the Secret class below.
// Destructors are called in reverse order, so if this is created after the memory itself,
// then it will unregister before destructing the protected memory.
class ManualSecret {
public:
    ManualSecret(void *address, size_t size);
    ManualSecret(const ManualSecret &) = delete;
    ManualSecret(ManualSecret &&) = delete;
    ~ManualSecret();

private:
    void *address;
};

template <typename T>
    requires std::is_trivially_copyable_v<T>
class Secret {
public:
    Secret() {
        privacy_protection.reg(reinterpret_cast<void *>(&value), sizeof(T));
    }
    ~Secret() {
        privacy_protection.unreg(reinterpret_cast<void *>(&value));
    }
    Secret(const Secret &other)
        : value(other.value) {
        privacy_protection.reg(reinterpret_cast<void *>(&value), sizeof(T));
    }
    Secret(Secret &&other)
        : value(std::move(other.value)) {
        privacy_protection.reg(reinterpret_cast<void *>(&value), sizeof(T));
    }
    Secret &operator=(const Secret &other) = default;
    Secret &operator=(Secret &&other) = default;

    T *operator->() {
        return &value;
    }

    T *get() {
        return &value;
    }

private:
    T value;
};

} // namespace crash_dump
