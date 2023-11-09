#pragma once
namespace ConstexprQuickSort {

// implementation of constexpr sort because we don't have const expr sort in C++17
// source: https://tristanbrindle.com/posts/a-more-useful-compile-time-quicksort
template <class ForwardIt1, class ForwardIt2>
constexpr void iter_swap(ForwardIt1 a, ForwardIt2 b) {
    auto temp = std::move(*a);
    *a = std::move(*b);
    *b = std::move(temp);
}

template <class InputIt, class UnaryPredicate>
constexpr InputIt find_if_not(InputIt first, InputIt last, UnaryPredicate q) {
    for (; first != last; ++first) {
        if (!q(*first)) {
            return first;
        }
    }
    return last;
}

template <class ForwardIt, class UnaryPredicate>
constexpr ForwardIt partition(ForwardIt first, ForwardIt last, UnaryPredicate p) {
    first = find_if_not(first, last, p);
    if (first == last) {
        return first;
    }

    for (ForwardIt i = std::next(first); i != last; ++i) {
        if (p(*i)) {
            iter_swap(i, first);
            ++first;
        }
    }
    return first;
}

template <class RAIt, class Compare = std::less<>>
constexpr void quick_sort(RAIt first, RAIt last, Compare cmp = Compare {}) {
    auto const N = std::distance(first, last);
    if (N <= 1) {
        return;
    }
    auto const pivot = *std::next(first, N / 2);
    auto const middle1 = partition(first, last, [=](auto const &elem) {
        return cmp(elem, pivot);
    });
    auto const middle2 = partition(middle1, last, [=](auto const &elem) {
        return !cmp(pivot, elem);
    });
    quick_sort(first, middle1, cmp);
    quick_sort(middle2, last, cmp);
}

template <typename Range, class Compare>
constexpr auto sort(Range &&range, Compare cmp = Compare {}) {
    quick_sort(std::begin(range), std::end(range), cmp);
    return range;
}
} // namespace ConstexprQuickSort
