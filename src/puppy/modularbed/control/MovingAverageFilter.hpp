#pragma once

#include <cstdint>

namespace modularbed {

class MovingAverageFilter {
public:
    MovingAverageFilter(uint32_t dataPointCount);
    ~MovingAverageFilter();

    void Reset(uint32_t value);
    uint32_t AddValue(uint32_t value);
    uint32_t GetValue();

private:
    uint32_t m_DataPointCount;
    uint32_t *m_pDataSet;
    uint32_t m_ActualDataIndex;
    uint64_t m_DataSum;
    uint32_t m_DataMovingAverage;
};

} // namespace modularbed
