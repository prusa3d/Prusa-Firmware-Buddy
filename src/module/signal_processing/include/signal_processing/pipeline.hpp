#pragma once

#include <chrono>
#include <cstddef>
#include <cstring>
#include <memory>
#include <ranges>
#include <new>
#include <tuple>
#include <type_traits>
#include <iterator>
#include <utility>
#include <array>
#include <bsod/bsod.h>

// Pull-based signal processing pipeline.
//
// Unlike std::ranges which assume data are always available, this pipeline
// models sources that might be temporarily empty (e.g., waiting for sensor
// data). Sources carry sampling frequency so frequency-dependent transforms
// are convenient and less error-prone.
//
// Usage:
//
//   auto source = sp::pipe::QueueSource{std::queue<float>{}};
//   source.push_sample(sample);
//
//   auto pipeline = sp::pipe::ref(source) | sp::pipe::transform([](float x) { return x * 2.0f; });
//
//   while (pipeline.poll() != sp::pipe::PollResult::done) {
//       if (pipeline.poll() == sp::pipe::PollResult::pending)
//           continue;
//       auto sample = pipeline.next();
//   }
//
// The pipeline is owning. Use sp::pipe::ref(source) to reference external
// sources.
//
// To implement new nodes:
//
// 1. Derive from NodeBase<S> and implement next():
//
//    template <Source S>
//    class MyNode : public NodeBase<S> {
//    public:
//        using sample_type = typename S::sample_type;
//
//        MyNode(S source, /* params */)
//            : NodeBase<S>(std::move(source)), /* state */ {}
//
//        sample_type next() {
//            return process(this->source.next());
//        }
//    };
//
// 2. Create a factory using adaptor<>:
//
//    inline auto my_node(int param) {
//        return adaptor<MyNode>(param);
//    }
//
// 3. Use it: auto pipeline = source | my_node(42);

namespace sp {
using SamplingFreq = float;
using Duration = std::chrono::microseconds;
} // namespace sp

namespace sp::pipe {

enum class PollResult {
    ready,
    pending,
    done,
};

// Source is any type that can produce samples via next(). poll() returns the
// tri-state availability. sampling_freq() carries frequency metadata.
template <class S>
concept Source = requires(S s, const S cs) {
    typename S::sample_type;
    { s.next() } -> std::same_as<typename S::sample_type>;
    { s.poll() } -> std::same_as<PollResult>;
    { cs.sampling_freq() } -> std::same_as<sp::SamplingFreq>;
};

// Free function convenience wrappers
inline bool available(Source auto &s) { return s.poll() == PollResult::ready; }
inline bool finished(Source auto &s) { return s.poll() == PollResult::done; }

namespace detail {

    // Shared helpers for multi-source nodes
    inline sp::SamplingFreq merge_sampling_freq(sp::SamplingFreq current, sp::SamplingFreq next) {
        if (current == sp::SamplingFreq { 0 }) {
            return next;
        }
        if (next == sp::SamplingFreq { 0 }) {
            return current;
        }
        debug_assert(current == next && "All sources must have the same sampling frequency");
        return current;
    }

    template <typename... Sources>
    sp::SamplingFreq resolve_sampling_freq(const std::tuple<Sources...> &sources) {
        return std::apply([](const auto &...srcs) {
            sp::SamplingFreq freq = sp::SamplingFreq { 0 };
            ((freq = merge_sampling_freq(freq, srcs.sampling_freq())), ...);
            return freq;
        },
            sources);
    }

} // namespace detail

// Base class for single-upstream nodes. Forwards poll() and sampling_freq().
// Derived classes provide sample_type and next().
template <Source S>
class NodeBase {
protected:
    S source;

public:
    explicit NodeBase(S source)
        : source(std::move(source)) {}

    PollResult poll() {
        return source.poll();
    }

