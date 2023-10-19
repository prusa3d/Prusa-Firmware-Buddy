#pragma once
#include <string.h>
#include <utility>
#include <cassert>

extern "C" size_t strlcat(char *, const char *, size_t);
extern "C" size_t strlcpy(char *dst, const char *src, size_t dsize);

class MutablePath {
private:
    // Must be able to contain hybrid SFN path + LFN tail
    static constexpr size_t BUFFER_SIZE = 256; // TODO: Provide proper defines of max path length
    mutable char path[BUFFER_SIZE];

    /**
     * @brief RAII guard class which is capable of temporarily pushing into a const MutablePath and then popping it
     *
     */
    struct TemporaryExtender {
        [[nodiscard]] TemporaryExtender(const MutablePath &path, const char *extension)
            : mp(path) {
            mp.push(extension);
            assert(strchr(extension, '/') == NULL); // pop woudln't return the path to previous state if someone attempts to push extension with '/' in it
        }
        ~TemporaryExtender() {
            mp.pop();
        }
        const MutablePath &mp;
    };

    /**
     * @brief Private const pop for TemporaryExtender, 'casting away' constness.
     *
     * @param component
     */
    void pop() const {
        pop_impl();
    }

    void pop_impl() const {
        char *slash = strrchr(path, '/');
        if (slash == nullptr) {
            return;
        }
        *slash = 0;
    }

    /**
     * @brief Private const push for TemporaryExtender, casting away constness
     *
     * @param component
     */
    void push(const char *component) const {
        push_impl(component);
    }

    void push_impl(const char *component) const {
        size_t length = strlen(path);
        if (length == 0 || path[length - 1] != '/') {
            strlcat(path, "/", sizeof(path));
        }
        strlcat(path, component, sizeof(path));
    }

public:
    MutablePath() {
        set("");
    }

    MutablePath(const char *path) {
        set(path);
    }

    void set(const char *path) {
        strlcpy(this->path, path, sizeof(this->path));
    }

    const char *get() const {
        return path;
    }

    void push(const char *component) {
        push_impl(component);
    }

    void pop() {
        pop_impl();
    }

    char *get_buffer() {
        return path;
    }

    static constexpr size_t maximum_length() {
        return sizeof(path);
    }

    /**
     * @brief Execute a callback on a path that's first extended (pushed) by a string.
     *
     * @param to_push String to extend the path with before calling the callback, ie "filename"
     * @param clb Callback to be executed with the extended path, needs to take the path as 'const char *' as first parameter (if incompatible for a function, wrap the function with a lambda to reorder arguments)
     * @param args Extra arguments to be given in order to the callback
     */
    template <typename ClbT, typename... Args>
    auto execute_with_pushed(const char *to_push, ClbT &&clb, Args &&...args) const {
        auto guard = TemporaryExtender { *this, to_push };
        return clb(get(), std::forward<decltype(args)...>(args)...);
    }
};
