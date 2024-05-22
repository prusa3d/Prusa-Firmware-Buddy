#include "IIR1Filter.hpp"

namespace modularbed {

IIR1Filter::IIR1Filter()
    : m_coefficient_1(0.5)
    , m_coefficient_2(0.5)
    , m_value(0) {}

IIR1Filter::IIR1Filter(float coefficient)
    : m_coefficient_1(coefficient)
    , m_coefficient_2(1 - coefficient)
    , m_value(0) {}

IIR1Filter::IIR1Filter(float coefficient, float initialValue)
    : m_coefficient_1(coefficient)
    , m_coefficient_2(1 - coefficient)
    , m_value(initialValue) {}

void IIR1Filter::SetCoefficient(float coefficient) {
    m_coefficient_1 = coefficient;
    m_coefficient_2 = 1 - coefficient;
}

float IIR1Filter::GetCoefficient() {
    return m_coefficient_1;
}

void IIR1Filter::SetValue(float value) {
    m_value = value;
}

float IIR1Filter::AddValue(float value) {
    m_value = m_coefficient_1 * value + m_coefficient_2 * m_value;
    return m_value;
}

float IIR1Filter::GetValue() {
    return m_value;
}

} // namespace modularbed
