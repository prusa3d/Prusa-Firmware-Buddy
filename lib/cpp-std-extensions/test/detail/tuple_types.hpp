#pragma once

struct move_only {
    constexpr move_only() = default;
    constexpr move_only(int i) : value{i} {}
    constexpr move_only(move_only &&) = default;
    constexpr auto operator=(move_only &&) noexcept -> move_only & = default;

    friend constexpr auto operator==(move_only const &lhs, move_only const &rhs)
        -> bool {
        return lhs.value == rhs.value;
    }

    int value{};
};

struct counter {
    counter() = default;
    counter(counter const &) { ++copies; }
    counter(counter &&) { ++moves; }
    auto operator=(counter const &) -> counter & {
        ++copies;
        return *this;
    }
    auto operator=(counter &&) -> counter & {
        ++moves;
        return *this;
    }

    static auto reset() -> void {
        copies = 0;
        moves = 0;
    }
    static inline int copies{};
    static inline int moves{};
};
