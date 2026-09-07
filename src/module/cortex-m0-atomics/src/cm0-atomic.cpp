#include <cstdint>
#include <cmsis_gcc.h>

namespace {

class CriticalSection {
    uint32_t primask;

public:
    [[gnu::always_inline]] inline CriticalSection()
        : primask(__get_PRIMASK()) {
        __disable_irq();
    }

    [[gnu::always_inline]] inline ~CriticalSection() {
        __set_PRIMASK(primask);
    }

    CriticalSection(const CriticalSection &) = delete;
    CriticalSection &operator=(const CriticalSection &) = delete;
};

template <auto op, typename T>
inline T atomic_op(volatile void *memv, T val, [[maybe_unused]] int model) [[gnu::always_inline]] {
    auto &mem = *static_cast<volatile T *>(memv);
    CriticalSection cs;
    const T r = mem;
    op(mem, val);
    return r;
}

constexpr auto op_add = [](auto &mem, auto val) { mem += val; };
constexpr auto op_sub = [](auto &mem, auto val) { mem -= val; };
constexpr auto op_xchg = [](auto &mem, auto val) { mem = val; };
constexpr auto op_or = [](auto &mem, auto val) { mem |= val; };
constexpr auto op_cmpxchg = [](auto &mem, auto &expected, auto desired) -> bool {
    const auto cur = mem;
    if (cur != expected) {
        expected = cur;
        return false;
    }
    mem = desired;
    return true;
};

template <auto op, typename T>
inline bool atomic_op(
    volatile void *memv, void *expectedv, T desired, [[maybe_unused]] int success_model, [[maybe_unused]] int failure_model) [[gnu::always_inline]] {
    auto &mem = *static_cast<volatile T *>(memv);
    auto &expected = *static_cast<T *>(expectedv);
    CriticalSection cs;
    return op(mem, expected, desired);
}

} // namespace

extern "C" [[gnu::used]] uint8_t __atomic_fetch_add_1(volatile void *memv, uint8_t val, int model) {
    return atomic_op<op_add>(memv, val, model);
}

extern "C" [[gnu::used]] uint16_t __atomic_fetch_add_2(volatile void *memv, uint16_t val, int model) {
    return atomic_op<op_add>(memv, val, model);
}

extern "C" [[gnu::used]] unsigned __atomic_fetch_add_4(volatile void *memv, unsigned val, [[maybe_unused]] int model) {
    return atomic_op<op_add>(memv, val, model);
}

extern "C" [[gnu::used]] unsigned __atomic_fetch_sub_4(volatile void *memv, unsigned val, [[maybe_unused]] int model) {
    return atomic_op<op_sub>(memv, val, model);
}

extern "C" [[gnu::used]] uint8_t __atomic_fetch_or_1(volatile void *memv, uint8_t val, [[maybe_unused]] int model) {
    return atomic_op<op_or>(memv, val, model);
}

extern "C" [[gnu::used]] unsigned __atomic_fetch_or_4(volatile void *memv, unsigned val, [[maybe_unused]] int model) {
    return atomic_op<op_or>(memv, val, model);
}

extern "C" [[gnu::used]] uint8_t __atomic_exchange_1(volatile void *memv, uint8_t val, [[maybe_unused]] int model) {
    return atomic_op<op_xchg>(memv, val, model);
}

extern "C" [[gnu::used]] unsigned __atomic_exchange_4(volatile void *memv, unsigned val, [[maybe_unused]] int model) {
    return atomic_op<op_xchg>(memv, val, model);
}

extern "C" [[gnu::used]] bool __atomic_compare_exchange_4(volatile void *ptr, void *expected, unsigned desired,
    [[maybe_unused]] bool weak, int success_memorder, int failure_memorder) {
    return atomic_op<op_cmpxchg>(ptr, expected, desired, success_memorder, failure_memorder);
}