    sp::SamplingFreq sampling_freq() const {
        return source.sampling_freq();
    }
};

// Generic adaptor helper: wraps a node class template (templated on a single
// Source type) into a factory lambda for operator|. Constructor args are
// captured and forwarded.
template <template <class> class Node, typename... Args>
auto adaptor(Args &&...args) {
    return [... args = std::forward<Args>(args)]<class S>(S && s) mutable
        requires Source<std::remove_cvref_t<S>>
    {
        return Node<std::remove_cvref_t<S>> {
            std::forward<S>(s), std::move(args)...
        };
    };
}

// The "core" piping operator allowing to chain pipelines.
template <class S, class Adaptor>
    requires(Source<std::remove_cvref_t<S>>) && (requires(Adaptor a, S s) { a(std::move(s)); })
auto operator|(S &&s, Adaptor &&a) {
    return std::forward<Adaptor>(a)(std::forward<S>(s));
}

// An infinite sequence of constant samples.
template <typename T>
class ConstantSource {
public:
    using sample_type = T;
    ConstantSource(T value, sp::SamplingFreq sampling_freq = sp::SamplingFreq { 0 })
        : value(value)
        , sampling_freq_value(sampling_freq) {}

    T next() {
        return value;
    }

    PollResult poll() {
        return PollResult::ready;
    }

    sp::SamplingFreq sampling_freq() const {
        return sampling_freq_value;
    }

private:
    T value;
    sp::SamplingFreq sampling_freq_value;
};

// A source that provides samples from a pair of iterators. Caller must ensure
// the iterators remain valid for the lifetime of the source.
template <class Iterator>
class IteratorSource {
public:
    using sample_type = typename std::iterator_traits<Iterator>::value_type;

    IteratorSource(Iterator begin, Iterator end, sp::SamplingFreq sampling_freq = sp::SamplingFreq { 0 })
        : current(begin)
        , end_(end)
        , sampling_freq_value(sampling_freq) {}

    sample_type next() {
        return *current++;
    }

    PollResult poll() {
        return current != end_ ? PollResult::ready : PollResult::done;
    }

    sp::SamplingFreq sampling_freq() const {
        return sampling_freq_value;
    }

private:
    Iterator current;
    Iterator end_;
    sp::SamplingFreq sampling_freq_value;
};

template <class Container>
class ContainerSource : public IteratorSource<typename Container::const_iterator> {
public:
    ContainerSource(const Container &container, sp::SamplingFreq sampling_freq = sp::SamplingFreq { 0 })
        : IteratorSource<typename Container::const_iterator>(
            container.begin(), container.end(), sampling_freq) {}
};

// A source where samples are pushed via push_sample(). Buffered by Queue.
template <typename Queue>
class QueueSource {
public:
    using sample_type = typename Queue::value_type;
    QueueSource(Queue &&queue, sp::SamplingFreq sampling_freq = sp::SamplingFreq { 0 })
        : queue(std::move(queue))
        , sampling_freq_value(sampling_freq) {}

    sample_type next() {
        debug_assert(poll() == PollResult::ready);
        auto val = queue.front();
        queue.pop();
        return val;
    }

    PollResult poll() {
        if (!queue.empty()) {
            return PollResult::ready;
        }
        return finalized ? PollResult::done : PollResult::pending;
    }

    void push_sample(const sample_type &sample) {
        queue.push(sample);
    }

    void finalize() {
        finalized = true;
    }

    sp::SamplingFreq sampling_freq() const {
        return sampling_freq_value;
    }

private:
    Queue queue;
    bool finalized = false;
    sp::SamplingFreq sampling_freq_value;
};

// Wraps another source by reference. The referenced source must outlive any
// pipeline using this reference.
template <Source S>
class ReferenceSource {
public:
    using sample_type = typename S::sample_type;

    explicit ReferenceSource(S &source)
        : source(source) {}

    sample_type next() {
        return source.next();
    }

    PollResult poll() {
        return source.poll();
    }

    sp::SamplingFreq sampling_freq() const {
        return source.sampling_freq();
    }

private:
    S &source;
};

template <Source S>
ReferenceSource<S> ref(S &source) {
    return ReferenceSource<S>(source);
}

template <Source S>
class OverrideSamplingFreqNode : public NodeBase<S> {
public:
    using sample_type = typename S::sample_type;

    OverrideSamplingFreqNode(S source, sp::SamplingFreq sampling_freq)
        : NodeBase<S>(std::move(source))
        , sampling_freq_override(sampling_freq) {}

    sample_type next() {
        return this->source.next();
    }

