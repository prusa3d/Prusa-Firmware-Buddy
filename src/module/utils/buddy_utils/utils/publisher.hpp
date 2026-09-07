/// \file
#pragma once

#include <type_traits>
#include <inplace_function.hpp>

#include <utils/uncopyable.hpp>
#include <bsod/bsod.h>

template <typename... Args>
class Subscriber;

/// Point for registering callbacks to
/// Represented as a linked list
/// !!! Not thread-safe
template <typename... Args>
class PublisherBase : Uncopyable {

public:
    using Subscriber = ::Subscriber<Args...>;
    friend Subscriber;

protected:
    /// Calls callbacks of all registered Subscribers
    /// The execution order depends on the insertion order - newer hooks execute first.
    /// Warning - if a hook removes itself during the call, it can cause UB or crash.
    template <typename... CallArgs>
    void call_all(CallArgs &&...args) {
        static_assert((std::is_same_v<std::decay_t<CallArgs>, std::decay_t<Args>> && ...), "Implicit argument conversion in a for loop");

        for (auto it = first_; it; it = it->next_) {
            it->callback_(std::forward<CallArgs>(args)...);
        }
    }

private:
    void insert(Subscriber *item) {
        item->next_ = first_;
        first_ = item;
    }

    void remove(Subscriber *item) {
        Subscriber **current = &first_;
        while (*current != item) {
            debug_assert(*current);
            current = &((*current)->next_);
        }
        *current = (*current)->next_;
    }

private:
    Subscriber *first_ = nullptr;
};

template <typename... Args>
class Publisher : public PublisherBase<Args...> {

public:
    using PublisherBase<Args...>::call_all;
};

/// Guard that registers the provided callback to the specified point
/// The hook gets removed when the function is destroyed
/// !!! Not thread safe
template <typename... Args>
class Subscriber : Uncopyable {

public:
    using Callback = stdext::inplace_function<void(Args...)>;
    using Publisher = ::PublisherBase<Args...>;
    friend Publisher;

public:
    Subscriber() {
    }

    Subscriber(const auto &callback) {
        set_callback(callback);
    }

    // Note: Template deducation problems without the "auto"
    Subscriber(Publisher &publisher, const auto &callback) {
        set_callback(callback);
        bind(publisher);
    }

    ~Subscriber() {
        unbind();
    }

public:
    void bind(Publisher &publisher) {
        unbind();

        debug_assert(callback_);
        publisher.insert(this);
        publisher_ = &publisher;
    }

    void unbind() {
        if (publisher_) {
            publisher_->remove(this);
            publisher_ = nullptr;
        }
    }

    // Note: Template deducation problems without the "auto"
    void set_callback(const auto &callback) {
        callback_ = callback;
    }

private:
    Publisher *publisher_ = nullptr;
    Subscriber *next_ = nullptr;
    Callback callback_ = {};
};

template <typename Publisher, typename Lambda>
class LambdaSubscriber : public Publisher::Subscriber {

public:
    LambdaSubscriber(Publisher &publisher, Lambda &&lambda)
        : Publisher::Subscriber(publisher, [this](auto... args) { lambda_(args...); })
        , lambda_(lambda) {
    }

private:
    Lambda lambda_;
};
