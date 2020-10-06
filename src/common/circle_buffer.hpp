// circle_buffer.hpp
#pragma once

#include <stdint.h>

/*****************************************************************************/
// general circle buffer
// you can never use entire size
// because write position (end) cannot be equal to begin
// because begin == end == empty
template <class T, size_t SIZE>
class CircleBuffer {
protected:
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

    bool ConsumeFirst(T &elem);      // data must be processed before next push_back
    bool ConsumeLast(T &elem);       // data must be processed before next push_back
    const T &GetFirstIfAble() const; // data must be processed before next push_back, must not be empty
    const T &GetLastIfAble() const;  // data must be processed before next push_back, must not be empty
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
    elem = GetFirstIfAble();
    incrementIndex(begin);
    return true;
}

template <class T, size_t SIZE>
bool CircleBuffer<T, SIZE>::ConsumeLast(T &elem) {
    if (IsEmpty())
        return false;
    elem = GetLastIfAble();
    decrementIndex(end);
    return true;
}

template <class T, size_t SIZE>
const T &CircleBuffer<T, SIZE>::GetFirstIfAble() const {
    return data[begin];
}

template <class T, size_t SIZE>
const T &CircleBuffer<T, SIZE>::GetLastIfAble() const {
    size_t index = end;
    decrementIndex(index);
    return data[index];
}

/*****************************************************************************/
// circle buffer for strings (T == std::array<char, MAX_LENGTH>)
static constexpr const char *CircleBufferEmpty = "";
#include <array>

// you can never use entire size
// because write position (end) cannot be equal to begin
// because begin == end == empty
template <size_t SIZE, size_t MAX_LENGTH>
class CircleStringBuffer : public CircleBuffer<std::array<char, MAX_LENGTH>, SIZE> {
    using parent = CircleBuffer<std::array<char, MAX_LENGTH>, SIZE>;

public:
    CircleStringBuffer() = default;

    void push_back(const char *const msg);
    void push_back_DontRewrite(const char *const msg);

    constexpr size_t MaxStrLen() const { return MAX_LENGTH; }

    const char *ConsumeFirst();   // data must be processed before next push_back
    const char *ConsumeLast();    // data must be processed before next push_back
    const char *GetFirst() const; // data must be processed before next push_back
    const char *GetLast() const;  // data must be processed before next push_back
};

template <size_t SIZE, size_t MAX_LENGTH>
void CircleStringBuffer<SIZE, MAX_LENGTH>::push_back(const char *const msg) {
    std::array<char, MAX_LENGTH> arr;
    strlcpy(arr.data(), msg, MAX_LENGTH);
    parent::push_back(arr);
}

template <size_t SIZE, size_t MAX_LENGTH>
void CircleStringBuffer<SIZE, MAX_LENGTH>::push_back_DontRewrite(const char *const msg) {
    std::array<char, MAX_LENGTH> arr;
    strlcpy(arr, msg, MAX_LENGTH);
    push_back_DontRewrite(arr);
}

template <size_t SIZE, size_t MAX_LENGTH>
const char *CircleStringBuffer<SIZE, MAX_LENGTH>::ConsumeFirst() {
    if (parent::IsEmpty())
        return CircleBufferEmpty;
    const char *ret = parent::GetFirstIfAble().data();
    parent::incrementIndex(parent::begin);
    return ret;
}

template <size_t SIZE, size_t MAX_LENGTH>
const char *CircleStringBuffer<SIZE, MAX_LENGTH>::ConsumeLast() {
    if (parent::IsEmpty())
        return CircleBufferEmpty;
    const char *ret = parent::GetLastIfAble().data();
    parent::decrementIndex(parent::end);
    return ret;
}

template <size_t SIZE, size_t MAX_LENGTH>
const char *CircleStringBuffer<SIZE, MAX_LENGTH>::GetFirst() const {
    return parent::IsEmpty() ? CircleBufferEmpty : parent::GetFirstIfAble().data();
}

template <size_t SIZE, size_t MAX_LENGTH>
const char *CircleStringBuffer<SIZE, MAX_LENGTH>::GetLast() const {
    return parent::IsEmpty() ? CircleBufferEmpty : parent::GetLastIfAble().data();
}