    sp::SamplingFreq sampling_freq() const {
        return sampling_freq_override;
    }

private:
    sp::SamplingFreq sampling_freq_override;
};

inline auto override_sampling_freq(sp::SamplingFreq sampling_freq) {
    return adaptor<OverrideSamplingFreqNode>(sampling_freq);
}

// Transforms each sample using a provided functor. Can change the sample type.
template <Source S, class Func>
class TransformNode : public NodeBase<S> {
public:
    using input_type = typename S::sample_type;
    using sample_type = std::invoke_result_t<Func, input_type>;

    TransformNode(S source, Func func)
        : NodeBase<S>(std::move(source))
        , func(std::move(func)) {}

    sample_type next() {
        return func(this->source.next());
    }

private:
    Func func;
};

template <class Func>
auto transform(Func func) {
    return [func = std::move(func)]<class S>(S && s)
        requires Source<std::remove_cvref_t<S>>
    {
        return TransformNode<std::remove_cvref_t<S>, Func> {
            std::forward<S>(s), func
        };
    };
}

// Explicit type cast node
template <typename T>
auto cast() {
    return transform([](auto x) { return static_cast<T>(x); });
}

template <Source S>
class IntegrateNode : public NodeBase<S> {
public:
    using sample_type = typename S::sample_type;

    explicit IntegrateNode(S source, sample_type initial_value = sample_type {})
        : NodeBase<S>(std::move(source))
        , accumulator(initial_value)
        , dt(static_cast<sample_type>(1.0) / static_cast<sample_type>(this->source.sampling_freq())) {
        debug_assert(this->source.sampling_freq() > sp::SamplingFreq { 0 } && "IntegrateNode requires a source with valid sampling frequency");
    }

    sample_type next() {
        accumulator = accumulator + this->source.next() * dt;
        return accumulator;
    }

private:
    sample_type accumulator;
    sample_type dt;
};

inline auto integrate() {
    return adaptor<IntegrateNode>();
}

template <typename T>
auto integrate(T initial_value) {
    return adaptor<IntegrateNode>(initial_value);
}

// Computes: y[n] = (x[n] - x[n-1]) * sampling_freq
template <Source S>
class DifferentiateNode : public NodeBase<S> {
public:
    using sample_type = typename S::sample_type;

    explicit DifferentiateNode(S source, sample_type initial_value = sample_type {})
        : NodeBase<S>(std::move(source))
        , previous_sample(initial_value)
        , sampling_freq_value(static_cast<sample_type>(this->source.sampling_freq())) {
        debug_assert(this->source.sampling_freq() > sp::SamplingFreq { 0 }
            && "DifferentiateNode requires a source with valid sampling frequency");
    }

    sample_type next() {
        sample_type current = this->source.next();
        sample_type result = (current - previous_sample) * sampling_freq_value;
        previous_sample = current;
        return result;
    }

    sp::SamplingFreq sampling_freq() const {
        return sampling_freq_value;
    }

private:
    sample_type previous_sample;
    sample_type sampling_freq_value;
};

inline auto differentiate() {
    return adaptor<DifferentiateNode>();
}

template <typename T>
auto differentiate(T initial_value) {
    return adaptor<DifferentiateNode>(initial_value);
}

// Drops the first N samples from the stream
template <Source S>
class DropSamplesNode : public NodeBase<S> {
public:
    using sample_type = typename S::sample_type;

    DropSamplesNode(S source, int samples_to_drop)
        : NodeBase<S>(std::move(source))
        , samples_to_drop(samples_to_drop)
        , samples_dropped(0) {}

    sample_type next() {
        drop_samples_if_needed();
        return this->source.next();
    }

    PollResult poll() {
        drop_samples_if_needed();
        return this->source.poll();
    }

private:
    void drop_samples_if_needed() {
        while (samples_dropped < samples_to_drop && this->source.poll() == PollResult::ready) {
            this->source.next();
            samples_dropped++;
        }
    }

    int samples_to_drop;
    int samples_dropped;
};

inline auto drop_samples(int samples) {
    return adaptor<DropSamplesNode>(samples);
}

inline auto drop(sp::Duration duration) {
    return [duration]<class S>(S && s)
        requires Source<std::remove_cvref_t<S>>
    {
        float seconds = std::chrono::duration<float>(duration).count();
        int samples = static_cast<int>(seconds * s.sampling_freq());
        return DropSamplesNode<std::remove_cvref_t<S>> {
            std::forward<S>(s), samples
        };
    };
}

// Takes only the first N samples from the stream
template <Source S>
class TakeSamplesNode : public NodeBase<S> {
public:
    using sample_type = typename S::sample_type;

