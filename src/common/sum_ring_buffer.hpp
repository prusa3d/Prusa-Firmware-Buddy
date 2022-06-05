#include <inttypes.h>
#include <stdlib.h>

template <class T, size_t SIZE>
class SumRingBuffer {
public:
    SumRingBuffer()
        : count(0)
        , index(0)
        , sum(0) {};

public:
    void Clear() {
        count = 0;
        index = 0;
        sum = 0;
    };
    void Put(T sample) {
        if (count < SIZE)
            count++;
        else
            sum -= pdata[index];
        sum += sample;
        pdata[index] = sample;
        if (++index >= SIZE)
            index = 0;
    };
    uint32_t GetSize() {
        return SIZE;
    };
    uint32_t GetCount() {
        return count;
    };
    T GetSum() {
        return sum;
    };

protected:
    uint32_t size;
    uint32_t count;
    uint32_t index;
    T pdata[SIZE];
    T sum;
};
