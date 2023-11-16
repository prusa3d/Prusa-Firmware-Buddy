#include <cinttypes>
#include <cstdlib>
#include <concepts>

template <std::integral T, size_t SIZE>
class SumRingBuffer {
public:
    typedef T sum_type;
    static_assert(SIZE > 0, "Invalid input");

    void Clear() {
        count = 0;
        index = 0;
        sum = 0;
    };
    void Put(T sample) {
        if (count < SIZE) {
            count++;
        } else {
            sum -= pdata[index];
        }
        sum += sample;
        pdata[index] = sample;
        if (++index >= SIZE) {
            index = 0;
        }
    };
    void PopLast() {
        if (count) {
            size_t last_idx = (index - count) % SIZE;
            sum -= pdata[last_idx];
            --count;
        }
    }
    size_t GetSize() {
        return SIZE;
    };
    size_t GetCount() {
        return count;
    };
    T GetSum() {
        return sum;
    };

protected:
    size_t count { 0 };
    size_t index { 0 };
    T pdata[SIZE] {};
    T sum { 0 };
};
