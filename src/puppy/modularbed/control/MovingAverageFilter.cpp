#include "MovingAverageFilter.hpp"
#include <assert.h>
#include <string.h>

namespace modularbed {

MovingAverageFilter::MovingAverageFilter(uint32_t dataPointCount)
    : m_DataPointCount(dataPointCount)
    , m_pDataSet(nullptr)
    , m_ActualDataIndex(0)
    , m_DataSum(0)
    , m_DataMovingAverage(0) {

    m_pDataSet = new uint32_t[m_DataPointCount];
    assert(m_pDataSet != nullptr);
    memset(m_pDataSet, 0, sizeof(uint32_t[m_DataPointCount]));
}

MovingAverageFilter::~MovingAverageFilter() {
    if (m_pDataSet != nullptr) {
        delete[] m_pDataSet;
    }
}

void MovingAverageFilter::Reset(uint32_t value) {
    for (uint32_t i = 0; i < m_DataPointCount; i++) {
        m_pDataSet[i] = value;
    }

    m_DataSum = ((uint64_t)value) * m_DataPointCount;
    m_DataMovingAverage = value;
}

uint32_t MovingAverageFilter::AddValue(uint32_t value) {
    m_DataSum -= m_pDataSet[m_ActualDataIndex];
    m_DataSum += value;
    m_pDataSet[m_ActualDataIndex] = value;

    m_ActualDataIndex++;
    if (m_ActualDataIndex >= m_DataPointCount) {
        m_ActualDataIndex = 0;
    }

    m_DataMovingAverage = m_DataSum / m_DataPointCount;
    return m_DataMovingAverage;
}

uint32_t MovingAverageFilter::GetValue() {
    return m_DataMovingAverage;
}

} // namespace modularbed
