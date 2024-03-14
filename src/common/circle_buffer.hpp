// circle_buffer.hpp
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <iterator>

/*****************************************************************************/
// general circular buffer
// you can never use entire size
// because write position (end) cannot be equal to begin
// because begin == end == empty
template <class T, size_t SIZE>
class CircleBuffer {
public:
    using Elem = T;

    /**
     * @brief Add and normalize index to be in range [0, SIZE).
     * Efficient for small deviations from the range.
     * @param index index
     * @param add addition to index
     * @return normalized result
     */
    static size_t normalizedAddition(size_t index, int add) {
        add += static_cast<int>(index);
        while (add >= static_cast<int>(SIZE)) {
            add -= SIZE;
        }
        while (add < 0) {
            add += SIZE;
        }
        return add;
    }
    static void incrementIndex(size_t &index) { index = normalizedAddition(index, 1); }
    static void decrementIndex(size_t &index) { index = normalizedAddition(index, -1); }

protected:
    T data[SIZE];
    size_t begin_pos; // position of first element
    size_t end_pos; // position behind last element == write position

public:
    template <typename constnessT> // Should be "T" or "const T"
    struct template_iterator {
        using iterator_concept [[maybe_unused]] = std::random_access_iterator_tag;
        using iterator_category = std::random_access_iterator_tag; // Needed for older GCC
        using difference_type = int;
        using element_type = constnessT;
        using pointer = element_type *;
        using reference = element_type &;

    private:
        pointer data; ///< Pointer to the data array
        const size_t *begin_pos; ///< Begin position
        size_t current_pos; ///< Iterator position

    public:
        template_iterator(pointer data_, size_t current_pos_, const size_t &begin_pos_)
            : data(data_)
            , begin_pos(&begin_pos_)
            , current_pos(current_pos_) {}

        template_iterator()
            : data(nullptr)
            , begin_pos(nullptr)
            , current_pos(0) {}

        template_iterator(const template_iterator &other)
            : data(other.data)
            , begin_pos(other.begin_pos)
            , current_pos(other.current_pos) {}

        template_iterator &operator=(const template_iterator &other) {
            data = other.data;
            current_pos = other.current_pos;
            begin_pos = other.begin_pos;
            return *this;
        }

        template_iterator(template_iterator &&other)
            : data(other.data)
            , begin_pos(other.begin_pos)
            , current_pos(other.current_pos) {
            if (this != &other) {
                other.data = nullptr;
                other.begin_pos = nullptr;
                other.current_pos = 0;
            }
        }

        template_iterator &operator=(template_iterator &&other) {
            data = other.data;
            current_pos = other.current_pos;
            begin_pos = other.begin_pos;
            if (this != &other) {
                other.data = nullptr;
                other.begin_pos = nullptr;
                other.current_pos = 0;
            }
            return *this;
        }

        /// Dereference
        reference operator*() const { return data[current_pos]; }
        pointer operator->() const { return &data[current_pos]; }

        /// Addition
        template_iterator operator+(difference_type add) const {
            template_iterator tmp = *this;
            tmp.current_pos = normalizedAddition(tmp.current_pos, add);
            return tmp;
        }

        friend template_iterator operator+(difference_type add, const template_iterator &other) { return other.operator+(add); }
        template_iterator operator+(const template_iterator &other) const { return operator+(other.current_pos); }
        reference operator[](difference_type add) const { return *operator+(add); }

        /// Addition assignment
        template_iterator &operator+=(difference_type add) {
            current_pos = normalizedAddition(current_pos, add);
            return *this;
        }

        /// Subtractions
        template_iterator operator-(difference_type subtract) const { return operator+(-subtract); }
        template_iterator &operator-=(difference_type subtract) { return operator+=(-subtract); }

        /// Postfix increment
        template_iterator operator++(difference_type) {
            template_iterator tmp = *this;
            (*this) += 1;
            return tmp;
        }

        /// Prefix increment
        template_iterator &operator++() {
            (*this) += 1;
            return (*this);
        }

        /// Postfix decrement
        template_iterator operator--(difference_type) {
            template_iterator tmp = *this;
            (*this) -= 1;
            return tmp;
        }

        /// Prefix decrement
        template_iterator &operator--() {
            (*this) -= 1;
            return (*this);
        }

        /// Difference
        friend difference_type operator-(const template_iterator &a, const template_iterator &b) {
            return normalizedAddition(a.current_pos,
                       -1 * static_cast<int>(*a.begin_pos))
                - normalizedAddition(b.current_pos,
                    -1 * static_cast<int>(*b.begin_pos));
        };

