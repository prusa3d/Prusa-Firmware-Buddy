#ifndef CTHASH_INTERNAL_ASSERT_HPP
#define CTHASH_INTERNAL_ASSERT_HPP

#ifndef NDEBUG
    #define CTHASH_ASSERT(e) cthash::assert_this(static_cast<bool>(e), #e, __FILE__, __LINE__);
#else
    #define CTHASH_ASSERT(e) ((void)(0))
#endif

#include <cstdio>
#include <cstdlib>

namespace cthash {

constexpr void assert_this(bool value, const char *expression, const char *file, unsigned line) {
    if (!value) {
        printf("%s:%u: failed assertion '%s'\n", file, line, expression);
        std::abort();
    }
}

} // namespace cthash

#endif
