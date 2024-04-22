// circle_buffer.hpp
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <iterator>

class CircleBufferBase {

public:
    /**
     * @brief Add and normalize index to be in range [0, capacity).
     * Efficient for small deviations from the range.
     * @param index index
     * @param add addition to index
     * @return normalized result
     */
    size_t normalizedAddition(size_t index, int add) const {
        add += static_cast<int>(index);
        while (add >= static_cast<int>(capacity_)) {
            add -= capacity_;
        }
        while (add < 0) {
            add += capacity_;
        }
        return add;
    }

public:
    /// Returns number of elements in the circle buffer
    size_t size() const {
        return (end_pos + capacity_ - begin_pos) % capacity_;
    }

    bool IsEmpty() const {
        return begin_pos == end_pos;
    }

    size_t capacity() const {
        return capacity_ - 1;
    }

    void clear() {
        begin_pos = end_pos = 0;
    }

protected:
    CircleBufferBase(size_t capacity)
        : capacity_(capacity) {
    }

protected:
    const size_t capacity_;
    size_t begin_pos = 0; // position of first element
    size_t end_pos = 0; // position behind last element == write position
};

template <class T>
class CircleBufferBaseT : public CircleBufferBase {

public:
    using Elem = T;

public:
    template <bool is_const_iter> // Should be "T" or "const T"
    class template_iterator {
        friend class CircleBufferBaseT<T>;

    public:
        using iterator_concept [[maybe_unused]] = std::random_access_iterator_tag;
        using iterator_category = std::random_access_iterator_tag; // Needed for older GCC
        using difference_type = int;
        using element_type = std::conditional_t<is_const_iter, const T, T>;
        using pointer = element_type *;
        using reference = element_type &;

        using Buffer = std::conditional_t<is_const_iter, const CircleBufferBaseT<T>, CircleBufferBaseT<T>>;

    public:
        template_iterator() = default;
        template_iterator(const template_iterator &) = default;

        template_iterator &operator=(const template_iterator &) = default;

        /// Dereference
        reference operator*() const { return buffer->data[current_pos]; }
        pointer operator->() const { return &buffer->data[current_pos]; }

        /// Addition
        template_iterator operator+(difference_type add) const {
            return template_iterator(buffer, buffer->normalizedAddition(current_pos, add));
        }

        friend template_iterator operator+(difference_type add, const template_iterator &other) { return other.operator+(add); }
        template_iterator operator+(const template_iterator &other) const { return operator+(other.current_pos); }
        reference operator[](difference_type add) const { return *operator+(add); }

        size_t position() const {
            return buffer->normalizedAddition(current_pos, -static_cast<int>(buffer->begin_pos));
        }

        /// Addition assignment
        template_iterator &operator+=(difference_type add) {
            current_pos = buffer->normalizedAddition(current_pos, add);
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
            return a.position() - b.position();
        };

        /// Comparison
        friend std::strong_ordering operator<=>(const template_iterator &a, const template_iterator &b) {
            return a.position() <=> b.position();
        };

        friend bool operator==(const template_iterator &, const template_iterator &) = default;

        /// Conversion to const_iterator
        operator template_iterator<true>() const { return template_iterator<true>(buffer, current_pos); };

    private:
        template_iterator(Buffer *buffer, size_t current_pos_)
            : buffer(buffer)
            , current_pos(current_pos_) {}

    private:
        Buffer *buffer = nullptr; ///< Pointer to the data array
        size_t current_pos = 0; ///< Iterator position
    };
    typedef template_iterator<false> iterator;
    typedef template_iterator<true> const_iterator;
    static_assert(std::random_access_iterator<iterator>, "Iterator is not complete");
    static_assert(std::random_access_iterator<const_iterator>, "Const iterator is not complete");

public:
    // Get iterators
    auto begin() { return iterator(this, begin_pos); }
    auto end() { return iterator(this, end_pos); }
    auto rbegin() { return std::reverse_iterator(iterator(this, end_pos)); }
    auto rend() { return std::reverse_iterator(iterator(this, begin_pos)); }

    // Get const iterators
    auto begin() const { return const_iterator(this, begin_pos); }
    auto end() const { return const_iterator(this, end_pos); }
    auto rbegin() const { return std::reverse_iterator(const_iterator(this, end_pos)); }
    auto rend() const { return std::reverse_iterator(const_iterator(this, begin_pos)); }

    // [] operator
    T &operator[](int offset) { return begin()[offset]; }
    const T &operator[](int offset) const { return begin()[offset]; }

    void push_back(T elem) {
        data[end_pos] = elem;
        end_pos = normalizedAddition(end_pos, 1);

        // Buffer is now full, ditch the last element
        if (begin_pos == end_pos) {
            begin_pos = normalizedAddition(begin_pos, 1);
        }
    }

    // data must be processed before next push_back
    bool ConsumeFirst(T &elem) {
        if (IsEmpty()) {
            return false;
        }
        elem = GetFirstIfAble();
        begin_pos = normalizedAddition(begin_pos, 1);
        return true;
    }

    // data must be processed before next push_back
    bool ConsumeLast(T &elem) {
        if (IsEmpty()) {
            return false;
        }
        elem = GetLastIfAble();
        end_pos = normalizedAddition(end_pos, -1);
        return true;
    }

    // data must be processed before next push_back, must not be empty
    const T &GetFirstIfAble() const {
        return data[begin_pos];
    }

    // data must be processed before next push_back, must not be empty
    const T &GetLastIfAble() const {
        return data[normalizedAddition(end_pos, -1)];
    }

protected:
    CircleBufferBaseT(Elem *data, size_t capacity)
        : CircleBufferBase(capacity)
        , data(data) {
    }

protected:
    Elem *data;
};

/*****************************************************************************/
// general circular buffer
// you can never use entire size
// because write position (end) cannot be equal to begin
// because begin == end == empty
template <class T, size_t capacity>
class CircleBuffer : public CircleBufferBaseT<T> {

public:
    CircleBuffer()
        : CircleBufferBaseT<T>(data_buffer.data(), capacity) {
        this->clear();
    }

protected:
    std::array<T, capacity> data_buffer;
};