        /// Comparison
        friend std::strong_ordering operator<=>(const template_iterator &a, const template_iterator &b) {
            return normalizedAddition(a.current_pos,
                       -1 * static_cast<int>(*a.begin_pos))
                <=> normalizedAddition(b.current_pos,
                    -1 * static_cast<int>(*b.begin_pos));
        };
        friend bool operator==(const template_iterator &a, const template_iterator &b) { return a.current_pos == b.current_pos; };

        /// Conversion to const_iterator
        operator template_iterator<const element_type>() const { return template_iterator<const element_type>(data, current_pos, *begin_pos); };
    };
    typedef template_iterator<T> iterator;
    typedef template_iterator<const T> const_iterator;
    static_assert(std::random_access_iterator<iterator>, "Iterator is not complete");
    static_assert(std::random_access_iterator<const_iterator>, "Const iterator is not complete");

    CircleBuffer() { clear(); }

    // Get iterators
    auto begin() { return iterator(data, begin_pos, begin_pos); }
    auto end() { return iterator(data, end_pos, begin_pos); }
    auto rbegin() { return std::reverse_iterator(iterator(data, end_pos, begin_pos)); }
    auto rend() { return std::reverse_iterator(iterator(data, begin_pos, begin_pos)); }

    // Get const iterators
    auto begin() const { return const_iterator(data, begin_pos, begin_pos); }
    auto end() const { return const_iterator(data, end_pos, begin_pos); }
    auto rbegin() const { return std::reverse_iterator(const_iterator(data, end_pos, begin_pos)); }
    auto rend() const { return std::reverse_iterator(const_iterator(data, begin_pos, begin_pos)); }

    // [] operator
    T &operator[](int offset) { return begin()[offset]; }
    const T &operator[](int offset) const { return begin()[offset]; }

    size_t size() const { return SIZE - 1; }
    void clear() { begin_pos = end_pos = 0; }
    void push_back(T elem);
    bool push_back_DontRewrite(T elem);
    size_t Count() const { return (end_pos + SIZE - begin_pos) % SIZE; }
    bool IsEmpty() const { return begin_pos == end_pos; }

    constexpr size_t Size() const { return SIZE; }

    bool ConsumeFirst(T &elem); // data must be processed before next push_back
    bool ConsumeLast(T &elem); // data must be processed before next push_back
    const T &GetFirstIfAble() const; // data must be processed before next push_back, must not be empty
    const T &GetLastIfAble() const; // data must be processed before next push_back, must not be empty
};

template <class T, size_t SIZE>
void CircleBuffer<T, SIZE>::push_back(T elem) {
    data[end_pos] = elem;
    incrementIndex(end_pos);
    if (begin_pos == end_pos) { // begin just was erased, set new begin
        incrementIndex(begin_pos);
    }
}

template <class T, size_t SIZE>
bool CircleBuffer<T, SIZE>::push_back_DontRewrite(T elem) {
    size_t index = end_pos;
    incrementIndex(index);
    if (begin_pos == index) { // full
        return false;
    } else {
        data[index] = elem;
        end_pos = index;
        return true;
    }
}

template <class T, size_t SIZE>
bool CircleBuffer<T, SIZE>::ConsumeFirst(T &elem) {
    if (IsEmpty()) {
        return false;
    }
    elem = GetFirstIfAble();
    incrementIndex(begin_pos);
    return true;
}

template <class T, size_t SIZE>
bool CircleBuffer<T, SIZE>::ConsumeLast(T &elem) {
    if (IsEmpty()) {
        return false;
    }
    elem = GetLastIfAble();
    decrementIndex(end_pos);
    return true;
}

template <class T, size_t SIZE>
const T &CircleBuffer<T, SIZE>::GetFirstIfAble() const {
    return data[begin_pos];
}

template <class T, size_t SIZE>
const T &CircleBuffer<T, SIZE>::GetLastIfAble() const {
    size_t index = end_pos;
    decrementIndex(index);
    return data[index];
}

/*****************************************************************************/
// circular buffer for strings (T == std::array<char, MAX_LENGTH>)
#include <array>

template <size_t MAX_LENGTH>
class Message {
    std::array<char, MAX_LENGTH> arr;

public:
    Message() { arr.fill('\0'); }
    Message(const char *msg) {
        strlcpy(arr.data(), msg, MAX_LENGTH);
    }

    operator const char *() const {
        return arr.data();
    }
};

template <size_t SIZE, size_t MAX_LENGTH>
using CircleStringBuffer = CircleBuffer<Message<MAX_LENGTH>, SIZE>;
