#ifndef SFL_TEST_XOBJ_HPP
#define SFL_TEST_XOBJ_HPP

namespace sfl
{
namespace test
{

class xobj
{
private:

    int* value_;

public:

    explicit xobj(int value) noexcept
    {
        value_ = new int(value);
    }

    xobj(const xobj& other) noexcept
    {
        value_ = new int(*other.value_);
    }

    xobj(xobj&& other) noexcept
    {
        value_ = new int(*other.value_);
        *other.value_ = -*other.value_;
    }

    xobj& operator=(const xobj& other) noexcept
    {
        *value_ = *other.value_;
        return *this;
    }

    xobj& operator=(xobj&& other) noexcept
    {
        *value_ = *other.value_;
        *other.value_ = -*other.value_;
        return *this;
    }

    ~xobj()
    {
        delete value_;
    }

    int value() const
    {
        return *value_;
    }

public:

    struct less
    {
        using is_transparent = void;

        bool operator()(const xobj& a, const xobj& b) const
        {
            return a.value() < b.value();
        }

        bool operator()(const xobj& a, int b) const
        {
            return a.value() < b;
        }

        bool operator()(int a, const xobj& b) const
        {
            return a < b.value();
        }
    };

    struct equal
    {
        using is_transparent = void;

        bool operator()(const xobj& a, const xobj& b) const
        {
            return a.value() == b.value();
        }

        bool operator()(const xobj& a, int b) const
        {
            return a.value() == b;
        }

        bool operator()(int a, const xobj& b) const
        {
            return a == b.value();
        }
    };
};

} // namespace test
} // namespace sfl

#endif // SFL_TEST_XOBJ_HPP