    TakeSamplesNode(S source, int samples_to_take)
        : NodeBase<S>(std::move(source))
        , samples_to_take(samples_to_take)
        , samples_taken(0) {}

    sample_type next() {
        samples_taken++;
        return this->source.next();
    }

    PollResult poll() {
        if (samples_taken >= samples_to_take) {
            return PollResult::done;
        }
        // Otherwise defer to upstream:
        return this->source.poll();
    }

private:
    int samples_to_take;
    int samples_taken;
};

// Takes samples from the stream for the specified duration, then close the
// source.
inline auto take_samples(int samples) {
    return adaptor<TakeSamplesNode>(samples);
}

inline auto take(sp::Duration duration) {
    return [duration]<class S>(S && s)
        requires Source<std::remove_cvref_t<S>>
    {
        float seconds = std::chrono::duration<float>(duration).count();
        debug_assert(seconds >= 0.f);
        int samples = static_cast<int>(seconds * s.sampling_freq());
        return TakeSamplesNode<std::remove_cvref_t<S>> {
            std::forward<S>(s), samples
        };
    };
}

// Chains multiple sources sequentially. Consumes from the first source until
// done, then moves to the next. All sources must have the same sampling
// frequency.
template <Source... Sources>
class ChainNode {
private:
    using FirstSource = std::tuple_element_t<0, std::tuple<Sources...>>;

public:
    using sample_type = typename FirstSource::sample_type;

    explicit ChainNode(Sources... sources)
        : sources(std::move(sources)...)
        , current_index(0)
        , sampling_freq_value(detail::resolve_sampling_freq(this->sources)) {}

    sample_type next() {
        advance_to_available_source(std::make_index_sequence<sizeof...(Sources)> {});
        debug_assert(current_index < sizeof...(Sources) && "ChainNode::next() called after all sources finished");
        return next_impl(std::make_index_sequence<sizeof...(Sources)> {});
    }

    PollResult poll() {
        advance_to_available_source(std::make_index_sequence<sizeof...(Sources)> {});
        if (current_index >= sizeof...(Sources)) {
            return PollResult::done;
        }
        return poll_current(std::make_index_sequence<sizeof...(Sources)> {});
    }

    sp::SamplingFreq sampling_freq() const {
        return sampling_freq_value;
    }

private:
    template <std::size_t... Is>
    sample_type next_impl(std::index_sequence<Is...>) {
        sample_type result {};
        ((current_index == Is ? (result = std::get<Is>(sources).next(), true) : false) || ...);
        return result;
    }

    template <std::size_t... Is>
    PollResult poll_current(std::index_sequence<Is...>) {
        PollResult result = PollResult::done;
        ((current_index == Is ? (result = std::get<Is>(sources).poll(), true) : false) || ...);
        return result;
    }

    template <std::size_t... Is>
    void advance_to_available_source(std::index_sequence<Is...>) {
        while (current_index < sizeof...(Sources)) {
            bool current_done = false;
            ((current_index == Is ? (current_done = (std::get<Is>(sources).poll() == PollResult::done)) : false) || ...);
            if (!current_done) {
                break;
            }
            current_index++;
        }
    }

    std::tuple<Sources...> sources;
    std::size_t current_index;
    sp::SamplingFreq sampling_freq_value;
};

// Zips multiple sources into tuples. Available when ALL are ready. Done when
// ANY is done.
template <Source... Sources>
class ZipNode {
public:
    using sample_type = std::tuple<typename Sources::sample_type...>;

    explicit ZipNode(Sources... sources)
        : sources(std::move(sources)...) {}

    sample_type next() {
        return std::apply([](auto &...srcs) {
            return std::make_tuple(srcs.next()...);
        },
            sources);
    }

    PollResult poll() {
        return std::apply([](auto &...srcs) {
            bool any_done = ((srcs.poll() == PollResult::done) || ...);
            if (any_done) {
                return PollResult::done;
            }
            bool all_ready = ((srcs.poll() == PollResult::ready) && ...);
            return all_ready ? PollResult::ready : PollResult::pending;
        },
            sources);
    }

