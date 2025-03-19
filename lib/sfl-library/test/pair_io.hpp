#ifndef SFL_TEST_PAIR_IO_HPP
#define SFL_TEST_PAIR_IO_HPP

#include <iosfwd>
#include <utility>

namespace std
{

template <typename T1, typename T2>
istream& operator>>(istream& is, pair<T1, T2>& p)
{
    return is >> p.first >> p.second;
}

template <typename T1, typename T2>
ostream& operator>>(ostream& os, const pair<T1, T2>& p)
{
    return os << p.first << p.second;
}

} // namespace std

#endif // SFL_TEST_PAIR_IO_HPP
