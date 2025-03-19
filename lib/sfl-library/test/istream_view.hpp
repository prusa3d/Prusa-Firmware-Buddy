#ifndef SFL_TEST_ISTREAM_VIEW_HPP
#define SFL_TEST_ISTREAM_VIEW_HPP

#include <istream>
#include <iterator>

namespace sfl
{

namespace test
{

template <typename T>
class istream_view
{
private:

    std::istream_iterator<T> begin_;
    std::istream_iterator<T> end_;

public:

    istream_view(std::istream& is)
        : begin_(std::istream_iterator<T>(is))
        , end_(std::istream_iterator<T>())
    {}

    std::istream_iterator<T>& begin()
    {
        return begin_;
    }

    const std::istream_iterator<T>& begin() const
    {
        return begin_;
    }

    std::istream_iterator<T>& end()
    {
        return end_;
    }

    const std::istream_iterator<T>& end() const
    {
        return end_;
    }
};

} // namespace test

} // namespace sfl

#endif // SFL_TEST_ISTREAM_VIEW_HPP
