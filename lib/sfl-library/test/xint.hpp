#ifndef SFL_TEST_XINT_HPP
#define SFL_TEST_XINT_HPP

#include "print.hpp"

#define SFL_TEST_XINT_DEFAULT_VALUE 789456123

namespace sfl
{
namespace test
{

class xint
{
private:

    static int counter_;
    int* value_;

public:

    xint() noexcept
    {
        ++counter_;
        value_ = new int(SFL_TEST_XINT_DEFAULT_VALUE);
        PRINT("  ++ xint::xint() [value = " << *value_ << "]");
    }

    xint(int value) noexcept
    {
        ++counter_;
        value_ = new int(value);
        PRINT("  ++ xint::xint(int) [value = " << *value_ << "]");
    }

    xint(const xint& other) noexcept
    {
        ++counter_;
        value_ = new int(*other.value_);
        PRINT("  ++ xint::xint(const xint&) [value = " << *value_ << "]");
    }

    xint(xint&& other) noexcept
    {
        ++counter_;
        value_ = new int(*other.value_);
        *other.value_ = -*other.value_;
        PRINT("  ++ xint::xint(xint&&) [value = " << *value_ << "]");
    }

    xint& operator=(const xint& other) noexcept
    {
        *value_ = *other.value_;
        PRINT("  ++ xint::operator=(const xint&) [value = " << *value_ << "]");
        return *this;
    }

    xint& operator=(xint&& other) noexcept
    {
        *value_ = *other.value_;
        *other.value_ = -*other.value_;
        PRINT("  ++ xint::operator=(xint&&) [value = " << *value_ << "]");
        return *this;
    }

    ~xint()
    {
        PRINT("  ++ xint::~xint() [value = " << *value_ << "]");
        delete value_;

        --counter_;

        if (counter_ < 0)
        {
            PRINT("ERROR: xint::~xint(): counter = " << counter_ << " < 0.");
            std::abort();
        }
    }

    friend bool operator==(const xint& x, const xint& y)
    {
        return *x.value_ == *y.value_;
    }

    friend bool operator==(int x, const xint& y)
    {
        return x == *y.value_;
    }

    friend bool operator==(const xint& x, int y)
    {
        return *x.value_ == y;
    }

    friend bool operator<(const xint& x, const xint& y)
    {
        return *x.value_ < *y.value_;
    }

    friend bool operator<(int x, const xint& y)
    {
        return x < *y.value_;
    }

    friend bool operator<(const xint& x, int y)
    {
        return *x.value_ < y;
    }
};

int xint::counter_ = 0;

} // namespace test
} // namespace sfl

#endif // SFL_TEST_XINT_HPP