    sp::SamplingFreq sampling_freq() const {
        return sampling_freq_value;
    }

private:
    std::tuple<Sources...> sources;
    sp::SamplingFreq sampling_freq_value = detail::resolve_sampling_freq(sources);
};

// Zips multiple sources, repeating the last value from sources that finish
// early. Output length equals the longest input. Asserts if any source is
// empty.
template <Source... Sources>
class ZipLongestTailNode {
public:
    using sample_type = std::tuple<typename Sources::sample_type...>;

    explicit ZipLongestTailNode(Sources... sources)
        : sources(std::move(sources)...)
        , last_values()
        , has_value {}
        , sampling_freq_value(detail::resolve_sampling_freq(this->sources)) {}

    sample_type next() {
        return next_impl(std::make_index_sequence<sizeof...(Sources)> {});
    }

    PollResult poll() {
        // Ready if ANY source is ready
        bool any_ready = std::apply([](auto &...srcs) {
            return ((srcs.poll() == PollResult::ready) || ...);
        },
            sources);
        if (any_ready) {
            return PollResult::ready;
        }

        // Done when ALL sources are done
        bool all_done = std::apply([](auto &...srcs) {
            return ((srcs.poll() == PollResult::done) && ...);
        },
            sources);
        return all_done ? PollResult::done : PollResult::pending;
    }

    sp::SamplingFreq sampling_freq() const {
        return sampling_freq_value;
    }

private:
    template <std::size_t... Is>
    sample_type next_impl(std::index_sequence<Is...>) {
        return std::make_tuple(get_next_value<Is>()...);
    }

    template <std::size_t I>
    auto get_next_value() -> typename std::tuple_element_t<I, std::tuple<Sources...>>::sample_type {
        auto &src = std::get<I>(sources);

        if (src.poll() == PollResult::ready) {
            auto value = src.next();
            std::get<I>(last_values) = value;
            has_value[I] = true;
            return value;
        } else {
            debug_assert(has_value[I] && "Source finished without providing any values");
            return std::get<I>(last_values);
        }
    }

    std::tuple<Sources...> sources;
    std::tuple<typename Sources::sample_type...> last_values;
    std::array<bool, sizeof...(Sources)> has_value;
    sp::SamplingFreq sampling_freq_value;
};

// Type-erased source. InlineSize == 0 uses heap allocation.
template <typename T, std::size_t InlineSize = 0>
class SignalSource {
private:
    static constexpr bool uses_inline_storage = InlineSize != 0;
    static constexpr std::size_t inline_storage_size = InlineSize;

    struct VTable {
        T(*next)
        (void *);
        PollResult (*poll)(void *);
        sp::SamplingFreq (*sampling_freq)(const void *);
        void (*destroy)(void *);
        void (*move_construct)(void *dst, void *src);
    };

    template <typename S>
    static const VTable vtable;

    alignas(std::max_align_t) std::byte storage[uses_inline_storage ? inline_storage_size : 1];
    void *object = nullptr;
    const VTable *vptr = nullptr;

public:
    using sample_type = T;

    template <Source S>
        requires std::same_as<typename std::remove_cvref_t<S>::sample_type, T>
        && (!std::same_as<std::remove_cvref_t<S>, SignalSource>)
    SignalSource(S &&source) {
        using Stored = std::remove_cvref_t<S>;

        if constexpr (uses_inline_storage) {
            static_assert(sizeof(Stored) <= inline_storage_size,
                "Source implementation does not fit in inline storage. "
                "Increase SignalSource inline size or use heap storage.");
            static_assert(alignof(Stored) <= alignof(std::max_align_t),
                "Source implementation has unsupported alignment requirements.");

            new (storage) Stored(std::forward<S>(source));
            object = storage;
        } else {
            object = new Stored(std::forward<S>(source));
        }

        vptr = &vtable<Stored>;
    }

    ~SignalSource() {
        reset();
    }

    SignalSource(SignalSource &&other)
        : vptr(other.vptr) {
        move_from(std::move(other));
    }

