#pragma once

/// Helper class to grant class T fine access to member functions of some other
/// class U, without befriending them.
///
/// See https://awesomekling.github.io/Serenity-C++-patterns-The-Badge/
template <class T, typename Tag = void>
class Badge {
    friend T;

private:
    consteval Badge() {
        // must not use `= default` here to prevent construction by anybody via
        // aggregate initialization
    }
};
