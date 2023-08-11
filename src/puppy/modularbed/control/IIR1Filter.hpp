#pragma once

#include <cstdint>

namespace modularbed {

class IIR1Filter {
public:
    IIR1Filter();
    IIR1Filter(float coefficient);
    IIR1Filter(float coefficient, float initialValue);

    void SetCoefficient(float coefficient);
    float GetCoefficient();

    void SetValue(float value);
    float AddValue(float value);
    float GetValue();

private:
    float m_coefficient_1;
    float m_coefficient_2;
    float m_value;
};

} // namespace modularbed