    SignalSource &operator=(SignalSource &&other) {
        if (this != &other) {
            reset();
            vptr = other.vptr;
            move_from(std::move(other));
        }
        return *this;
    }

    SignalSource(const SignalSource &) = delete;
    SignalSource &operator=(const SignalSource &) = delete;

    T next() {
        debug_assert(object != nullptr);
        return vptr->next(object);
    }

    PollResult poll() {
        debug_assert(object != nullptr);
        return vptr->poll(object);
    }

    sp::SamplingFreq sampling_freq() const {
        debug_assert(object != nullptr);
        return vptr->sampling_freq(object);
    }

private:
    void reset() {
        if (object == nullptr) {
            return;
        }

        vptr->destroy(object);
        if constexpr (!uses_inline_storage) {
            ::operator delete(object);
        }

        object = nullptr;
        vptr = nullptr;
    }

    void move_from(SignalSource &&other) {
        if (other.object == nullptr) {
            object = nullptr;
            vptr = nullptr;
            return;
        }

        if constexpr (uses_inline_storage) {
            vptr->move_construct(storage, other.object);
            object = storage;
            other.object = nullptr;
            other.vptr = nullptr;
        } else {
            object = other.object;
            other.object = nullptr;
            other.vptr = nullptr;
        }
    }
};

template <typename T, std::size_t InlineSize>
template <typename S>
const typename SignalSource<T, InlineSize>::VTable SignalSource<T, InlineSize>::vtable = {
    .next = [](void *ptr) -> T {
        return static_cast<S *>(ptr)->next();
    },
    .poll = [](void *ptr) -> PollResult {
        return static_cast<S *>(ptr)->poll();
    },
    .sampling_freq = [](const void *ptr) -> sp::SamplingFreq {
        return static_cast<const S *>(ptr)->sampling_freq();
    },
    .destroy = [](void *ptr) { static_cast<S *>(ptr)->~S(); },
    .move_construct = [](void *dst, void *src) {
        new (dst) S(std::move(*static_cast<S *>(src)));
        static_cast<S *>(src)->~S(); },
};

// Automatic storage deduction for the type-erased source. Use when you want
// type erasure but don't want to worry about whether the source fits in the
// inline storage. Use as: auto source{inline_source(ConstantSource<float>(3.14f))};
template <Source S>
SignalSource<typename S::sample_type, sizeof(S)> inline_source(S s) {
    return SignalSource<typename S::sample_type, sizeof(S)>(std::move(s));
}

template <Source... Sources>
ZipNode<std::remove_cvref_t<Sources>...> zip(Sources &&...sources) {
    return ZipNode<std::remove_cvref_t<Sources>...> { std::forward<Sources>(sources)... };
}

template <Source... Sources>
ZipLongestTailNode<std::remove_cvref_t<Sources>...> zip_longest_tail(Sources &&...sources) {
    return ZipLongestTailNode<std::remove_cvref_t<Sources>...> { std::forward<Sources>(sources)... };
}

template <Source... Sources>
ChainNode<std::remove_cvref_t<Sources>...> chain(Sources &&...sources) {
    return ChainNode<std::remove_cvref_t<Sources>...> { std::forward<Sources>(sources)... };
}

template <std::ranges::range R>
ContainerSource<std::remove_cvref_t<R>> make_source(const R &range,
    sp::SamplingFreq sampling_freq = sp::SamplingFreq { 0 }) {
    return ContainerSource<std::remove_cvref_t<R>>(range, sampling_freq);
}

template <typename T>
ConstantSource<std::remove_cvref_t<T>> make_constant(T &&value,
    sp::SamplingFreq sampling_freq = sp::SamplingFreq { 0 }) {
    return ConstantSource<std::remove_cvref_t<T>>(std::forward<T>(value), sampling_freq);
}

// Type erasure adaptors
template <typename T, std::size_t InlineSize = 0>
auto type_erase() {
    return []<Source S>(S && s)
        requires std::same_as<typename S::sample_type, T>
    {
        return SignalSource<T, InlineSize>(std::forward<S>(s));
    };
}

template <std::size_t InlineSize = 0>
auto type_erase_auto() {
    return []<Source S>(S &&s) {
        return SignalSource<typename S::sample_type, InlineSize>(std::forward<S>(s));
    };
}

} // namespace sp::pipe
