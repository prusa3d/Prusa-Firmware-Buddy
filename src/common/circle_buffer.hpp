// circle_buffer.hpp
#pragma once

#include <stdint.h>

static constexpr const char *CircleBufferEmpty = "";

// you can never use entire size
// because write position (end) cannot be equal to begin
// because begin == end == empty
template <size_t SIZE, size_t MAX_LENGTH>
class CircleStringBuffer {
    char msg_data[SIZE][MAX_LENGTH];
    size_t begin; // position of first element
    size_t end;   // position behind last element == write position
    size_t pushed;
    static void incrementIndex(size_t &index) { index = (index + 1) % SIZE; }
    static void decrementIndex(size_t &index) { index = (index + SIZE - 1) % SIZE; }

public:
    CircleStringBuffer()
        : begin(0)
        , end(0)
        , pushed(0) {}

    void push_back(const char *const msg);
    void push_back_DontRewrite(const char *const msg);
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
void CircleStringBuffer<SIZE, MAX_LENGTH>::push_back(const char *const msg) {
    strlcpy(msg_data[end], msg, MAX_LENGTH);
    incrementIndex(end);
    if (begin == end) { //begin just was erased, set new begin
        incrementIndex(begin);
    }
    ++pushed;
}

template <size_t SIZE, size_t MAX_LENGTH>
void CircleStringBuffer<SIZE, MAX_LENGTH>::push_back_DontRewrite(const char *const msg) {
    size_t index = begin;
    incrementIndex(index);
    if (index != end)
        push_back(msg);
}

template <size_t SIZE, size_t MAX_LENGTH>
const char *CircleStringBuffer<SIZE, MAX_LENGTH>::ConsumeFirst() {
    if (IsEmpty())
        return CircleBufferEmpty;
    const char *ret = GetFirst();
    incrementIndex(begin);
    return ret;
}

template <size_t SIZE, size_t MAX_LENGTH>
const char *CircleStringBuffer<SIZE, MAX_LENGTH>::ConsumeLast() {
    if (IsEmpty())
        return CircleBufferEmpty;
    const char *ret = GetLast();
    decrementIndex(end);
    return ret;
}

template <size_t SIZE, size_t MAX_LENGTH>
const char *CircleStringBuffer<SIZE, MAX_LENGTH>::GetFirst() const {
    return IsEmpty() ? CircleBufferEmpty : msg_data[begin];
}

template <size_t SIZE, size_t MAX_LENGTH>
const char *CircleStringBuffer<SIZE, MAX_LENGTH>::GetLast() const {
    size_t index = end;
    decrementIndex(index);
    return IsEmpty() ? CircleBufferEmpty : msg_data[index];
}

/*****************************************************************************/
// you can never use entire size
// because write position (end) cannot be equal to begin
// because begin == end == empty
template <class T, size_t SIZE>
class CircleBuffer {
    T data[SIZE];
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

    void push_back(T elem);
    bool push_back_DontRewrite(T elem);
    size_t Count() const { return (end + SIZE - begin) % SIZE; }
    bool IsEmpty() const { return begin == end; }
    size_t PushedCount() const { return pushed; }

    constexpr size_t Size() const { return SIZE; }

    bool ConsumeFirst(T &elem); // data must be processed before next push_back
    bool ConsumeLast(T &elem);  // data must be processed before next push_back
    const T &GetFirst() const;  // data must be processed before next push_back, must not be empty
    const T &GetLast() const;   // data must be processed before next push_back, must not be empty
};

template <class T, size_t SIZE>
void CircleBuffer<T, SIZE>::push_back(T elem) {
    data[end] = elem;
    incrementIndex(end);
    if (begin == end) { //begin just was erased, set new begin
        incrementIndex(begin);
    }
    ++pushed;
}

template <class T, size_t SIZE>
bool CircleBuffer<T, SIZE>::push_back_DontRewrite(T elem) {
    size_t index = begin;
    incrementIndex(index);
    if (index != end) {
        push_back(elem);
        return true;
    }
    return false;
}

template <class T, size_t SIZE>
bool CircleBuffer<T, SIZE>::ConsumeFirst(T &elem) {
    if (IsEmpty())
        return false;
    elem = GetFirst();
    incrementIndex(begin);
    return true;
}

template <class T, size_t SIZE>
bool CircleBuffer<T, SIZE>::ConsumeLast(T &elem) {
    if (IsEmpty())
        return false;
    elem = GetLast();
    decrementIndex(end);
    return true;
}

template <class T, size_t SIZE>
const T &CircleBuffer<T, SIZE>::GetFirst() const {
    return data[begin];
}

template <class T, size_t SIZE>
const T &CircleBuffer<T, SIZE>::GetLast() const {
    size_t index = end;
    decrementIndex(index);
    return data[index];
}
