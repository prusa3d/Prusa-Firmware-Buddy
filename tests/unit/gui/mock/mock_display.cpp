/**
 * @file mock_display.cpp
 * @author Radek Vana
 * @date 2021-01-11
 */
#include "mock_display.hpp"

IMockDisplay *MockDisplay::instance = nullptr;

IMockDisplay &MockDisplay::Instance() {
    if (!instance) {
        throw "MockDisplay accesing nullptr";
    }
    return *instance;
}

uint16_t MockDisplay::Cols() {
    return Instance().Cols();
}

uint16_t MockDisplay::Rows() {
    return Instance().Rows();
}

uint16_t MockDisplay::BuffRows() {
    return Instance().BuffRows();
}

void MockDisplay::init() {
    Instance().clear(COLOR_WHITE);
}

void MockDisplay::Bind(IMockDisplay &disp) {
    instance = &disp;
}
