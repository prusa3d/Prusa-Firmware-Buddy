#pragma once

#include <freertos_mutex.hpp>
// Why is the FILE_PATH_BUFFER_LEN in gui?
#include <gui/file_list_defs.h>

#include <array>
#include <atomic>
#include <optional>

namespace transfers {

/// A place to report and retrieve filesystem changes (for now only on the usb),
/// that should be then reported to connect server, so it knows what (or that something) happpend.
///
/// # Usage
///
/// The changed_path function is right now called for all changes through prusa link and connect.
/// Changes can also happen from such things as screenshots and crash dumps. In the future we might
/// want to extend this to cover those. Connect then picks it up using consume_path and reports t
/// he change in an event to the server.
///
/// # Locking
///
/// Because it must be called for multiple threads it uses locking. The Status should not
/// be held longer then needed, because it holds the lock and thus blocks all other operations.
/// The only constraint on the caller is, that it does not call changed_path while holding the
/// Status, that would result in a deadlock, but it logically makes no sense anyway.
///
/// This gets used as a singleton, even though it is not a real one, for the purpose
/// of writing tests for it.
class ChangedPath {
private:
    using Mutex = FreeRTOS_Mutex;
    using Lock = std::unique_lock<Mutex>;

public:
    enum class Incident {
        Created,
        Deleted,
        Combined
    };

    enum class Type {
        File,
        Folder
    };

    class Status {
    private:
        friend class ChangedPath;
        Lock lock;
        Status(Lock &&lock)
            : lock(std::move(lock)) {}

        char *path { nullptr };
        Type type {};
        Incident incident {};

    public:
        Status(Status &&other) = default;
        Status &operator=(Status &&other) = default;
        Status(const Status &other) = delete;
        Status &operator=(const Status &other) = delete;

        /// Copies the changed path to provided buffer
        /// and resets the path to signal we reported
        /// the changes
        ///
        /// If for any reason you would need to check the
        /// path anywhere else and not use it to report
        /// it to the server, you would need to add a new
        /// viewer function, that do not do the reset.
        ///
        /// return false if the provided buffer is not big enough.
        bool consume_path(char *out, size_t size) const;

#ifdef UNITTESTS
        const char *get_path() const;
#endif
        bool is_file() const;
        Incident what_happend() const;
    };

public:
    void changed_path(const char *filepath, Type type, Incident incident);

    /// Request the changes to fs since last report
    ///
    /// If none nullopt is returned
    std::optional<Status> status();

    /// The global instance.
    static ChangedPath instance;

private:
    mutable Mutex mutex;

    std::array<char, FILE_PATH_BUFFER_LEN> path {};
    Type type {};
    Incident incident {};
};

}
