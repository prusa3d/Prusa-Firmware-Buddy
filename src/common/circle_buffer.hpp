// circle_buffer.hpp
#pragma once

#include <stdint.h>

static constexpr const char *CircleBufferEmpty = "";

// you can never use entire size
// because write position (end) cannot be equal to begin
// because begin == end == empty
template <size_t SIZE, size_t MAX_LENGTH>
class CircleBuffer {
    char msg_data[SIZE][MAX_LENGTH];
    size_t begin; // position of first element
    size_t end;   // position behind last element == write position
    size_t pushed;
    static void incrementIndex(size_t &index) { index = (index + 1) % SIZE; }
    static void decrementIndex(size_t &index) { index = (index + SIZE - 1) % SIZE; }

public:
    CircleBuffer()
        : begin(0)
        , end(0)
        , pushed(0) {}

    void push_back(const char *const popup_msg);
    void push_back_DontRewrite(const char *const popup_msg);
    size_t Count() const { return (end + SIZE - begin) % SIZE; }
    bool IsEmpty() const { return begin == end; }
    size_t PushedCount() const { return pushed; }

    constexpr size_t Size() const { return SIZE; }
    constexpr size_t MaxStrLen() const { return MAX_LENGTH; }

    const char *ConsumeFirst();   // data must be processed before next push_back
    const char *ConsumeLast();    // data must be processed before next push_back
    const char *GetFirst() const; // data must be processed before next push_back
    const char *GetLast() const;  // data must be processed before next push_back
};

template <size_t SIZE, size_t MAX_LENGTH>
void CircleBuffer<SIZE, MAX_LENGTH>::push_back(const char *const popup_msg) {
    strlcpy(msg_data[end], popup_msg, MAX_LENGTH);
    incrementIndex(end);
    if (begin == end) { //begin just was erased, set new begin
        incrementIndex(begin);
    }
    ++pushed;
}

template <size_t SIZE, size_t MAX_LENGTH>
void CircleBuffer<SIZE, MAX_LENGTH>::push_back_DontRewrite(const char *const popup_msg) {
    size_t index = begin;
    incrementIndex(index);
    if (index != end)
        push_back(popup_msg);
}

template <size_t SIZE, size_t MAX_LENGTH>
const char *CircleBuffer<SIZE, MAX_LENGTH>::ConsumeFirst() {
    if (IsEmpty())
        return CircleBufferEmpty;
    const char *ret = GetFirst();
    incrementIndex(begin);
    return ret;
}

template <size_t SIZE, size_t MAX_LENGTH>
const char *CircleBuffer<SIZE, MAX_LENGTH>::ConsumeLast() {
    if (IsEmpty())
        return CircleBufferEmpty;
    const char *ret = GetLast();
    decrementIndex(end);
    return ret;
}

template <size_t SIZE, size_t MAX_LENGTH>
const char *CircleBuffer<SIZE, MAX_LENGTH>::GetFirst() const {
    return IsEmpty() ? CircleBufferEmpty : msg_data[begin];
}

template <size_t SIZE, size_t MAX_LENGTH>
const char *CircleBuffer<SIZE, MAX_LENGTH>::GetLast() const {
    size_t index = end;
    decrementIndex(index);
    return IsEmpty() ? CircleBufferEmpty : msg_data[index];
}
