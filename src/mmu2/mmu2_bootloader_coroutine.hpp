#pragma once

#include <coroutine>
#include <variant>
#include <memory_resource>

// Small coroutine framework
// Basic premise is that we have tasks that can depend on other tasks

namespace MMU2::bootloader {

struct PromiseBase;
class TaskBase;

template <typename Result_>
class Task;

template <typename Result_>
struct Promise;

struct FinalAwaiter {
    bool await_ready() const noexcept { return false; }
    void await_resume() const noexcept {}

    template <typename T>
    std::coroutine_handle<> await_suspend(std::coroutine_handle<Promise<T>> h) noexcept {
        // If a parent coroutine is waiting for the current coroutine to finish, resume it
        return h.promise().blocked_task ?: std::noop_coroutine();
    }
};

struct PromiseBase {
    std::suspend_never initial_suspend() { return {}; }
    FinalAwaiter final_suspend() const noexcept { return {}; }

    void unhandled_exception() {}

    /// Task that is waiting for the current task to be finished
    std::coroutine_handle<> blocked_task;

    struct AllocHeader {
        std::pmr::memory_resource *memory_resource;
        size_t bytes;
    };

    /// The constructor parameters are passed straight from the Task coroutine function.
    /// We expect first parameter to be allocator
    template <typename Mgr, typename... Args>
    void *operator new(std::size_t bytes, Mgr &mgr, Args...) {
        bytes += sizeof(AllocHeader);

        void *result = mgr.co_memory_resource.allocate(bytes);

        // Store pointer to allocator in the first bytes
        *reinterpret_cast<AllocHeader *>(result) = AllocHeader {
            .memory_resource = &mgr.co_memory_resource,
            .bytes = bytes,
        };

        return reinterpret_cast<uint8_t *>(result) + sizeof(AllocHeader);
    }

    void operator delete(void *ptr) {
        AllocHeader &header = *reinterpret_cast<AllocHeader *>(reinterpret_cast<uint8_t *>(ptr) - sizeof(AllocHeader));
        header.memory_resource->deallocate(&header, header.bytes);
    }
};

template <typename Result>
struct PromiseT : PromiseBase {
    Task<Result> get_return_object();
};

template <typename Result>
struct Promise : PromiseT<Result> {
    void return_value(Result &&value) { result = std::move(value); }
    Result result;
};

template <>
struct Promise<void> : PromiseT<void> {
    void return_void() {}
};

// Inspired by https://www.jeremyong.com/cpp/2021/01/04/cpp20-coroutines-a-minimal-async-framework/
template <typename Result_>
class Task {

public:
    using Result = Result_;
    using Handle = std::coroutine_handle<Promise<Result>>;

    using promise_type = Promise<Result>;

public:
    Task() = default;
    Task(Task &&o) = default;

    explicit Task(Handle h)
        : handle_(h) {}
    ~Task() { clear(); }

    Task &operator=(Task &&o) {
        // Destroy handle if the current object has one
        clear();

        // Swap our clear handle with the other tasks handle
        std::swap(handle_, o.handle_);

        return *this;
    }

    [[nodiscard]] bool is_active() const {
        return handle_ && !handle_.done();
    }

    void clear() {
        if (handle_) {
            handle_.destroy();
            handle_ = {};
        }
    }

public:
    bool await_ready() const noexcept {
        // Our tasks don't suspend intially, so they might already be ready when we're waiting for them.
        return handle_ && handle_.done();
    }
    Result await_resume() const noexcept {
        if constexpr (!std::is_same_v<Result, void>) {
            return std::move(handle_.promise().result);
        }
    }
    void await_suspend(std::coroutine_handle<> h) noexcept {
        // h is the coroutine handle that's waiting for the current task
        // store the waiting handle to the promise so that we can wake it up when the task finishes
        handle_.promise().blocked_task = h;
    }

private:
    Handle handle_;
};

template <typename Result>
Task<Result> PromiseT<Result>::get_return_object() {
    return Task<Result>(std::coroutine_handle<Promise<Result>>::from_promise(static_cast<Promise<Result> &>(*this)));
}

} // namespace MMU2::bootloader
