//
// g++ -std=c++11 -g -O0 -Wall -Wextra -Wpedantic -Werror -Wfatal-errors -I ../include static_flat_multimap.cpp
// valgrind --leak-check=full ./a.out
//

#undef NDEBUG // This is very important. Must be in the first line.

#define SFL_TEST_STATIC_FLAT_MULTIMAP
#include "sfl/static_flat_multimap.hpp"

#include "check.hpp"
#include "istream_view.hpp"
#include "nth.hpp"
#include "pair_io.hpp"
#include "print.hpp"

#include "xint.hpp"
#include "xobj.hpp"

#include <sstream>
#include <vector>

void test_static_flat_multimap()
{
    using sfl::test::xint;
    using sfl::test::xobj;

    PRINT("Test PRIVATE member function insert_exactly_at(const_iterator, Args&&...)");
    {
        // Insert at the end
        {
            sfl::static_flat_multimap<xint, xint, 5, std::less<xint>> map;

            using value_type = std::pair<xint, xint>;

            {
                CHECK(map.empty() == true);
                CHECK(map.full() == false);
                CHECK(map.size() == 0);
                CHECK(map.capacity() == 5);
                CHECK(map.available() == 5);
            }

            {
                PRINT(">");
                const auto res = map.insert_exactly_at(map.end(), value_type(10, 1));
                PRINT("<");

                CHECK(res == map.nth(0));
                CHECK(map.empty() == false);
                CHECK(map.full() == false);
                CHECK(map.size() == 1);
                CHECK(map.capacity() == 5);
                CHECK(map.available() == 4);
                CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            }

            {
                PRINT(">");
                const auto res = map.insert_exactly_at(map.end(), value_type(20, 1));
                PRINT("<");

                CHECK(res == map.nth(1));
                CHECK(map.empty() == false);
                CHECK(map.full() == false);
                CHECK(map.size() == 2);
                CHECK(map.capacity() == 5);
                CHECK(map.available() == 3);
                CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
                CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
            }

            {
                PRINT(">");
                const auto res = map.insert_exactly_at(map.end(), value_type(30, 1));
                PRINT("<");

                CHECK(res == map.nth(2));
                CHECK(map.empty() == false);
                CHECK(map.full() == false);
                CHECK(map.size() == 3);
                CHECK(map.capacity() == 5);
                CHECK(map.available() == 2);
                CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
                CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
                CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
            }

            {
                PRINT(">");
                const auto res = map.insert_exactly_at(map.end(), value_type(40, 1));
                PRINT("<");

                CHECK(res == map.nth(3));
                CHECK(map.empty() == false);
                CHECK(map.full() == false);
                CHECK(map.size() == 4);
                CHECK(map.capacity() == 5);
                CHECK(map.available() == 1);
                CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
                CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
                CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
                CHECK(map.nth(3)->first == 40); CHECK(map.nth(3)->second == 1);
            }

            {
                PRINT(">");
                const auto res = map.insert_exactly_at(map.end(), value_type(50, 1));
                PRINT("<");

                CHECK(res == map.nth(4));
                CHECK(map.empty() == false);
                CHECK(map.full() == true);
                CHECK(map.size() == 5);
                CHECK(map.capacity() == 5);
                CHECK(map.available() == 0);
                CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
                CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
                CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
                CHECK(map.nth(3)->first == 40); CHECK(map.nth(3)->second == 1);
                CHECK(map.nth(4)->first == 50); CHECK(map.nth(4)->second == 1);
            }
        }

        // Insert at the begin
        {
            sfl::static_flat_multimap<xint, xint, 5, std::less<xint>> map;

            using value_type = std::pair<xint, xint>;

            {
                CHECK(map.empty() == true);
                CHECK(map.full() == false);
                CHECK(map.size() == 0);
                CHECK(map.capacity() == 5);
                CHECK(map.available() == 5);
            }

            {
                PRINT(">");
                const auto res = map.insert_exactly_at(map.begin(), value_type(50, 1));
                PRINT("<");

                CHECK(res == map.nth(0));
                CHECK(map.empty() == false);
                CHECK(map.full() == false);
                CHECK(map.size() == 1);
                CHECK(map.capacity() == 5);
                CHECK(map.available() == 4);
                CHECK(map.nth(0)->first == 50); CHECK(map.nth(0)->second == 1);
            }

            {
                PRINT(">");
                const auto res = map.insert_exactly_at(map.begin(), value_type(40, 1));
                PRINT("<");

                CHECK(res == map.nth(0));
                CHECK(map.empty() == false);
                CHECK(map.full() == false);
                CHECK(map.size() == 2);
                CHECK(map.capacity() == 5);
                CHECK(map.available() == 3);
                CHECK(map.nth(0)->first == 40); CHECK(map.nth(0)->second == 1);
                CHECK(map.nth(1)->first == 50); CHECK(map.nth(1)->second == 1);
            }

            {
                PRINT(">");
                const auto res = map.insert_exactly_at(map.begin(), value_type(30, 1));
                PRINT("<");

                CHECK(res == map.nth(0));
                CHECK(map.empty() == false);
                CHECK(map.full() == false);
                CHECK(map.size() == 3);
                CHECK(map.capacity() == 5);
                CHECK(map.available() == 2);
                CHECK(map.nth(0)->first == 30); CHECK(map.nth(0)->second == 1);
                CHECK(map.nth(1)->first == 40); CHECK(map.nth(1)->second == 1);
                CHECK(map.nth(2)->first == 50); CHECK(map.nth(2)->second == 1);
            }

            {
                PRINT(">");
                const auto res = map.insert_exactly_at(map.begin(), value_type(20, 1));
                PRINT("<");

                CHECK(res == map.nth(0));
                CHECK(map.empty() == false);
                CHECK(map.full() == false);
                CHECK(map.size() == 4);
                CHECK(map.capacity() == 5);
                CHECK(map.available() == 1);
                CHECK(map.nth(0)->first == 20); CHECK(map.nth(0)->second == 1);
                CHECK(map.nth(1)->first == 30); CHECK(map.nth(1)->second == 1);
                CHECK(map.nth(2)->first == 40); CHECK(map.nth(2)->second == 1);
                CHECK(map.nth(3)->first == 50); CHECK(map.nth(3)->second == 1);
            }

            {
                PRINT(">");
                const auto res = map.insert_exactly_at(map.begin(), value_type(10, 1));
                PRINT("<");

                CHECK(res == map.nth(0));
                CHECK(map.empty() == false);
                CHECK(map.full() == true);
                CHECK(map.size() == 5);
                CHECK(map.capacity() == 5);
                CHECK(map.available() == 0);
                CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
                CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
                CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
                CHECK(map.nth(3)->first == 40); CHECK(map.nth(3)->second == 1);
                CHECK(map.nth(4)->first == 50); CHECK(map.nth(4)->second == 1);
            }
        }

        {
            sfl::static_flat_multimap<xint, xint, 5, std::less<xint>> map;

            using value_type = std::pair<xint, xint>;

            map.insert_exactly_at(map.end(), value_type(10, 1));
            map.insert_exactly_at(map.end(), value_type(20, 1));
            map.insert_exactly_at(map.end(), value_type(30, 1));
            map.insert_exactly_at(map.end(), value_type(40, 1));

            CHECK(map.empty() == false);
            CHECK(map.full() == false);
            CHECK(map.size() == 4);
            CHECK(map.capacity() == 5);
            CHECK(map.available() == 1);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
            CHECK(map.nth(3)->first == 40); CHECK(map.nth(3)->second == 1);

            const auto res = map.insert_exactly_at(map.nth(0), value_type(5, 1));

            CHECK(res == map.nth(0));
            CHECK(map.empty() == false);
            CHECK(map.full() == true);
            CHECK(map.size() == 5);
            CHECK(map.capacity() == 5);
            CHECK(map.available() == 0);
            CHECK(map.nth(0)->first ==  5); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 10); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 20); CHECK(map.nth(2)->second == 1);
            CHECK(map.nth(3)->first == 30); CHECK(map.nth(3)->second == 1);
            CHECK(map.nth(4)->first == 40); CHECK(map.nth(4)->second == 1);
        }

        {
            sfl::static_flat_multimap<xint, xint, 5, std::less<xint>> map;

            using value_type = std::pair<xint, xint>;

            map.insert_exactly_at(map.end(), value_type(10, 1));
            map.insert_exactly_at(map.end(), value_type(20, 1));
            map.insert_exactly_at(map.end(), value_type(30, 1));
            map.insert_exactly_at(map.end(), value_type(40, 1));

            CHECK(map.empty() == false);
            CHECK(map.full() == false);
            CHECK(map.size() == 4);
            CHECK(map.capacity() == 5);
            CHECK(map.available() == 1);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
            CHECK(map.nth(3)->first == 40); CHECK(map.nth(3)->second == 1);

            const auto res = map.insert_exactly_at(map.nth(1), value_type(15, 1));

            CHECK(res == map.nth(1));
            CHECK(map.empty() == false);
            CHECK(map.full() == true);
            CHECK(map.size() == 5);
            CHECK(map.capacity() == 5);
            CHECK(map.available() == 0);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 15); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 20); CHECK(map.nth(2)->second == 1);
            CHECK(map.nth(3)->first == 30); CHECK(map.nth(3)->second == 1);
            CHECK(map.nth(4)->first == 40); CHECK(map.nth(4)->second == 1);
        }

        {
            sfl::static_flat_multimap<xint, xint, 5, std::less<xint>> map;

            using value_type = std::pair<xint, xint>;

            map.insert_exactly_at(map.end(), value_type(10, 1));
            map.insert_exactly_at(map.end(), value_type(20, 1));
            map.insert_exactly_at(map.end(), value_type(30, 1));
            map.insert_exactly_at(map.end(), value_type(40, 1));

            CHECK(map.empty() == false);
            CHECK(map.full() == false);
            CHECK(map.size() == 4);
            CHECK(map.capacity() == 5);
            CHECK(map.available() == 1);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
            CHECK(map.nth(3)->first == 40); CHECK(map.nth(3)->second == 1);

            const auto res = map.insert_exactly_at(map.nth(2), value_type(25, 1));

            CHECK(res == map.nth(2));
            CHECK(map.empty() == false);
            CHECK(map.full() == true);
            CHECK(map.size() == 5);
            CHECK(map.capacity() == 5);
            CHECK(map.available() == 0);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 25); CHECK(map.nth(2)->second == 1);
            CHECK(map.nth(3)->first == 30); CHECK(map.nth(3)->second == 1);
            CHECK(map.nth(4)->first == 40); CHECK(map.nth(4)->second == 1);
        }

        {
            sfl::static_flat_multimap<xint, xint, 5, std::less<xint>> map;

            using value_type = std::pair<xint, xint>;

            map.insert_exactly_at(map.end(), value_type(10, 1));
            map.insert_exactly_at(map.end(), value_type(20, 1));
            map.insert_exactly_at(map.end(), value_type(30, 1));
            map.insert_exactly_at(map.end(), value_type(40, 1));

            CHECK(map.empty() == false);
            CHECK(map.full() == false);
            CHECK(map.size() == 4);
            CHECK(map.capacity() == 5);
            CHECK(map.available() == 1);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
            CHECK(map.nth(3)->first == 40); CHECK(map.nth(3)->second == 1);

            const auto res = map.insert_exactly_at(map.nth(3), value_type(35, 1));

            CHECK(res == map.nth(3));
            CHECK(map.empty() == false);
            CHECK(map.full() == true);
            CHECK(map.size() == 5);
            CHECK(map.capacity() == 5);
            CHECK(map.available() == 0);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
            CHECK(map.nth(3)->first == 35); CHECK(map.nth(3)->second == 1);
            CHECK(map.nth(4)->first == 40); CHECK(map.nth(4)->second == 1);
        }

        {
            sfl::static_flat_multimap<xint, xint, 5, std::less<xint>> map;

            using value_type = std::pair<xint, xint>;

            map.insert_exactly_at(map.end(), value_type(10, 1));
            map.insert_exactly_at(map.end(), value_type(20, 1));
            map.insert_exactly_at(map.end(), value_type(30, 1));
            map.insert_exactly_at(map.end(), value_type(40, 1));

            CHECK(map.empty() == false);
            CHECK(map.full() == false);
            CHECK(map.size() == 4);
            CHECK(map.capacity() == 5);
            CHECK(map.available() == 1);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
            CHECK(map.nth(3)->first == 40); CHECK(map.nth(3)->second == 1);

            const auto res = map.insert_exactly_at(map.nth(4), value_type(45, 1));

            CHECK(res == map.nth(4));
            CHECK(map.empty() == false);
            CHECK(map.full() == true);
            CHECK(map.size() == 5);
            CHECK(map.capacity() == 5);
            CHECK(map.available() == 0);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
            CHECK(map.nth(3)->first == 40); CHECK(map.nth(3)->second == 1);
            CHECK(map.nth(4)->first == 45); CHECK(map.nth(4)->second == 1);
        }
    }

    PRINT("Test PRIVATE member function is_insert_hint_good(const_iterator, const Value&)");
    {
        sfl::static_flat_multimap<xint, xint, 100, std::less<xint>> map;

        using value_type = std::pair<xint, xint>;

        ///////////////////////////////////////////////////////////////////////////

        map.insert_exactly_at(map.end(), value_type(20, 1));
        map.insert_exactly_at(map.end(), value_type(40, 1));
        map.insert_exactly_at(map.end(), value_type(40, 2));
        map.insert_exactly_at(map.end(), value_type(40, 3));
        map.insert_exactly_at(map.end(), value_type(60, 1));

        CHECK(map.size() == 5);
        CHECK(map.nth(0)->first == 20); CHECK(map.nth(0)->second == 1);
        CHECK(map.nth(1)->first == 40); CHECK(map.nth(1)->second == 1);
        CHECK(map.nth(2)->first == 40); CHECK(map.nth(2)->second == 2);
        CHECK(map.nth(3)->first == 40); CHECK(map.nth(3)->second == 3);
        CHECK(map.nth(4)->first == 60); CHECK(map.nth(4)->second == 1);

        ///////////////////////////////////////////////////////////////////////////

        CHECK(map.is_insert_hint_good(map.nth(0), value_type(20, 1)) == true);
        CHECK(map.is_insert_hint_good(map.nth(1), value_type(20, 1)) == true);
        CHECK(map.is_insert_hint_good(map.nth(2), value_type(20, 1)) == false);
        CHECK(map.is_insert_hint_good(map.nth(3), value_type(20, 1)) == false);
        CHECK(map.is_insert_hint_good(map.nth(4), value_type(20, 1)) == false);
        CHECK(map.is_insert_hint_good(map.nth(5), value_type(20, 1)) == false);

        CHECK(map.is_insert_hint_good(map.nth(0), value_type(40, 1)) == false);
        CHECK(map.is_insert_hint_good(map.nth(1), value_type(40, 1)) == true);
        CHECK(map.is_insert_hint_good(map.nth(2), value_type(40, 1)) == true);
        CHECK(map.is_insert_hint_good(map.nth(3), value_type(40, 1)) == true);
        CHECK(map.is_insert_hint_good(map.nth(4), value_type(40, 1)) == true);
        CHECK(map.is_insert_hint_good(map.nth(5), value_type(40, 1)) == false);

        CHECK(map.is_insert_hint_good(map.nth(0), value_type(60, 1)) == false);
        CHECK(map.is_insert_hint_good(map.nth(1), value_type(60, 1)) == false);
        CHECK(map.is_insert_hint_good(map.nth(2), value_type(60, 1)) == false);
        CHECK(map.is_insert_hint_good(map.nth(3), value_type(60, 1)) == false);
        CHECK(map.is_insert_hint_good(map.nth(4), value_type(60, 1)) == true);
        CHECK(map.is_insert_hint_good(map.nth(5), value_type(60, 1)) == true);

        ///////////////////////////////////////////////////////////////////////////

        CHECK(map.is_insert_hint_good(map.nth(0), value_type(10, 1)) == true);
        CHECK(map.is_insert_hint_good(map.nth(1), value_type(10, 1)) == false);
        CHECK(map.is_insert_hint_good(map.nth(2), value_type(10, 1)) == false);
        CHECK(map.is_insert_hint_good(map.nth(3), value_type(10, 1)) == false);
        CHECK(map.is_insert_hint_good(map.nth(4), value_type(10, 1)) == false);
        CHECK(map.is_insert_hint_good(map.nth(5), value_type(10, 1)) == false);

        CHECK(map.is_insert_hint_good(map.nth(0), value_type(30, 1)) == false);
        CHECK(map.is_insert_hint_good(map.nth(1), value_type(30, 1)) == true);
        CHECK(map.is_insert_hint_good(map.nth(2), value_type(30, 1)) == false);
        CHECK(map.is_insert_hint_good(map.nth(3), value_type(30, 1)) == false);
        CHECK(map.is_insert_hint_good(map.nth(4), value_type(30, 1)) == false);
        CHECK(map.is_insert_hint_good(map.nth(5), value_type(30, 1)) == false);

        CHECK(map.is_insert_hint_good(map.nth(0), value_type(50, 1)) == false);
        CHECK(map.is_insert_hint_good(map.nth(1), value_type(50, 1)) == false);
        CHECK(map.is_insert_hint_good(map.nth(2), value_type(50, 1)) == false);
        CHECK(map.is_insert_hint_good(map.nth(3), value_type(50, 1)) == false);
        CHECK(map.is_insert_hint_good(map.nth(4), value_type(50, 1)) == true);
        CHECK(map.is_insert_hint_good(map.nth(5), value_type(50, 1)) == false);

        CHECK(map.is_insert_hint_good(map.nth(0), value_type(70, 1)) == false);
        CHECK(map.is_insert_hint_good(map.nth(1), value_type(70, 1)) == false);
        CHECK(map.is_insert_hint_good(map.nth(2), value_type(70, 1)) == false);
        CHECK(map.is_insert_hint_good(map.nth(3), value_type(70, 1)) == false);
        CHECK(map.is_insert_hint_good(map.nth(4), value_type(70, 1)) == false);
        CHECK(map.is_insert_hint_good(map.nth(5), value_type(70, 1)) == true);
    }

    ///////////////////////////////////////////////////////////////////////////////

    PRINT("Test begin, end, cbegin, cend, rbegin, rend, crbegin, crend, nth, index_of");
    {
        sfl::static_flat_multimap<xint, xint, 100, std::less<xint>> map;

        using value_type = std::pair<xint, xint>;

        map.insert_exactly_at(map.end(), value_type(20, 1));
        map.insert_exactly_at(map.end(), value_type(40, 1));
        map.insert_exactly_at(map.end(), value_type(60, 1));

        CHECK(map.size() == 3);
        CHECK(map.nth(0)->first == 20); CHECK(map.nth(0)->second == 1);
        CHECK(map.nth(1)->first == 40); CHECK(map.nth(1)->second == 1);
        CHECK(map.nth(2)->first == 60); CHECK(map.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        auto it = map.begin();
        CHECK(it->first == 20); CHECK(it->second == 1); ++it;
        CHECK(it->first == 40); CHECK(it->second == 1); ++it;
        CHECK(it->first == 60); CHECK(it->second == 1); ++it;
        CHECK(it == map.end());

        ///////////////////////////////////////////////////////////////////////

        auto cit = map.cbegin();
        CHECK(cit->first == 20); CHECK(cit->second == 1); ++cit;
        CHECK(cit->first == 40); CHECK(cit->second == 1); ++cit;
        CHECK(cit->first == 60); CHECK(cit->second == 1); ++cit;
        CHECK(cit == map.cend());

        ///////////////////////////////////////////////////////////////////////

        auto rit = map.rbegin();
        CHECK(rit->first == 60); CHECK(rit->second == 1); ++rit;
        CHECK(rit->first == 40); CHECK(rit->second == 1); ++rit;
        CHECK(rit->first == 20); CHECK(rit->second == 1); ++rit;
        CHECK(rit == map.rend());

        ///////////////////////////////////////////////////////////////////////

        auto crit = map.crbegin();
        CHECK(crit->first == 60); CHECK(crit->second == 1); ++crit;
        CHECK(crit->first == 40); CHECK(crit->second == 1); ++crit;
        CHECK(crit->first == 20); CHECK(crit->second == 1); ++crit;
        CHECK(crit == map.crend());

        ///////////////////////////////////////////////////////////////////////

        CHECK(map.nth(0)->first == 20); CHECK(map.nth(0)->second == 1);
        CHECK(map.nth(1)->first == 40); CHECK(map.nth(1)->second == 1);
        CHECK(map.nth(2)->first == 60); CHECK(map.nth(2)->second == 1);
        CHECK(map.nth(3) == map.end());

        ///////////////////////////////////////////////////////////////////////

        CHECK(std::next(map.begin(), 0) == map.nth(0));
        CHECK(std::next(map.begin(), 1) == map.nth(1));
        CHECK(std::next(map.begin(), 2) == map.nth(2));
        CHECK(std::next(map.begin(), 3) == map.nth(3));

        ///////////////////////////////////////////////////////////////////////

        CHECK(std::next(map.cbegin(), 0) == map.nth(0));
        CHECK(std::next(map.cbegin(), 1) == map.nth(1));
        CHECK(std::next(map.cbegin(), 2) == map.nth(2));
        CHECK(std::next(map.cbegin(), 3) == map.nth(3));

        ///////////////////////////////////////////////////////////////////////

        CHECK(map.nth(0) < map.nth(1));
        CHECK(map.nth(0) < map.nth(2));
        CHECK(map.nth(0) < map.nth(3));

        CHECK(map.nth(1) < map.nth(2));
        CHECK(map.nth(1) < map.nth(3));

        CHECK(map.nth(2) < map.nth(3));

        ///////////////////////////////////////////////////////////////////////

        CHECK(map.index_of(map.nth(0)) == 0);
        CHECK(map.index_of(map.nth(1)) == 1);
        CHECK(map.index_of(map.nth(2)) == 2);
        CHECK(map.index_of(map.nth(3)) == 3);
    }

    ///////////////////////////////////////////////////////////////////////////

    PRINT("Test static_capacity");
    {
        CHECK((sfl::static_flat_multimap<xint, xint, 100, std::less<xint>>::static_capacity == 100));
    }

    ///////////////////////////////////////////////////////////////////////////////

    PRINT("Test key_comp()");
    {
        {
            sfl::static_flat_multimap<xint, xint, 100, std::less<xint>> map;

            auto key_comp = map.key_comp();

            CHECK(key_comp(10, 10) == false);
            CHECK(key_comp(10, 20) == true);
            CHECK(key_comp(20, 10) == false);
            CHECK(key_comp(20, 20) == false);
        }

        {
            sfl::static_flat_multimap<xobj, xint, 100, xobj::less> map;

            auto key_comp = map.key_comp();

            CHECK(key_comp(xobj(10), 10) == false);
            CHECK(key_comp(xobj(10), 20) == true);
            CHECK(key_comp(xobj(20), 10) == false);
            CHECK(key_comp(xobj(20), 20) == false);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////

    PRINT("Test value_comp()");
    {
        {
            sfl::static_flat_multimap<xint, xint, 100, std::less<xint>> map;

            auto value_comp = map.value_comp();

            CHECK(value_comp({10, 1}, {10, 2}) == false);
            CHECK(value_comp({10, 1}, {20, 2}) == true);
            CHECK(value_comp({20, 1}, {10, 2}) == false);
            CHECK(value_comp({20, 1}, {20, 2}) == false);
        }

        {
            sfl::static_flat_multimap<xobj, xint, 100, xobj::less> map;

            auto value_comp = map.value_comp();

            CHECK(value_comp({xobj(10), 1}, {xobj(10), 2}) == false);
            CHECK(value_comp({xobj(10), 1}, {xobj(20), 2}) == true);
            CHECK(value_comp({xobj(20), 1}, {xobj(10), 2}) == false);
            CHECK(value_comp({xobj(20), 1}, {xobj(20), 2}) == false);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////

    PRINT("Test lower_bound, upper_bound, equal_range, find, count, contains");
    {
        // xint, xint
        {
            sfl::static_flat_multimap<xint, xint, 100, std::less<xint>> map;

            using value_type = std::pair<xint, xint>;

            map.insert_exactly_at(map.end(), value_type(20, 1));
            map.insert_exactly_at(map.end(), value_type(40, 1));
            map.insert_exactly_at(map.end(), value_type(60, 1));

            CHECK(map.size() == 3);
            CHECK(map.nth(0)->first == 20); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 40); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 60); CHECK(map.nth(2)->second == 1);

            ///////////////////////////////////////////////////////////////////////

            CHECK(map.lower_bound(10) == map.nth(0));
            CHECK(map.lower_bound(20) == map.nth(0));
            CHECK(map.lower_bound(30) == map.nth(1));
            CHECK(map.lower_bound(40) == map.nth(1));
            CHECK(map.lower_bound(50) == map.nth(2));
            CHECK(map.lower_bound(60) == map.nth(2));
            CHECK(map.lower_bound(70) == map.nth(3));

            ///////////////////////////////////////////////////////////////////////

            CHECK(map.upper_bound(10) == map.nth(0));
            CHECK(map.upper_bound(20) == map.nth(1));
            CHECK(map.upper_bound(30) == map.nth(1));
            CHECK(map.upper_bound(40) == map.nth(2));
            CHECK(map.upper_bound(50) == map.nth(2));
            CHECK(map.upper_bound(60) == map.nth(3));
            CHECK(map.upper_bound(70) == map.nth(3));

            ///////////////////////////////////////////////////////////////////////

            CHECK(map.equal_range(10) == std::make_pair(map.nth(0), map.nth(0)));
            CHECK(map.equal_range(20) == std::make_pair(map.nth(0), map.nth(1)));
            CHECK(map.equal_range(30) == std::make_pair(map.nth(1), map.nth(1)));
            CHECK(map.equal_range(40) == std::make_pair(map.nth(1), map.nth(2)));
            CHECK(map.equal_range(50) == std::make_pair(map.nth(2), map.nth(2)));
            CHECK(map.equal_range(60) == std::make_pair(map.nth(2), map.nth(3)));
            CHECK(map.equal_range(70) == std::make_pair(map.nth(3), map.nth(3)));

            ///////////////////////////////////////////////////////////////////////

            CHECK(map.find(10) == map.end());
            CHECK(map.find(20) == map.nth(0));
            CHECK(map.find(30) == map.end());
            CHECK(map.find(40) == map.nth(1));
            CHECK(map.find(50) == map.end());
            CHECK(map.find(60) == map.nth(2));
            CHECK(map.find(70) == map.end());

            ///////////////////////////////////////////////////////////////////////

            CHECK(map.count(10) == 0);
            CHECK(map.count(20) == 1);
            CHECK(map.count(30) == 0);
            CHECK(map.count(40) == 1);
            CHECK(map.count(50) == 0);
            CHECK(map.count(60) == 1);
            CHECK(map.count(70) == 0);

            ///////////////////////////////////////////////////////////////////////

            CHECK(map.contains(10) == false);
            CHECK(map.contains(20) == true);
            CHECK(map.contains(30) == false);
            CHECK(map.contains(40) == true);
            CHECK(map.contains(50) == false);
            CHECK(map.contains(60) == true);
            CHECK(map.contains(70) == false);
        }

        // xobj, xint
        {
            sfl::static_flat_multimap<xobj, xint, 100, xobj::less> map;

            using value_type = std::pair<xobj, xint>;

            map.insert_exactly_at(map.end(), value_type(std::piecewise_construct, std::forward_as_tuple(20), std::forward_as_tuple(1)));
            map.insert_exactly_at(map.end(), value_type(std::piecewise_construct, std::forward_as_tuple(40), std::forward_as_tuple(1)));
            map.insert_exactly_at(map.end(), value_type(std::piecewise_construct, std::forward_as_tuple(60), std::forward_as_tuple(1)));

            CHECK(map.size() == 3);
            CHECK(map.nth(0)->first.value() == 20); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first.value() == 40); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first.value() == 60); CHECK(map.nth(2)->second == 1);

            ///////////////////////////////////////////////////////////////////////

            CHECK(map.lower_bound(10) == map.nth(0));
            CHECK(map.lower_bound(20) == map.nth(0));
            CHECK(map.lower_bound(30) == map.nth(1));
            CHECK(map.lower_bound(40) == map.nth(1));
            CHECK(map.lower_bound(50) == map.nth(2));
            CHECK(map.lower_bound(60) == map.nth(2));
            CHECK(map.lower_bound(70) == map.nth(3));

            ///////////////////////////////////////////////////////////////////////

            CHECK(map.upper_bound(10) == map.nth(0));
            CHECK(map.upper_bound(20) == map.nth(1));
            CHECK(map.upper_bound(30) == map.nth(1));
            CHECK(map.upper_bound(40) == map.nth(2));
            CHECK(map.upper_bound(50) == map.nth(2));
            CHECK(map.upper_bound(60) == map.nth(3));
            CHECK(map.upper_bound(70) == map.nth(3));

            ///////////////////////////////////////////////////////////////////////

            CHECK(map.equal_range(10) == std::make_pair(map.nth(0), map.nth(0)));
            CHECK(map.equal_range(20) == std::make_pair(map.nth(0), map.nth(1)));
            CHECK(map.equal_range(30) == std::make_pair(map.nth(1), map.nth(1)));
            CHECK(map.equal_range(40) == std::make_pair(map.nth(1), map.nth(2)));
            CHECK(map.equal_range(50) == std::make_pair(map.nth(2), map.nth(2)));
            CHECK(map.equal_range(60) == std::make_pair(map.nth(2), map.nth(3)));
            CHECK(map.equal_range(70) == std::make_pair(map.nth(3), map.nth(3)));

            ///////////////////////////////////////////////////////////////////////

            CHECK(map.find(10) == map.end());
            CHECK(map.find(20) == map.nth(0));
            CHECK(map.find(30) == map.end());
            CHECK(map.find(40) == map.nth(1));
            CHECK(map.find(50) == map.end());
            CHECK(map.find(60) == map.nth(2));
            CHECK(map.find(70) == map.end());

            ///////////////////////////////////////////////////////////////////////

            CHECK(map.count(10) == 0);
            CHECK(map.count(20) == 1);
            CHECK(map.count(30) == 0);
            CHECK(map.count(40) == 1);
            CHECK(map.count(50) == 0);
            CHECK(map.count(60) == 1);
            CHECK(map.count(70) == 0);

            ///////////////////////////////////////////////////////////////////////

            CHECK(map.contains(10) == false);
            CHECK(map.contains(20) == true);
            CHECK(map.contains(30) == false);
            CHECK(map.contains(40) == true);
            CHECK(map.contains(50) == false);
            CHECK(map.contains(60) == true);
            CHECK(map.contains(70) == false);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////

    PRINT("Test clear()");
    {
        sfl::static_flat_multimap<xint, xint, 100, std::less<xint>> map;

        using value_type = std::pair<xint, xint>;

        CHECK(map.size() == 0);

        map.insert_exactly_at(map.end(), value_type(10, 1));
        map.insert_exactly_at(map.end(), value_type(20, 1));
        map.insert_exactly_at(map.end(), value_type(30, 1));

        CHECK(map.size() == 3);
        CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
        CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
        CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);

        map.clear();

        CHECK(map.size() == 0);

        map.insert_exactly_at(map.end(), value_type(40, 2));
        map.insert_exactly_at(map.end(), value_type(50, 2));
        map.insert_exactly_at(map.end(), value_type(60, 2));

        CHECK(map.size() == 3);
        CHECK(map.nth(0)->first == 40); CHECK(map.nth(0)->second == 2);
        CHECK(map.nth(1)->first == 50); CHECK(map.nth(1)->second == 2);
        CHECK(map.nth(2)->first == 60); CHECK(map.nth(2)->second == 2);

        map.clear();

        CHECK(map.size() == 0);
    }

    PRINT("Test emplace(Args&&...)");
    {
        sfl::static_flat_multimap<xint, xint, 100, std::less<xint>> map;

        {
            CHECK(map.emplace(20, 1) == map.nth(0));
            CHECK(map.emplace(40, 1) == map.nth(1));
            CHECK(map.emplace(60, 1) == map.nth(2));

            CHECK(map.emplace(10, 1) == map.nth(0));
            CHECK(map.emplace(30, 1) == map.nth(2));
            CHECK(map.emplace(50, 1) == map.nth(4));

            CHECK(map.size() == 6);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
            CHECK(map.nth(3)->first == 40); CHECK(map.nth(3)->second == 1);
            CHECK(map.nth(4)->first == 50); CHECK(map.nth(4)->second == 1);
            CHECK(map.nth(5)->first == 60); CHECK(map.nth(5)->second == 1);
        }

        {
            CHECK(map.emplace(20, 2) == map.nth(1));
            CHECK(map.emplace(40, 2) == map.nth(4));
            CHECK(map.emplace(60, 2) == map.nth(7));

            CHECK(map.emplace(10, 2) == map.nth(0));
            CHECK(map.emplace(30, 2) == map.nth(4));
            CHECK(map.emplace(50, 2) == map.nth(8));

            CHECK(map.size() == 12);
            CHECK(map.nth( 0)->first == 10); CHECK(map.nth( 0)->second == 2);
            CHECK(map.nth( 1)->first == 10); CHECK(map.nth( 1)->second == 1);
            CHECK(map.nth( 2)->first == 20); CHECK(map.nth( 2)->second == 2);
            CHECK(map.nth( 3)->first == 20); CHECK(map.nth( 3)->second == 1);
            CHECK(map.nth( 4)->first == 30); CHECK(map.nth( 4)->second == 2);
            CHECK(map.nth( 5)->first == 30); CHECK(map.nth( 5)->second == 1);
            CHECK(map.nth( 6)->first == 40); CHECK(map.nth( 6)->second == 2);
            CHECK(map.nth( 7)->first == 40); CHECK(map.nth( 7)->second == 1);
            CHECK(map.nth( 8)->first == 50); CHECK(map.nth( 8)->second == 2);
            CHECK(map.nth( 9)->first == 50); CHECK(map.nth( 9)->second == 1);
            CHECK(map.nth(10)->first == 60); CHECK(map.nth(10)->second == 2);
            CHECK(map.nth(11)->first == 60); CHECK(map.nth(11)->second == 1);
        }
    }

    PRINT("Test emplace_hint(const_iterator, Args&&...)");
    {
        sfl::static_flat_multimap<xint, xint, 100, std::less<xint>> map;

        {
            CHECK(map.emplace_hint(map.begin(), 20, 1) == map.nth(0));
            CHECK(map.emplace_hint(map.begin(), 40, 1) == map.nth(1));
            CHECK(map.emplace_hint(map.begin(), 60, 1) == map.nth(2));

            CHECK(map.emplace_hint(map.begin(), 10, 1) == map.nth(0));
            CHECK(map.emplace_hint(map.begin(), 30, 1) == map.nth(2));
            CHECK(map.emplace_hint(map.begin(), 50, 1) == map.nth(4));

            CHECK(map.size() == 6);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
            CHECK(map.nth(3)->first == 40); CHECK(map.nth(3)->second == 1);
            CHECK(map.nth(4)->first == 50); CHECK(map.nth(4)->second == 1);
            CHECK(map.nth(5)->first == 60); CHECK(map.nth(5)->second == 1);
        }

        {
            CHECK(map.emplace_hint(map.begin(), 20, 2) == map.nth(1));
            CHECK(map.emplace_hint(map.begin(), 40, 2) == map.nth(4));
            CHECK(map.emplace_hint(map.begin(), 60, 2) == map.nth(7));

            CHECK(map.emplace_hint(map.begin(), 10, 2) == map.nth(0));
            CHECK(map.emplace_hint(map.begin(), 30, 2) == map.nth(4));
            CHECK(map.emplace_hint(map.begin(), 50, 2) == map.nth(8));

            CHECK(map.size() == 12);
            CHECK(map.nth( 0)->first == 10); CHECK(map.nth( 0)->second == 2);
            CHECK(map.nth( 1)->first == 10); CHECK(map.nth( 1)->second == 1);
            CHECK(map.nth( 2)->first == 20); CHECK(map.nth( 2)->second == 2);
            CHECK(map.nth( 3)->first == 20); CHECK(map.nth( 3)->second == 1);
            CHECK(map.nth( 4)->first == 30); CHECK(map.nth( 4)->second == 2);
            CHECK(map.nth( 5)->first == 30); CHECK(map.nth( 5)->second == 1);
            CHECK(map.nth( 6)->first == 40); CHECK(map.nth( 6)->second == 2);
            CHECK(map.nth( 7)->first == 40); CHECK(map.nth( 7)->second == 1);
            CHECK(map.nth( 8)->first == 50); CHECK(map.nth( 8)->second == 2);
            CHECK(map.nth( 9)->first == 50); CHECK(map.nth( 9)->second == 1);
            CHECK(map.nth(10)->first == 60); CHECK(map.nth(10)->second == 2);
            CHECK(map.nth(11)->first == 60); CHECK(map.nth(11)->second == 1);
        }
    }

    PRINT("Test insert(const value_type&)");
    {
        sfl::static_flat_multimap<xint, xint, 100, std::less<xint>> map;

        using value_type = std::pair<xint, xint>;

        {
            value_type value_20_1(20, 1);
            value_type value_40_1(40, 1);
            value_type value_60_1(60, 1);

            value_type value_10_1(10, 1);
            value_type value_30_1(30, 1);
            value_type value_50_1(50, 1);

            CHECK(map.insert(value_20_1) == map.nth(0));
            CHECK(map.insert(value_40_1) == map.nth(1));
            CHECK(map.insert(value_60_1) == map.nth(2));

            CHECK(map.insert(value_10_1) == map.nth(0));
            CHECK(map.insert(value_30_1) == map.nth(2));
            CHECK(map.insert(value_50_1) == map.nth(4));

            CHECK(map.size() == 6);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
            CHECK(map.nth(3)->first == 40); CHECK(map.nth(3)->second == 1);
            CHECK(map.nth(4)->first == 50); CHECK(map.nth(4)->second == 1);
            CHECK(map.nth(5)->first == 60); CHECK(map.nth(5)->second == 1);

            CHECK(value_20_1.first == 20); CHECK(value_20_1.second == 1);
            CHECK(value_40_1.first == 40); CHECK(value_40_1.second == 1);
            CHECK(value_60_1.first == 60); CHECK(value_60_1.second == 1);

            CHECK(value_10_1.first == 10); CHECK(value_10_1.second == 1);
            CHECK(value_30_1.first == 30); CHECK(value_30_1.second == 1);
            CHECK(value_50_1.first == 50); CHECK(value_50_1.second == 1);
        }

        {
            value_type value_20_2(20, 2);
            value_type value_40_2(40, 2);
            value_type value_60_2(60, 2);

            value_type value_10_2(10, 2);
            value_type value_30_2(30, 2);
            value_type value_50_2(50, 2);

            CHECK(map.insert(value_20_2) == map.nth(1));
            CHECK(map.insert(value_40_2) == map.nth(4));
            CHECK(map.insert(value_60_2) == map.nth(7));

            CHECK(map.insert(value_10_2) == map.nth(0));
            CHECK(map.insert(value_30_2) == map.nth(4));
            CHECK(map.insert(value_50_2) == map.nth(8));

            CHECK(map.size() == 12);
            CHECK(map.nth( 0)->first == 10); CHECK(map.nth( 0)->second == 2);
            CHECK(map.nth( 1)->first == 10); CHECK(map.nth( 1)->second == 1);
            CHECK(map.nth( 2)->first == 20); CHECK(map.nth( 2)->second == 2);
            CHECK(map.nth( 3)->first == 20); CHECK(map.nth( 3)->second == 1);
            CHECK(map.nth( 4)->first == 30); CHECK(map.nth( 4)->second == 2);
            CHECK(map.nth( 5)->first == 30); CHECK(map.nth( 5)->second == 1);
            CHECK(map.nth( 6)->first == 40); CHECK(map.nth( 6)->second == 2);
            CHECK(map.nth( 7)->first == 40); CHECK(map.nth( 7)->second == 1);
            CHECK(map.nth( 8)->first == 50); CHECK(map.nth( 8)->second == 2);
            CHECK(map.nth( 9)->first == 50); CHECK(map.nth( 9)->second == 1);
            CHECK(map.nth(10)->first == 60); CHECK(map.nth(10)->second == 2);
            CHECK(map.nth(11)->first == 60); CHECK(map.nth(11)->second == 1);

            CHECK(value_20_2.first == 20); CHECK(value_20_2.second == 2);
            CHECK(value_40_2.first == 40); CHECK(value_40_2.second == 2);
            CHECK(value_60_2.first == 60); CHECK(value_60_2.second == 2);

            CHECK(value_10_2.first == 10); CHECK(value_10_2.second == 2);
            CHECK(value_30_2.first == 30); CHECK(value_30_2.second == 2);
            CHECK(value_50_2.first == 50); CHECK(value_50_2.second == 2);
        }
    }

    PRINT("Test insert(value_type&&)");
    {
        sfl::static_flat_multimap<xint, xint, 100, std::less<xint>> map;

        using value_type = std::pair<xint, xint>;

        {
            value_type value_20_1(20, 1);
            value_type value_40_1(40, 1);
            value_type value_60_1(60, 1);

            value_type value_10_1(10, 1);
            value_type value_30_1(30, 1);
            value_type value_50_1(50, 1);

            CHECK(map.insert(std::move(value_20_1)) == map.nth(0));
            CHECK(map.insert(std::move(value_40_1)) == map.nth(1));
            CHECK(map.insert(std::move(value_60_1)) == map.nth(2));

            CHECK(map.insert(std::move(value_10_1)) == map.nth(0));
            CHECK(map.insert(std::move(value_30_1)) == map.nth(2));
            CHECK(map.insert(std::move(value_50_1)) == map.nth(4));

            CHECK(map.size() == 6);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
            CHECK(map.nth(3)->first == 40); CHECK(map.nth(3)->second == 1);
            CHECK(map.nth(4)->first == 50); CHECK(map.nth(4)->second == 1);
            CHECK(map.nth(5)->first == 60); CHECK(map.nth(5)->second == 1);

            CHECK(value_20_1.first == -20); CHECK(value_20_1.second == -1);
            CHECK(value_40_1.first == -40); CHECK(value_40_1.second == -1);
            CHECK(value_60_1.first == -60); CHECK(value_60_1.second == -1);

            CHECK(value_10_1.first == -10); CHECK(value_10_1.second == -1);
            CHECK(value_30_1.first == -30); CHECK(value_30_1.second == -1);
            CHECK(value_50_1.first == -50); CHECK(value_50_1.second == -1);
        }

        {
            value_type value_20_2(20, 2);
            value_type value_40_2(40, 2);
            value_type value_60_2(60, 2);

            value_type value_10_2(10, 2);
            value_type value_30_2(30, 2);
            value_type value_50_2(50, 2);

            CHECK(map.insert(std::move(value_20_2)) == map.nth(1));
            CHECK(map.insert(std::move(value_40_2)) == map.nth(4));
            CHECK(map.insert(std::move(value_60_2)) == map.nth(7));

            CHECK(map.insert(std::move(value_10_2)) == map.nth(0));
            CHECK(map.insert(std::move(value_30_2)) == map.nth(4));
            CHECK(map.insert(std::move(value_50_2)) == map.nth(8));

            CHECK(map.size() == 12);
            CHECK(map.nth( 0)->first == 10); CHECK(map.nth( 0)->second == 2);
            CHECK(map.nth( 1)->first == 10); CHECK(map.nth( 1)->second == 1);
            CHECK(map.nth( 2)->first == 20); CHECK(map.nth( 2)->second == 2);
            CHECK(map.nth( 3)->first == 20); CHECK(map.nth( 3)->second == 1);
            CHECK(map.nth( 4)->first == 30); CHECK(map.nth( 4)->second == 2);
            CHECK(map.nth( 5)->first == 30); CHECK(map.nth( 5)->second == 1);
            CHECK(map.nth( 6)->first == 40); CHECK(map.nth( 6)->second == 2);
            CHECK(map.nth( 7)->first == 40); CHECK(map.nth( 7)->second == 1);
            CHECK(map.nth( 8)->first == 50); CHECK(map.nth( 8)->second == 2);
            CHECK(map.nth( 9)->first == 50); CHECK(map.nth( 9)->second == 1);
            CHECK(map.nth(10)->first == 60); CHECK(map.nth(10)->second == 2);
            CHECK(map.nth(11)->first == 60); CHECK(map.nth(11)->second == 1);

            CHECK(value_20_2.first == -20); CHECK(value_20_2.second == -2);
            CHECK(value_40_2.first == -40); CHECK(value_40_2.second == -2);
            CHECK(value_60_2.first == -60); CHECK(value_60_2.second == -2);

            CHECK(value_10_2.first == -10); CHECK(value_10_2.second == -2);
            CHECK(value_30_2.first == -30); CHECK(value_30_2.second == -2);
            CHECK(value_50_2.first == -50); CHECK(value_50_2.second == -2);
        }
    }

    PRINT("Test insert(P&&)");
    {
        sfl::static_flat_multimap<xint, xint, 100, std::less<xint>> map;

        using value_type = std::pair<long, long>;

        {
            value_type value_20_1(20, 1);
            value_type value_40_1(40, 1);
            value_type value_60_1(60, 1);

            value_type value_10_1(10, 1);
            value_type value_30_1(30, 1);
            value_type value_50_1(50, 1);

            CHECK(map.insert(value_20_1) == map.nth(0));
            CHECK(map.insert(value_40_1) == map.nth(1));
            CHECK(map.insert(value_60_1) == map.nth(2));

            CHECK(map.insert(value_10_1) == map.nth(0));
            CHECK(map.insert(value_30_1) == map.nth(2));
            CHECK(map.insert(value_50_1) == map.nth(4));

            CHECK(map.size() == 6);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
            CHECK(map.nth(3)->first == 40); CHECK(map.nth(3)->second == 1);
            CHECK(map.nth(4)->first == 50); CHECK(map.nth(4)->second == 1);
            CHECK(map.nth(5)->first == 60); CHECK(map.nth(5)->second == 1);

            CHECK(value_20_1.first == 20); CHECK(value_20_1.second == 1);
            CHECK(value_40_1.first == 40); CHECK(value_40_1.second == 1);
            CHECK(value_60_1.first == 60); CHECK(value_60_1.second == 1);

            CHECK(value_10_1.first == 10); CHECK(value_10_1.second == 1);
            CHECK(value_30_1.first == 30); CHECK(value_30_1.second == 1);
            CHECK(value_50_1.first == 50); CHECK(value_50_1.second == 1);
        }

        {
            value_type value_20_2(20, 2);
            value_type value_40_2(40, 2);
            value_type value_60_2(60, 2);

            value_type value_10_2(10, 2);
            value_type value_30_2(30, 2);
            value_type value_50_2(50, 2);

            CHECK(map.insert(value_20_2) == map.nth(1));
            CHECK(map.insert(value_40_2) == map.nth(4));
            CHECK(map.insert(value_60_2) == map.nth(7));

            CHECK(map.insert(value_10_2) == map.nth(0));
            CHECK(map.insert(value_30_2) == map.nth(4));
            CHECK(map.insert(value_50_2) == map.nth(8));

            CHECK(map.size() == 12);
            CHECK(map.nth( 0)->first == 10); CHECK(map.nth( 0)->second == 2);
            CHECK(map.nth( 1)->first == 10); CHECK(map.nth( 1)->second == 1);
            CHECK(map.nth( 2)->first == 20); CHECK(map.nth( 2)->second == 2);
            CHECK(map.nth( 3)->first == 20); CHECK(map.nth( 3)->second == 1);
            CHECK(map.nth( 4)->first == 30); CHECK(map.nth( 4)->second == 2);
            CHECK(map.nth( 5)->first == 30); CHECK(map.nth( 5)->second == 1);
            CHECK(map.nth( 6)->first == 40); CHECK(map.nth( 6)->second == 2);
            CHECK(map.nth( 7)->first == 40); CHECK(map.nth( 7)->second == 1);
            CHECK(map.nth( 8)->first == 50); CHECK(map.nth( 8)->second == 2);
            CHECK(map.nth( 9)->first == 50); CHECK(map.nth( 9)->second == 1);
            CHECK(map.nth(10)->first == 60); CHECK(map.nth(10)->second == 2);
            CHECK(map.nth(11)->first == 60); CHECK(map.nth(11)->second == 1);

            CHECK(value_20_2.first == 20); CHECK(value_20_2.second == 2);
            CHECK(value_40_2.first == 40); CHECK(value_40_2.second == 2);
            CHECK(value_60_2.first == 60); CHECK(value_60_2.second == 2);

            CHECK(value_10_2.first == 10); CHECK(value_10_2.second == 2);
            CHECK(value_30_2.first == 30); CHECK(value_30_2.second == 2);
            CHECK(value_50_2.first == 50); CHECK(value_50_2.second == 2);
        }
    }

    PRINT("Test insert(const_iterator, const value_type&)");
    {
        sfl::static_flat_multimap<xint, xint, 100, std::less<xint>> map;

        using value_type = std::pair<xint, xint>;

        {
            value_type value_20_1(20, 1);
            value_type value_40_1(40, 1);
            value_type value_60_1(60, 1);

            value_type value_10_1(10, 1);
            value_type value_30_1(30, 1);
            value_type value_50_1(50, 1);

            CHECK(map.insert(map.begin(), value_20_1) == map.nth(0));
            CHECK(map.insert(map.begin(), value_40_1) == map.nth(1));
            CHECK(map.insert(map.begin(), value_60_1) == map.nth(2));

            CHECK(map.insert(map.begin(), value_10_1) == map.nth(0));
            CHECK(map.insert(map.begin(), value_30_1) == map.nth(2));
            CHECK(map.insert(map.begin(), value_50_1) == map.nth(4));

            CHECK(map.size() == 6);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
            CHECK(map.nth(3)->first == 40); CHECK(map.nth(3)->second == 1);
            CHECK(map.nth(4)->first == 50); CHECK(map.nth(4)->second == 1);
            CHECK(map.nth(5)->first == 60); CHECK(map.nth(5)->second == 1);

            CHECK(value_20_1.first == 20); CHECK(value_20_1.second == 1);
            CHECK(value_40_1.first == 40); CHECK(value_40_1.second == 1);
            CHECK(value_60_1.first == 60); CHECK(value_60_1.second == 1);

            CHECK(value_10_1.first == 10); CHECK(value_10_1.second == 1);
            CHECK(value_30_1.first == 30); CHECK(value_30_1.second == 1);
            CHECK(value_50_1.first == 50); CHECK(value_50_1.second == 1);
        }

        {
            value_type value_20_2(20, 2);
            value_type value_40_2(40, 2);
            value_type value_60_2(60, 2);

            value_type value_10_2(10, 2);
            value_type value_30_2(30, 2);
            value_type value_50_2(50, 2);

            CHECK(map.insert(map.begin(), value_20_2) == map.nth(1));
            CHECK(map.insert(map.begin(), value_40_2) == map.nth(4));
            CHECK(map.insert(map.begin(), value_60_2) == map.nth(7));

            CHECK(map.insert(map.begin(), value_10_2) == map.nth(0));
            CHECK(map.insert(map.begin(), value_30_2) == map.nth(4));
            CHECK(map.insert(map.begin(), value_50_2) == map.nth(8));

            CHECK(map.size() == 12);
            CHECK(map.nth( 0)->first == 10); CHECK(map.nth( 0)->second == 2);
            CHECK(map.nth( 1)->first == 10); CHECK(map.nth( 1)->second == 1);
            CHECK(map.nth( 2)->first == 20); CHECK(map.nth( 2)->second == 2);
            CHECK(map.nth( 3)->first == 20); CHECK(map.nth( 3)->second == 1);
            CHECK(map.nth( 4)->first == 30); CHECK(map.nth( 4)->second == 2);
            CHECK(map.nth( 5)->first == 30); CHECK(map.nth( 5)->second == 1);
            CHECK(map.nth( 6)->first == 40); CHECK(map.nth( 6)->second == 2);
            CHECK(map.nth( 7)->first == 40); CHECK(map.nth( 7)->second == 1);
            CHECK(map.nth( 8)->first == 50); CHECK(map.nth( 8)->second == 2);
            CHECK(map.nth( 9)->first == 50); CHECK(map.nth( 9)->second == 1);
            CHECK(map.nth(10)->first == 60); CHECK(map.nth(10)->second == 2);
            CHECK(map.nth(11)->first == 60); CHECK(map.nth(11)->second == 1);

            CHECK(value_20_2.first == 20); CHECK(value_20_2.second == 2);
            CHECK(value_40_2.first == 40); CHECK(value_40_2.second == 2);
            CHECK(value_60_2.first == 60); CHECK(value_60_2.second == 2);

            CHECK(value_10_2.first == 10); CHECK(value_10_2.second == 2);
            CHECK(value_30_2.first == 30); CHECK(value_30_2.second == 2);
            CHECK(value_50_2.first == 50); CHECK(value_50_2.second == 2);
        }
    }

    PRINT("Test insert(const_iterator, value_type&&)");
    {
        sfl::static_flat_multimap<xint, xint, 100, std::less<xint>> map;

        using value_type = std::pair<xint, xint>;

        {
            value_type value_20_1(20, 1);
            value_type value_40_1(40, 1);
            value_type value_60_1(60, 1);

            value_type value_10_1(10, 1);
            value_type value_30_1(30, 1);
            value_type value_50_1(50, 1);

            CHECK(map.insert(map.begin(), std::move(value_20_1)) == map.nth(0));
            CHECK(map.insert(map.begin(), std::move(value_40_1)) == map.nth(1));
            CHECK(map.insert(map.begin(), std::move(value_60_1)) == map.nth(2));

            CHECK(map.insert(map.begin(), std::move(value_10_1)) == map.nth(0));
            CHECK(map.insert(map.begin(), std::move(value_30_1)) == map.nth(2));
            CHECK(map.insert(map.begin(), std::move(value_50_1)) == map.nth(4));

            CHECK(map.size() == 6);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
            CHECK(map.nth(3)->first == 40); CHECK(map.nth(3)->second == 1);
            CHECK(map.nth(4)->first == 50); CHECK(map.nth(4)->second == 1);
            CHECK(map.nth(5)->first == 60); CHECK(map.nth(5)->second == 1);

            CHECK(value_20_1.first == -20); CHECK(value_20_1.second == -1);
            CHECK(value_40_1.first == -40); CHECK(value_40_1.second == -1);
            CHECK(value_60_1.first == -60); CHECK(value_60_1.second == -1);

            CHECK(value_10_1.first == -10); CHECK(value_10_1.second == -1);
            CHECK(value_30_1.first == -30); CHECK(value_30_1.second == -1);
            CHECK(value_50_1.first == -50); CHECK(value_50_1.second == -1);
        }

        {
            value_type value_20_2(20, 2);
            value_type value_40_2(40, 2);
            value_type value_60_2(60, 2);

            value_type value_10_2(10, 2);
            value_type value_30_2(30, 2);
            value_type value_50_2(50, 2);

            CHECK(map.insert(map.begin(), std::move(value_20_2)) == map.nth(1));
            CHECK(map.insert(map.begin(), std::move(value_40_2)) == map.nth(4));
            CHECK(map.insert(map.begin(), std::move(value_60_2)) == map.nth(7));

            CHECK(map.insert(map.begin(), std::move(value_10_2)) == map.nth(0));
            CHECK(map.insert(map.begin(), std::move(value_30_2)) == map.nth(4));
            CHECK(map.insert(map.begin(), std::move(value_50_2)) == map.nth(8));

            CHECK(map.size() == 12);
            CHECK(map.nth( 0)->first == 10); CHECK(map.nth( 0)->second == 2);
            CHECK(map.nth( 1)->first == 10); CHECK(map.nth( 1)->second == 1);
            CHECK(map.nth( 2)->first == 20); CHECK(map.nth( 2)->second == 2);
            CHECK(map.nth( 3)->first == 20); CHECK(map.nth( 3)->second == 1);
            CHECK(map.nth( 4)->first == 30); CHECK(map.nth( 4)->second == 2);
            CHECK(map.nth( 5)->first == 30); CHECK(map.nth( 5)->second == 1);
            CHECK(map.nth( 6)->first == 40); CHECK(map.nth( 6)->second == 2);
            CHECK(map.nth( 7)->first == 40); CHECK(map.nth( 7)->second == 1);
            CHECK(map.nth( 8)->first == 50); CHECK(map.nth( 8)->second == 2);
            CHECK(map.nth( 9)->first == 50); CHECK(map.nth( 9)->second == 1);
            CHECK(map.nth(10)->first == 60); CHECK(map.nth(10)->second == 2);
            CHECK(map.nth(11)->first == 60); CHECK(map.nth(11)->second == 1);

            CHECK(value_20_2.first == -20); CHECK(value_20_2.second == -2);
            CHECK(value_40_2.first == -40); CHECK(value_40_2.second == -2);
            CHECK(value_60_2.first == -60); CHECK(value_60_2.second == -2);

            CHECK(value_10_2.first == -10); CHECK(value_10_2.second == -2);
            CHECK(value_30_2.first == -30); CHECK(value_30_2.second == -2);
            CHECK(value_50_2.first == -50); CHECK(value_50_2.second == -2);
        }
    }

    PRINT("Test insert(const_iterator, P&&)");
    {
        sfl::static_flat_multimap<xint, xint, 100, std::less<xint>> map;

        using value_type = std::pair<long, long>;

        {
            value_type value_20_1(20, 1);
            value_type value_40_1(40, 1);
            value_type value_60_1(60, 1);

            value_type value_10_1(10, 1);
            value_type value_30_1(30, 1);
            value_type value_50_1(50, 1);

            CHECK(map.insert(map.begin(), value_20_1) == map.nth(0));
            CHECK(map.insert(map.begin(), value_40_1) == map.nth(1));
            CHECK(map.insert(map.begin(), value_60_1) == map.nth(2));

            CHECK(map.insert(map.begin(), value_10_1) == map.nth(0));
            CHECK(map.insert(map.begin(), value_30_1) == map.nth(2));
            CHECK(map.insert(map.begin(), value_50_1) == map.nth(4));

            CHECK(map.size() == 6);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
            CHECK(map.nth(3)->first == 40); CHECK(map.nth(3)->second == 1);
            CHECK(map.nth(4)->first == 50); CHECK(map.nth(4)->second == 1);
            CHECK(map.nth(5)->first == 60); CHECK(map.nth(5)->second == 1);

            CHECK(value_20_1.first == 20); CHECK(value_20_1.second == 1);
            CHECK(value_40_1.first == 40); CHECK(value_40_1.second == 1);
            CHECK(value_60_1.first == 60); CHECK(value_60_1.second == 1);

            CHECK(value_10_1.first == 10); CHECK(value_10_1.second == 1);
            CHECK(value_30_1.first == 30); CHECK(value_30_1.second == 1);
            CHECK(value_50_1.first == 50); CHECK(value_50_1.second == 1);
        }

        {
            value_type value_20_2(20, 2);
            value_type value_40_2(40, 2);
            value_type value_60_2(60, 2);

            value_type value_10_2(10, 2);
            value_type value_30_2(30, 2);
            value_type value_50_2(50, 2);

            CHECK(map.insert(map.begin(), value_20_2) == map.nth(1));
            CHECK(map.insert(map.begin(), value_40_2) == map.nth(4));
            CHECK(map.insert(map.begin(), value_60_2) == map.nth(7));

            CHECK(map.insert(map.begin(), value_10_2) == map.nth(0));
            CHECK(map.insert(map.begin(), value_30_2) == map.nth(4));
            CHECK(map.insert(map.begin(), value_50_2) == map.nth(8));

            CHECK(map.size() == 12);
            CHECK(map.nth( 0)->first == 10); CHECK(map.nth( 0)->second == 2);
            CHECK(map.nth( 1)->first == 10); CHECK(map.nth( 1)->second == 1);
            CHECK(map.nth( 2)->first == 20); CHECK(map.nth( 2)->second == 2);
            CHECK(map.nth( 3)->first == 20); CHECK(map.nth( 3)->second == 1);
            CHECK(map.nth( 4)->first == 30); CHECK(map.nth( 4)->second == 2);
            CHECK(map.nth( 5)->first == 30); CHECK(map.nth( 5)->second == 1);
            CHECK(map.nth( 6)->first == 40); CHECK(map.nth( 6)->second == 2);
            CHECK(map.nth( 7)->first == 40); CHECK(map.nth( 7)->second == 1);
            CHECK(map.nth( 8)->first == 50); CHECK(map.nth( 8)->second == 2);
            CHECK(map.nth( 9)->first == 50); CHECK(map.nth( 9)->second == 1);
            CHECK(map.nth(10)->first == 60); CHECK(map.nth(10)->second == 2);
            CHECK(map.nth(11)->first == 60); CHECK(map.nth(11)->second == 1);

            CHECK(value_20_2.first == 20); CHECK(value_20_2.second == 2);
            CHECK(value_40_2.first == 40); CHECK(value_40_2.second == 2);
            CHECK(value_60_2.first == 60); CHECK(value_60_2.second == 2);

            CHECK(value_10_2.first == 10); CHECK(value_10_2.second == 2);
            CHECK(value_30_2.first == 30); CHECK(value_30_2.second == 2);
            CHECK(value_50_2.first == 50); CHECK(value_50_2.second == 2);
        }
    }

    PRINT("Test insert(InputIt, InputIt)");
    {
        sfl::static_flat_multimap<xint, xint, 100, std::less<xint>> map;

        {
            std::vector<std::pair<xint, xint>> data1
            (
                {
                    {20, 1},
                    {40, 1},
                    {60, 1}
                }
            );

            std::vector<std::pair<xint, xint>> data2
            (
                {
                    {10, 1},
                    {30, 1},
                    {50, 1}
                }
            );

            map.insert(data1.begin(), data1.end());
            map.insert(data2.begin(), data2.end());

            CHECK(map.size() == 6);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
            CHECK(map.nth(3)->first == 40); CHECK(map.nth(3)->second == 1);
            CHECK(map.nth(4)->first == 50); CHECK(map.nth(4)->second == 1);
            CHECK(map.nth(5)->first == 60); CHECK(map.nth(5)->second == 1);
        }

        {
            std::vector<std::pair<xint, xint>> data1
            (
                {
                    {20, 2},
                    {40, 2},
                    {60, 2}
                }
            );

            std::vector<std::pair<xint, xint>> data2
            (
                {
                    {10, 2},
                    {30, 2},
                    {50, 2}
                }
            );

            map.insert(data1.begin(), data1.end());
            map.insert(data2.begin(), data2.end());

            CHECK(map.size() == 12);
            CHECK(map.nth( 0)->first == 10); CHECK(map.nth( 0)->second == 2);
            CHECK(map.nth( 1)->first == 10); CHECK(map.nth( 1)->second == 1);
            CHECK(map.nth( 2)->first == 20); CHECK(map.nth( 2)->second == 2);
            CHECK(map.nth( 3)->first == 20); CHECK(map.nth( 3)->second == 1);
            CHECK(map.nth( 4)->first == 30); CHECK(map.nth( 4)->second == 2);
            CHECK(map.nth( 5)->first == 30); CHECK(map.nth( 5)->second == 1);
            CHECK(map.nth( 6)->first == 40); CHECK(map.nth( 6)->second == 2);
            CHECK(map.nth( 7)->first == 40); CHECK(map.nth( 7)->second == 1);
            CHECK(map.nth( 8)->first == 50); CHECK(map.nth( 8)->second == 2);
            CHECK(map.nth( 9)->first == 50); CHECK(map.nth( 9)->second == 1);
            CHECK(map.nth(10)->first == 60); CHECK(map.nth(10)->second == 2);
            CHECK(map.nth(11)->first == 60); CHECK(map.nth(11)->second == 1);
        }
    }

    PRINT("Test insert(std::initializer_list)");
    {
        sfl::static_flat_multimap<xint, xint, 100, std::less<xint>> map;

        {
            std::initializer_list<std::pair<xint, xint>> ilist1
            {
                {20, 1},
                {40, 1},
                {60, 1}
            };

            std::initializer_list<std::pair<xint, xint>> ilist2
            {
                {10, 1},
                {30, 1},
                {50, 1}
            };

            map.insert(ilist1);
            map.insert(ilist2);

            CHECK(map.size() == 6);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
            CHECK(map.nth(3)->first == 40); CHECK(map.nth(3)->second == 1);
            CHECK(map.nth(4)->first == 50); CHECK(map.nth(4)->second == 1);
            CHECK(map.nth(5)->first == 60); CHECK(map.nth(5)->second == 1);
        }

        {
            std::initializer_list<std::pair<xint, xint>> ilist1
            {
                {20, 2},
                {40, 2},
                {60, 2}
            };

            std::initializer_list<std::pair<xint, xint>> ilist2
            {
                {10, 2},
                {30, 2},
                {50, 2}
            };

            map.insert(ilist1);
            map.insert(ilist2);

            CHECK(map.size() == 12);
            CHECK(map.nth( 0)->first == 10); CHECK(map.nth( 0)->second == 2);
            CHECK(map.nth( 1)->first == 10); CHECK(map.nth( 1)->second == 1);
            CHECK(map.nth( 2)->first == 20); CHECK(map.nth( 2)->second == 2);
            CHECK(map.nth( 3)->first == 20); CHECK(map.nth( 3)->second == 1);
            CHECK(map.nth( 4)->first == 30); CHECK(map.nth( 4)->second == 2);
            CHECK(map.nth( 5)->first == 30); CHECK(map.nth( 5)->second == 1);
            CHECK(map.nth( 6)->first == 40); CHECK(map.nth( 6)->second == 2);
            CHECK(map.nth( 7)->first == 40); CHECK(map.nth( 7)->second == 1);
            CHECK(map.nth( 8)->first == 50); CHECK(map.nth( 8)->second == 2);
            CHECK(map.nth( 9)->first == 50); CHECK(map.nth( 9)->second == 1);
            CHECK(map.nth(10)->first == 60); CHECK(map.nth(10)->second == 2);
            CHECK(map.nth(11)->first == 60); CHECK(map.nth(11)->second == 1);
        }
    }

    PRINT("Test insert_range(Range&&");
    {
        // Input iterator (exactly)
        {
            std::istringstream iss("10 1 20 1 30 1 20 2 20 3");

            sfl::static_flat_multimap<xint, xint, 32, std::less<xint>> map;

            #if SFL_CPP_VERSION >= SFL_CPP_20
            map.insert_range(std::views::istream<std::pair<int, int>>(iss));
            #else
            map.insert_range(sfl::test::istream_view<std::pair<int, int>>(iss));
            #endif

            CHECK(map.size() == 5);
            CHECK(NTH(map, 0)->first == 10); CHECK(NTH(map, 0)->second == 1);
            CHECK(NTH(map, 1)->first == 20); CHECK(NTH(map, 1)->second == 3);
            CHECK(NTH(map, 2)->first == 20); CHECK(NTH(map, 2)->second == 2);
            CHECK(NTH(map, 3)->first == 20); CHECK(NTH(map, 3)->second == 1);
            CHECK(NTH(map, 4)->first == 30); CHECK(NTH(map, 4)->second == 1);
        }

        // Forward iterator
        {
            std::vector<std::pair<xint, xint>> data
            (
                {
                    {10, 1},
                    {20, 1},
                    {30, 1},
                    {20, 2},
                    {20, 3}
                }
            );

            sfl::static_flat_multimap<xint, xint, 32, std::less<xint>> map;

            #if SFL_CPP_VERSION >= SFL_CPP_20
            map.insert_range(std::views::all(data));
            #else
            map.insert_range(data);
            #endif

            CHECK(map.size() == 5);
            CHECK(NTH(map, 0)->first == 10); CHECK(NTH(map, 0)->second == 1);
            CHECK(NTH(map, 1)->first == 20); CHECK(NTH(map, 1)->second == 3);
            CHECK(NTH(map, 2)->first == 20); CHECK(NTH(map, 2)->second == 2);
            CHECK(NTH(map, 3)->first == 20); CHECK(NTH(map, 3)->second == 1);
            CHECK(NTH(map, 4)->first == 30); CHECK(NTH(map, 4)->second == 1);
        }
    }

    PRINT("Test erase(const_iterator)");
    {
        // Erase at the end
        {
            sfl::static_flat_multimap<xint, xint, 100, std::less<xint>> map;

            map.emplace(10, 1);
            map.emplace(20, 1);
            map.emplace(30, 1);
            map.emplace(40, 1);
            map.emplace(50, 1);

            CHECK(map.size() == 5);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
            CHECK(map.nth(3)->first == 40); CHECK(map.nth(3)->second == 1);
            CHECK(map.nth(4)->first == 50); CHECK(map.nth(4)->second == 1);

            ///////////////////////////////////////////////////////////////////////

            CHECK(map.erase(map.nth(4)) == map.nth(4));
            CHECK(map.size() == 4);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
            CHECK(map.nth(3)->first == 40); CHECK(map.nth(3)->second == 1);

            ///////////////////////////////////////////////////////////////////////

            CHECK(map.erase(map.nth(3)) == map.nth(3));
            CHECK(map.size() == 3);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);

            ///////////////////////////////////////////////////////////////////////

            CHECK(map.erase(map.nth(2)) == map.nth(2));
            CHECK(map.size() == 2);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);

            ///////////////////////////////////////////////////////////////////////

            CHECK(map.erase(map.nth(1)) == map.nth(1));
            CHECK(map.size() == 1);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);

            ///////////////////////////////////////////////////////////////////////

            CHECK(map.erase(map.nth(0)) == map.nth(0));
            CHECK(map.size() == 0);
        }

        // Erase at the begin
        {
            sfl::static_flat_multimap<xint, xint, 100, std::less<xint>> map;

            map.emplace(10, 1);
            map.emplace(20, 1);
            map.emplace(30, 1);
            map.emplace(40, 1);
            map.emplace(50, 1);

            CHECK(map.size() == 5);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
            CHECK(map.nth(3)->first == 40); CHECK(map.nth(3)->second == 1);
            CHECK(map.nth(4)->first == 50); CHECK(map.nth(4)->second == 1);

            ///////////////////////////////////////////////////////////////////////

            CHECK(map.erase(map.nth(0)) == map.nth(0));
            CHECK(map.size() == 4);
            CHECK(map.nth(0)->first == 20); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 30); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 40); CHECK(map.nth(2)->second == 1);
            CHECK(map.nth(3)->first == 50); CHECK(map.nth(3)->second == 1);

            ///////////////////////////////////////////////////////////////////////

            CHECK(map.erase(map.nth(0)) == map.nth(0));
            CHECK(map.size() == 3);
            CHECK(map.nth(0)->first == 30); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 40); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 50); CHECK(map.nth(2)->second == 1);

            ///////////////////////////////////////////////////////////////////////

            CHECK(map.erase(map.nth(0)) == map.nth(0));
            CHECK(map.size() == 2);
            CHECK(map.nth(0)->first == 40); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 50); CHECK(map.nth(1)->second == 1);

            ///////////////////////////////////////////////////////////////////////

            CHECK(map.erase(map.nth(0)) == map.nth(0));
            CHECK(map.size() == 1);
            CHECK(map.nth(0)->first == 50); CHECK(map.nth(0)->second == 1);

            ///////////////////////////////////////////////////////////////////////

            CHECK(map.erase(map.nth(0)) == map.nth(0));
            CHECK(map.size() == 0);
        }

        // Erase near the end
        {
            sfl::static_flat_multimap<xint, xint, 100, std::less<xint>> map;

            map.emplace(10, 1);
            map.emplace(20, 1);
            map.emplace(30, 1);
            map.emplace(40, 1);
            map.emplace(50, 1);

            CHECK(map.size() == 5);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
            CHECK(map.nth(3)->first == 40); CHECK(map.nth(3)->second == 1);
            CHECK(map.nth(4)->first == 50); CHECK(map.nth(4)->second == 1);

            ///////////////////////////////////////////////////////////////////////

            CHECK(map.erase(map.nth(3)) == map.nth(3));
            CHECK(map.size() == 4);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
            CHECK(map.nth(3)->first == 50); CHECK(map.nth(3)->second == 1);

            ///////////////////////////////////////////////////////////////////////

            CHECK(map.erase(map.nth(2)) == map.nth(2));
            CHECK(map.size() == 3);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 50); CHECK(map.nth(2)->second == 1);

            ///////////////////////////////////////////////////////////////////////

            CHECK(map.erase(map.nth(1)) == map.nth(1));
            CHECK(map.size() == 2);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 50); CHECK(map.nth(1)->second == 1);

            ///////////////////////////////////////////////////////////////////////

            CHECK(map.erase(map.nth(0)) == map.nth(0));
            CHECK(map.size() == 1);
            CHECK(map.nth(0)->first == 50); CHECK(map.nth(0)->second == 1);

            ///////////////////////////////////////////////////////////////////////

            CHECK(map.erase(map.nth(0)) == map.nth(0));
            CHECK(map.size() == 0);
        }

        // Erase near the begin
        {
            sfl::static_flat_multimap<xint, xint, 100, std::less<xint>> map;

            map.emplace(10, 1);
            map.emplace(20, 1);
            map.emplace(30, 1);
            map.emplace(40, 1);
            map.emplace(50, 1);

            CHECK(map.size() == 5);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
            CHECK(map.nth(3)->first == 40); CHECK(map.nth(3)->second == 1);
            CHECK(map.nth(4)->first == 50); CHECK(map.nth(4)->second == 1);

            ///////////////////////////////////////////////////////////////////////

            CHECK(map.erase(map.nth(1)) == map.nth(1));
            CHECK(map.size() == 4);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 30); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 40); CHECK(map.nth(2)->second == 1);
            CHECK(map.nth(3)->first == 50); CHECK(map.nth(3)->second == 1);

            ///////////////////////////////////////////////////////////////////////

            CHECK(map.erase(map.nth(1)) == map.nth(1));
            CHECK(map.size() == 3);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 40); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 50); CHECK(map.nth(2)->second == 1);

            ///////////////////////////////////////////////////////////////////////

            CHECK(map.erase(map.nth(1)) == map.nth(1));
            CHECK(map.size() == 2);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 50); CHECK(map.nth(1)->second == 1);

            ///////////////////////////////////////////////////////////////////////

            CHECK(map.erase(map.nth(1)) == map.nth(1));
            CHECK(map.size() == 1);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);

            ///////////////////////////////////////////////////////////////////////

            CHECK(map.erase(map.nth(0)) == map.nth(0));
            CHECK(map.size() == 0);
        }
    }

    PRINT("Test erase(const_iterator, const_iterator)");
    {
        // Erase at the end
        {
            sfl::static_flat_multimap<xint, xint, 100, std::less<xint>> map;

            map.emplace(10, 1);
            map.emplace(20, 1);
            map.emplace(30, 1);
            map.emplace(40, 1);
            map.emplace(50, 1);
            map.emplace(60, 1);
            map.emplace(70, 1);
            map.emplace(80, 1);
            map.emplace(90, 1);

            CHECK(map.size() == 9);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
            CHECK(map.nth(3)->first == 40); CHECK(map.nth(3)->second == 1);
            CHECK(map.nth(4)->first == 50); CHECK(map.nth(4)->second == 1);
            CHECK(map.nth(5)->first == 60); CHECK(map.nth(5)->second == 1);
            CHECK(map.nth(6)->first == 70); CHECK(map.nth(6)->second == 1);
            CHECK(map.nth(7)->first == 80); CHECK(map.nth(7)->second == 1);
            CHECK(map.nth(8)->first == 90); CHECK(map.nth(8)->second == 1);

            ///////////////////////////////////////////////////////////////////////

            CHECK(map.erase(map.nth(6), map.nth(9)) == map.nth(6));
            CHECK(map.size() == 6);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
            CHECK(map.nth(3)->first == 40); CHECK(map.nth(3)->second == 1);
            CHECK(map.nth(4)->first == 50); CHECK(map.nth(4)->second == 1);
            CHECK(map.nth(5)->first == 60); CHECK(map.nth(5)->second == 1);

            ///////////////////////////////////////////////////////////////////////

            CHECK(map.erase(map.nth(3), map.nth(6)) == map.nth(3));
            CHECK(map.size() == 3);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);

            ///////////////////////////////////////////////////////////////////////

            CHECK(map.erase(map.nth(0), map.nth(3)) == map.nth(0));
            CHECK(map.size() == 0);
        }

        // Erase at the begin
        {
            sfl::static_flat_multimap<xint, xint, 100, std::less<xint>> map;

            map.emplace(10, 1);
            map.emplace(20, 1);
            map.emplace(30, 1);
            map.emplace(40, 1);
            map.emplace(50, 1);
            map.emplace(60, 1);
            map.emplace(70, 1);
            map.emplace(80, 1);
            map.emplace(90, 1);

            CHECK(map.size() == 9);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
            CHECK(map.nth(3)->first == 40); CHECK(map.nth(3)->second == 1);
            CHECK(map.nth(4)->first == 50); CHECK(map.nth(4)->second == 1);
            CHECK(map.nth(5)->first == 60); CHECK(map.nth(5)->second == 1);
            CHECK(map.nth(6)->first == 70); CHECK(map.nth(6)->second == 1);
            CHECK(map.nth(7)->first == 80); CHECK(map.nth(7)->second == 1);
            CHECK(map.nth(8)->first == 90); CHECK(map.nth(8)->second == 1);

            ///////////////////////////////////////////////////////////////////////

            CHECK(map.erase(map.nth(0), map.nth(3)) == map.nth(0));
            CHECK(map.size() == 6);
            CHECK(map.nth(0)->first == 40); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 50); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 60); CHECK(map.nth(2)->second == 1);
            CHECK(map.nth(3)->first == 70); CHECK(map.nth(3)->second == 1);
            CHECK(map.nth(4)->first == 80); CHECK(map.nth(4)->second == 1);
            CHECK(map.nth(5)->first == 90); CHECK(map.nth(5)->second == 1);

            ///////////////////////////////////////////////////////////////////////

            CHECK(map.erase(map.nth(0), map.nth(3)) == map.nth(0));
            CHECK(map.size() == 3);
            CHECK(map.nth(0)->first == 70); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 80); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 90); CHECK(map.nth(2)->second == 1);

            ///////////////////////////////////////////////////////////////////////

            CHECK(map.erase(map.nth(0), map.nth(3)) == map.nth(0));
            CHECK(map.size() == 0);
        }

        // Erase near the end
        {
            sfl::static_flat_multimap<xint, xint, 100, std::less<xint>> map;

            map.emplace(10, 1);
            map.emplace(20, 1);
            map.emplace(30, 1);
            map.emplace(40, 1);
            map.emplace(50, 1);
            map.emplace(60, 1);
            map.emplace(70, 1);
            map.emplace(80, 1);
            map.emplace(90, 1);

            CHECK(map.size() == 9);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
            CHECK(map.nth(3)->first == 40); CHECK(map.nth(3)->second == 1);
            CHECK(map.nth(4)->first == 50); CHECK(map.nth(4)->second == 1);
            CHECK(map.nth(5)->first == 60); CHECK(map.nth(5)->second == 1);
            CHECK(map.nth(6)->first == 70); CHECK(map.nth(6)->second == 1);
            CHECK(map.nth(7)->first == 80); CHECK(map.nth(7)->second == 1);
            CHECK(map.nth(8)->first == 90); CHECK(map.nth(8)->second == 1);

            ///////////////////////////////////////////////////////////////////////

            CHECK(map.erase(map.nth(5), map.nth(8)) == map.nth(5));
            CHECK(map.size() == 6);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
            CHECK(map.nth(3)->first == 40); CHECK(map.nth(3)->second == 1);
            CHECK(map.nth(4)->first == 50); CHECK(map.nth(4)->second == 1);
            CHECK(map.nth(5)->first == 90); CHECK(map.nth(5)->second == 1);

            ///////////////////////////////////////////////////////////////////////

            CHECK(map.erase(map.nth(2), map.nth(5)) == map.nth(2));
            CHECK(map.size() == 3);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 90); CHECK(map.nth(2)->second == 1);

            ///////////////////////////////////////////////////////////////////////

            CHECK(map.erase(map.nth(1), map.nth(2)) == map.nth(1));
            CHECK(map.size() == 2);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 90); CHECK(map.nth(1)->second == 1);

            ///////////////////////////////////////////////////////////////////////

            CHECK(map.erase(map.nth(0), map.nth(2)) == map.nth(0));
            CHECK(map.size() == 0);
        }

        // Erase near the begin
        {
            sfl::static_flat_multimap<xint, xint, 100, std::less<xint>> map;

            map.emplace(10, 1);
            map.emplace(20, 1);
            map.emplace(30, 1);
            map.emplace(40, 1);
            map.emplace(50, 1);
            map.emplace(60, 1);
            map.emplace(70, 1);
            map.emplace(80, 1);
            map.emplace(90, 1);

            CHECK(map.size() == 9);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
            CHECK(map.nth(3)->first == 40); CHECK(map.nth(3)->second == 1);
            CHECK(map.nth(4)->first == 50); CHECK(map.nth(4)->second == 1);
            CHECK(map.nth(5)->first == 60); CHECK(map.nth(5)->second == 1);
            CHECK(map.nth(6)->first == 70); CHECK(map.nth(6)->second == 1);
            CHECK(map.nth(7)->first == 80); CHECK(map.nth(7)->second == 1);
            CHECK(map.nth(8)->first == 90); CHECK(map.nth(8)->second == 1);

            ///////////////////////////////////////////////////////////////////////

            CHECK(map.erase(map.nth(1), map.nth(4)) == map.nth(1));
            CHECK(map.size() == 6);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 50); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 60); CHECK(map.nth(2)->second == 1);
            CHECK(map.nth(3)->first == 70); CHECK(map.nth(3)->second == 1);
            CHECK(map.nth(4)->first == 80); CHECK(map.nth(4)->second == 1);
            CHECK(map.nth(5)->first == 90); CHECK(map.nth(5)->second == 1);

            ///////////////////////////////////////////////////////////////////////

            CHECK(map.erase(map.nth(1), map.nth(4)) == map.nth(1));
            CHECK(map.size() == 3);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 80); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 90); CHECK(map.nth(2)->second == 1);

            ///////////////////////////////////////////////////////////////////////

            CHECK(map.erase(map.nth(1), map.nth(2)) == map.nth(1));
            CHECK(map.size() == 2);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 90); CHECK(map.nth(1)->second == 1);

            ///////////////////////////////////////////////////////////////////////

            CHECK(map.erase(map.nth(0), map.nth(2)) == map.nth(0));
            CHECK(map.size() == 0);
        }
    }

    PRINT("Test erase(const Key&)");
    {
        sfl::static_flat_multimap<xint, xint, 100, std::less<xint>> map;

        map.emplace(10, 1);
        map.emplace(20, 1);
        map.emplace(20, 2);
        map.emplace(20, 3);
        map.emplace(30, 1);

        CHECK(map.size() == 5);
        CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
        CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 3);
        CHECK(map.nth(2)->first == 20); CHECK(map.nth(2)->second == 2);
        CHECK(map.nth(3)->first == 20); CHECK(map.nth(3)->second == 1);
        CHECK(map.nth(4)->first == 30); CHECK(map.nth(4)->second == 1);

        CHECK(map.erase(30) == 1);
        CHECK(map.erase(30) == 0);
        CHECK(map.size() == 4);
        CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
        CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 3);
        CHECK(map.nth(2)->first == 20); CHECK(map.nth(2)->second == 2);
        CHECK(map.nth(3)->first == 20); CHECK(map.nth(3)->second == 1);

        CHECK(map.erase(20) == 3);
        CHECK(map.erase(20) == 0);
        CHECK(map.size() == 1);
        CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);

        CHECK(map.erase(10) == 1);
        CHECK(map.erase(10) == 0);
        CHECK(map.size() == 0);
    }

    PRINT("Test erase(K&&)");
    {
        sfl::static_flat_multimap<xobj, xint, 100, xobj::less> map;

        map.emplace(std::piecewise_construct, std::forward_as_tuple(10), std::forward_as_tuple(1));
        map.emplace(std::piecewise_construct, std::forward_as_tuple(20), std::forward_as_tuple(1));
        map.emplace(std::piecewise_construct, std::forward_as_tuple(20), std::forward_as_tuple(2));
        map.emplace(std::piecewise_construct, std::forward_as_tuple(20), std::forward_as_tuple(3));
        map.emplace(std::piecewise_construct, std::forward_as_tuple(30), std::forward_as_tuple(1));

        CHECK(map.size() == 5);
        CHECK(map.nth(0)->first.value() == 10); CHECK(map.nth(0)->second == 1);
        CHECK(map.nth(1)->first.value() == 20); CHECK(map.nth(1)->second == 3);
        CHECK(map.nth(2)->first.value() == 20); CHECK(map.nth(2)->second == 2);
        CHECK(map.nth(3)->first.value() == 20); CHECK(map.nth(3)->second == 1);
        CHECK(map.nth(4)->first.value() == 30); CHECK(map.nth(4)->second == 1);

        CHECK(map.erase(30) == 1);
        CHECK(map.erase(30) == 0);
        CHECK(map.size() == 4);
        CHECK(map.nth(0)->first.value() == 10); CHECK(map.nth(0)->second == 1);
        CHECK(map.nth(1)->first.value() == 20); CHECK(map.nth(1)->second == 3);
        CHECK(map.nth(2)->first.value() == 20); CHECK(map.nth(2)->second == 2);
        CHECK(map.nth(3)->first.value() == 20); CHECK(map.nth(3)->second == 1);

        CHECK(map.erase(20) == 3);
        CHECK(map.erase(20) == 0);
        CHECK(map.size() == 1);
        CHECK(map.nth(0)->first.value() == 10); CHECK(map.nth(0)->second == 1);

        CHECK(map.erase(10) == 1);
        CHECK(map.erase(10) == 0);
        CHECK(map.size() == 0);
    }

    PRINT("Test swap(container&)");
    {
        // Swap with self
        {
            sfl::static_flat_multimap<xint, xint, 100, std::less<xint>> map;

            map.emplace(10, 1);
            map.emplace(20, 1);
            map.emplace(30, 1);

            CHECK(map.size() == 3);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);

            ///////////////////////////////////////////////////////////////////

            map.swap(map);

            CHECK(map.size() == 3);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
        }

        // map1.size() == map2.size()
        {
            sfl::static_flat_multimap<xint, xint, 100, std::less<xint>> map1, map2;

            map1.emplace(10, 1);
            map1.emplace(20, 1);
            map1.emplace(30, 1);

            map2.emplace(40, 2);
            map2.emplace(50, 2);
            map2.emplace(60, 2);

            CHECK(map1.size() == 3);
            CHECK(map1.nth(0)->first == 10); CHECK(map1.nth(0)->second == 1);
            CHECK(map1.nth(1)->first == 20); CHECK(map1.nth(1)->second == 1);
            CHECK(map1.nth(2)->first == 30); CHECK(map1.nth(2)->second == 1);

            CHECK(map2.size() == 3);
            CHECK(map2.nth(0)->first == 40); CHECK(map2.nth(0)->second == 2);
            CHECK(map2.nth(1)->first == 50); CHECK(map2.nth(1)->second == 2);
            CHECK(map2.nth(2)->first == 60); CHECK(map2.nth(2)->second == 2);

            ///////////////////////////////////////////////////////////////////

            map1.swap(map2);

            CHECK(map1.size() == 3);
            CHECK(map1.nth(0)->first == 40); CHECK(map1.nth(0)->second == 2);
            CHECK(map1.nth(1)->first == 50); CHECK(map1.nth(1)->second == 2);
            CHECK(map1.nth(2)->first == 60); CHECK(map1.nth(2)->second == 2);

            CHECK(map2.size() == 3);
            CHECK(map2.nth(0)->first == 10); CHECK(map2.nth(0)->second == 1);
            CHECK(map2.nth(1)->first == 20); CHECK(map2.nth(1)->second == 1);
            CHECK(map2.nth(2)->first == 30); CHECK(map2.nth(2)->second == 1);

            ///////////////////////////////////////////////////////////////////

            map1.swap(map2);

            CHECK(map1.size() == 3);
            CHECK(map1.nth(0)->first == 10); CHECK(map1.nth(0)->second == 1);
            CHECK(map1.nth(1)->first == 20); CHECK(map1.nth(1)->second == 1);
            CHECK(map1.nth(2)->first == 30); CHECK(map1.nth(2)->second == 1);

            CHECK(map2.size() == 3);
            CHECK(map2.nth(0)->first == 40); CHECK(map2.nth(0)->second == 2);
            CHECK(map2.nth(1)->first == 50); CHECK(map2.nth(1)->second == 2);
            CHECK(map2.nth(2)->first == 60); CHECK(map2.nth(2)->second == 2);
        }

        // map1.size() != map2.size()
        {
            sfl::static_flat_multimap<xint, xint, 100, std::less<xint>> map1, map2;

            map1.emplace(10, 1);
            map1.emplace(20, 1);
            map1.emplace(30, 1);

            map2.emplace(40, 2);
            map2.emplace(50, 2);
            map2.emplace(60, 2);
            map2.emplace(70, 2);
            map2.emplace(80, 2);

            CHECK(map1.size() == 3);
            CHECK(map1.nth(0)->first == 10); CHECK(map1.nth(0)->second == 1);
            CHECK(map1.nth(1)->first == 20); CHECK(map1.nth(1)->second == 1);
            CHECK(map1.nth(2)->first == 30); CHECK(map1.nth(2)->second == 1);

            CHECK(map2.size() == 5);
            CHECK(map2.nth(0)->first == 40); CHECK(map2.nth(0)->second == 2);
            CHECK(map2.nth(1)->first == 50); CHECK(map2.nth(1)->second == 2);
            CHECK(map2.nth(2)->first == 60); CHECK(map2.nth(2)->second == 2);
            CHECK(map2.nth(3)->first == 70); CHECK(map2.nth(3)->second == 2);
            CHECK(map2.nth(4)->first == 80); CHECK(map2.nth(4)->second == 2);

            ///////////////////////////////////////////////////////////////////

            map1.swap(map2);

            CHECK(map1.size() == 5);
            CHECK(map1.nth(0)->first == 40); CHECK(map1.nth(0)->second == 2);
            CHECK(map1.nth(1)->first == 50); CHECK(map1.nth(1)->second == 2);
            CHECK(map1.nth(2)->first == 60); CHECK(map1.nth(2)->second == 2);
            CHECK(map1.nth(3)->first == 70); CHECK(map1.nth(3)->second == 2);
            CHECK(map1.nth(4)->first == 80); CHECK(map1.nth(4)->second == 2);

            CHECK(map2.size() == 3);
            CHECK(map2.nth(0)->first == 10); CHECK(map2.nth(0)->second == 1);
            CHECK(map2.nth(1)->first == 20); CHECK(map2.nth(1)->second == 1);
            CHECK(map2.nth(2)->first == 30); CHECK(map2.nth(2)->second == 1);

            ///////////////////////////////////////////////////////////////////

            map1.swap(map2);

            CHECK(map1.size() == 3);
            CHECK(map1.nth(0)->first == 10); CHECK(map1.nth(0)->second == 1);
            CHECK(map1.nth(1)->first == 20); CHECK(map1.nth(1)->second == 1);
            CHECK(map1.nth(2)->first == 30); CHECK(map1.nth(2)->second == 1);

            CHECK(map2.size() == 5);
            CHECK(map2.nth(0)->first == 40); CHECK(map2.nth(0)->second == 2);
            CHECK(map2.nth(1)->first == 50); CHECK(map2.nth(1)->second == 2);
            CHECK(map2.nth(2)->first == 60); CHECK(map2.nth(2)->second == 2);
            CHECK(map2.nth(3)->first == 70); CHECK(map2.nth(3)->second == 2);
            CHECK(map2.nth(4)->first == 80); CHECK(map2.nth(4)->second == 2);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////

    PRINT("Test data()");
    {
        sfl::static_flat_multimap<xint, xint, 100, std::less<xint>> map;

        map.emplace(10, 1);
        map.emplace(20, 1);
        map.emplace(30, 1);

        CHECK(map.size() == 3);
        CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
        CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
        CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////////

        auto data = map.data();
        CHECK(data->first == 10); CHECK(data->second == 1); ++data;
        CHECK(data->first == 20); CHECK(data->second == 1); ++data;
        CHECK(data->first == 30); CHECK(data->second == 1); ++data;
    }

    ///////////////////////////////////////////////////////////////////////////////

    PRINT("Test container()");
    {
        sfl::static_flat_multimap<xint, xint, 100, std::less<xint>> map;

        CHECK(map.size() == 0);
        CHECK(map.capacity() == 100);
        CHECK(map.available() == 100);
    }

    PRINT("Test container(const Compare&)");
    {
        std::less<xint> comp;

        sfl::static_flat_multimap<xint, xint, 100, std::less<xint>> map(comp);

        CHECK(map.size() == 0);
        CHECK(map.capacity() == 100);
        CHECK(map.available() == 100);
    }

    PRINT("Test container(InputIt, InputIt)");
    {
        std::vector<std::pair<xint, xint>> data
        (
            {
                {20, 1},
                {40, 1},
                {60, 1},

                {10, 1},
                {30, 1},
                {50, 1},

                {20, 2},
                {40, 2},
                {60, 2},

                {10, 2},
                {30, 2},
                {50, 2}
            }
        );

        sfl::static_flat_multimap<xint, xint, 100, std::less<xint>> map(data.begin(), data.end());

        CHECK(map.size() == 12);
        CHECK(map.nth( 0)->first == 10); CHECK(map.nth( 0)->second == 2);
        CHECK(map.nth( 1)->first == 10); CHECK(map.nth( 1)->second == 1);
        CHECK(map.nth( 2)->first == 20); CHECK(map.nth( 2)->second == 2);
        CHECK(map.nth( 3)->first == 20); CHECK(map.nth( 3)->second == 1);
        CHECK(map.nth( 4)->first == 30); CHECK(map.nth( 4)->second == 2);
        CHECK(map.nth( 5)->first == 30); CHECK(map.nth( 5)->second == 1);
        CHECK(map.nth( 6)->first == 40); CHECK(map.nth( 6)->second == 2);
        CHECK(map.nth( 7)->first == 40); CHECK(map.nth( 7)->second == 1);
        CHECK(map.nth( 8)->first == 50); CHECK(map.nth( 8)->second == 2);
        CHECK(map.nth( 9)->first == 50); CHECK(map.nth( 9)->second == 1);
        CHECK(map.nth(10)->first == 60); CHECK(map.nth(10)->second == 2);
        CHECK(map.nth(11)->first == 60); CHECK(map.nth(11)->second == 1);
    }

    PRINT("Test container(InputIt, InputIt, const Compare&)");
    {
        std::vector<std::pair<xint, xint>> data
        (
            {
                {20, 1},
                {40, 1},
                {60, 1},

                {10, 1},
                {30, 1},
                {50, 1},

                {20, 2},
                {40, 2},
                {60, 2},

                {10, 2},
                {30, 2},
                {50, 2}
            }
        );

        std::less<xint> comp;

        sfl::static_flat_multimap<xint, xint, 100, std::less<xint>> map(data.begin(), data.end(), comp);

        CHECK(map.size() == 12);
        CHECK(map.nth( 0)->first == 10); CHECK(map.nth( 0)->second == 2);
        CHECK(map.nth( 1)->first == 10); CHECK(map.nth( 1)->second == 1);
        CHECK(map.nth( 2)->first == 20); CHECK(map.nth( 2)->second == 2);
        CHECK(map.nth( 3)->first == 20); CHECK(map.nth( 3)->second == 1);
        CHECK(map.nth( 4)->first == 30); CHECK(map.nth( 4)->second == 2);
        CHECK(map.nth( 5)->first == 30); CHECK(map.nth( 5)->second == 1);
        CHECK(map.nth( 6)->first == 40); CHECK(map.nth( 6)->second == 2);
        CHECK(map.nth( 7)->first == 40); CHECK(map.nth( 7)->second == 1);
        CHECK(map.nth( 8)->first == 50); CHECK(map.nth( 8)->second == 2);
        CHECK(map.nth( 9)->first == 50); CHECK(map.nth( 9)->second == 1);
        CHECK(map.nth(10)->first == 60); CHECK(map.nth(10)->second == 2);
        CHECK(map.nth(11)->first == 60); CHECK(map.nth(11)->second == 1);
    }

    PRINT("Test container(std::initializer_list)");
    {
        std::initializer_list<std::pair<xint, xint>> ilist
        {
            {20, 1},
            {40, 1},
            {60, 1},

            {10, 1},
            {30, 1},
            {50, 1},

            {20, 2},
            {40, 2},
            {60, 2},

            {10, 2},
            {30, 2},
            {50, 2}
        };

        sfl::static_flat_multimap<xint, xint, 100, std::less<xint>> map(ilist);

        CHECK(map.size() == 12);
        CHECK(map.nth( 0)->first == 10); CHECK(map.nth( 0)->second == 2);
        CHECK(map.nth( 1)->first == 10); CHECK(map.nth( 1)->second == 1);
        CHECK(map.nth( 2)->first == 20); CHECK(map.nth( 2)->second == 2);
        CHECK(map.nth( 3)->first == 20); CHECK(map.nth( 3)->second == 1);
        CHECK(map.nth( 4)->first == 30); CHECK(map.nth( 4)->second == 2);
        CHECK(map.nth( 5)->first == 30); CHECK(map.nth( 5)->second == 1);
        CHECK(map.nth( 6)->first == 40); CHECK(map.nth( 6)->second == 2);
        CHECK(map.nth( 7)->first == 40); CHECK(map.nth( 7)->second == 1);
        CHECK(map.nth( 8)->first == 50); CHECK(map.nth( 8)->second == 2);
        CHECK(map.nth( 9)->first == 50); CHECK(map.nth( 9)->second == 1);
        CHECK(map.nth(10)->first == 60); CHECK(map.nth(10)->second == 2);
        CHECK(map.nth(11)->first == 60); CHECK(map.nth(11)->second == 1);
    }

    PRINT("Test container(std::initializer_list, const Compare&)");
    {
        std::initializer_list<std::pair<xint, xint>> ilist
        {
            {20, 1},
            {40, 1},
            {60, 1},

            {10, 1},
            {30, 1},
            {50, 1},

            {20, 2},
            {40, 2},
            {60, 2},

            {10, 2},
            {30, 2},
            {50, 2}
        };

        std::less<xint> comp;

        sfl::static_flat_multimap<xint, xint, 100, std::less<xint>> map(ilist, comp);

        CHECK(map.size() == 12);
        CHECK(map.nth( 0)->first == 10); CHECK(map.nth( 0)->second == 2);
        CHECK(map.nth( 1)->first == 10); CHECK(map.nth( 1)->second == 1);
        CHECK(map.nth( 2)->first == 20); CHECK(map.nth( 2)->second == 2);
        CHECK(map.nth( 3)->first == 20); CHECK(map.nth( 3)->second == 1);
        CHECK(map.nth( 4)->first == 30); CHECK(map.nth( 4)->second == 2);
        CHECK(map.nth( 5)->first == 30); CHECK(map.nth( 5)->second == 1);
        CHECK(map.nth( 6)->first == 40); CHECK(map.nth( 6)->second == 2);
        CHECK(map.nth( 7)->first == 40); CHECK(map.nth( 7)->second == 1);
        CHECK(map.nth( 8)->first == 50); CHECK(map.nth( 8)->second == 2);
        CHECK(map.nth( 9)->first == 50); CHECK(map.nth( 9)->second == 1);
        CHECK(map.nth(10)->first == 60); CHECK(map.nth(10)->second == 2);
        CHECK(map.nth(11)->first == 60); CHECK(map.nth(11)->second == 1);
    }

    PRINT("Test container(const container&)");
    {
        sfl::static_flat_multimap<xint, xint, 100, std::less<xint>> map1;

        map1.emplace(10, 1);
        map1.emplace(20, 1);
        map1.emplace(30, 1);

        CHECK(map1.size() == 3);
        CHECK(map1.nth(0)->first == 10); CHECK(map1.nth(0)->second == 1);
        CHECK(map1.nth(1)->first == 20); CHECK(map1.nth(1)->second == 1);
        CHECK(map1.nth(2)->first == 30); CHECK(map1.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        sfl::static_flat_multimap<xint, xint, 100, std::less<xint>> map2(map1);

        CHECK(map2.size() == 3);
        CHECK(map2.nth(0)->first == 10); CHECK(map2.nth(0)->second == 1);
        CHECK(map2.nth(1)->first == 20); CHECK(map2.nth(1)->second == 1);
        CHECK(map2.nth(2)->first == 30); CHECK(map2.nth(2)->second == 1);
    }

    PRINT("Test container(container&&)");
    {
        sfl::static_flat_multimap<xint, xint, 100, std::less<xint>> map1;

        map1.emplace(10, 1);
        map1.emplace(20, 1);
        map1.emplace(30, 1);

        CHECK(map1.size() == 3);
        CHECK(map1.nth(0)->first == 10); CHECK(map1.nth(0)->second == 1);
        CHECK(map1.nth(1)->first == 20); CHECK(map1.nth(1)->second == 1);
        CHECK(map1.nth(2)->first == 30); CHECK(map1.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        sfl::static_flat_multimap<xint, xint, 100, std::less<xint>> map2(std::move(map1));

        CHECK(map2.size() == 3);
        CHECK(map2.nth(0)->first == 10); CHECK(map2.nth(0)->second == 1);
        CHECK(map2.nth(1)->first == 20); CHECK(map2.nth(1)->second == 1);
        CHECK(map2.nth(2)->first == 30); CHECK(map2.nth(2)->second == 1);

        CHECK(map1.size() == 3);
        CHECK(map1.nth(0)->first == -10); CHECK(map1.nth(0)->second == -1);
        CHECK(map1.nth(1)->first == -20); CHECK(map1.nth(1)->second == -1);
        CHECK(map1.nth(2)->first == -30); CHECK(map1.nth(2)->second == -1);
    }

    PRINT("Test container(sfl::from_range_t, Range&&)");
    {
        // Input iterator (exactly)
        {
            std::istringstream iss("10 1 20 1 30 1 20 2 20 3");

            #if SFL_CPP_VERSION >= SFL_CPP_20
            sfl::static_flat_multimap<xint, xint, 32, std::less<xint>> map
            (
                (sfl::from_range_t()),
                (std::views::istream<std::pair<int, int>>(iss))
            );
            #else
            sfl::static_flat_multimap<xint, xint, 32, std::less<xint>> map
            (
                (sfl::from_range_t()),
                (sfl::test::istream_view<std::pair<int, int>>(iss))
            );
            #endif

            CHECK(map.empty() == false);
            CHECK(map.size() == 5);
            CHECK(map.max_size() > 0);
            CHECK(NTH(map, 0)->first == 10); CHECK(NTH(map, 0)->second == 1);
            CHECK(NTH(map, 1)->first == 20); CHECK(NTH(map, 1)->second == 3);
            CHECK(NTH(map, 2)->first == 20); CHECK(NTH(map, 2)->second == 2);
            CHECK(NTH(map, 3)->first == 20); CHECK(NTH(map, 3)->second == 1);
            CHECK(NTH(map, 4)->first == 30); CHECK(NTH(map, 4)->second == 1);
        }

        // Forward iterator
        {
            std::vector<std::pair<xint, xint>> data
            (
                {
                    {10, 1},
                    {20, 1},
                    {30, 1},
                    {20, 2},
                    {20, 3}
                }
            );

            #if SFL_CPP_VERSION >= SFL_CPP_20
            sfl::static_flat_multimap<xint, xint, 32, std::less<xint>> map
            (
                sfl::from_range_t(),
                std::views::all(data)
            );
            #else
            sfl::static_flat_multimap<xint, xint, 32, std::less<xint>> map
            (
                sfl::from_range_t(),
                data
            );
            #endif

            CHECK(map.empty() == false);
            CHECK(map.size() == 5);
            CHECK(map.max_size() > 0);
            CHECK(NTH(map, 0)->first == 10); CHECK(NTH(map, 0)->second == 1);
            CHECK(NTH(map, 1)->first == 20); CHECK(NTH(map, 1)->second == 3);
            CHECK(NTH(map, 2)->first == 20); CHECK(NTH(map, 2)->second == 2);
            CHECK(NTH(map, 3)->first == 20); CHECK(NTH(map, 3)->second == 1);
            CHECK(NTH(map, 4)->first == 30); CHECK(NTH(map, 4)->second == 1);
        }
    }

    PRINT("Test container(sfl::from_range_t, Range&&, const Compare&)");
    {
        // Input iterator (exactly)
        {
            std::istringstream iss("10 1 20 1 30 1 20 2 20 3");

            std::less<xint> comp;

            #if SFL_CPP_VERSION >= SFL_CPP_20
            sfl::static_flat_multimap<xint, xint, 32, std::less<xint>> map
            (
                (sfl::from_range_t()),
                (std::views::istream<std::pair<int, int>>(iss)),
                comp
            );
            #else
            sfl::static_flat_multimap<xint, xint, 32, std::less<xint>> map
            (
                (sfl::from_range_t()),
                (sfl::test::istream_view<std::pair<int, int>>(iss)),
                comp
            );
            #endif

            CHECK(map.empty() == false);
            CHECK(map.size() == 5);
            CHECK(map.max_size() > 0);
            CHECK(NTH(map, 0)->first == 10); CHECK(NTH(map, 0)->second == 1);
            CHECK(NTH(map, 1)->first == 20); CHECK(NTH(map, 1)->second == 3);
            CHECK(NTH(map, 2)->first == 20); CHECK(NTH(map, 2)->second == 2);
            CHECK(NTH(map, 3)->first == 20); CHECK(NTH(map, 3)->second == 1);
            CHECK(NTH(map, 4)->first == 30); CHECK(NTH(map, 4)->second == 1);
        }

        // Forward iterator
        {
            std::vector<std::pair<xint, xint>> data
            (
                {
                    {10, 1},
                    {20, 1},
                    {30, 1},
                    {20, 2},
                    {20, 3}
                }
            );

            std::less<xint> comp;

            #if SFL_CPP_VERSION >= SFL_CPP_20
            sfl::static_flat_multimap<xint, xint, 32, std::less<xint>> map
            (
                sfl::from_range_t(),
                std::views::all(data),
                comp
            );
            #else
            sfl::static_flat_multimap<xint, xint, 32, std::less<xint>> map
            (
                sfl::from_range_t(),
                data,
                comp
            );
            #endif

            CHECK(map.empty() == false);
            CHECK(map.size() == 5);
            CHECK(map.max_size() > 0);
            CHECK(NTH(map, 0)->first == 10); CHECK(NTH(map, 0)->second == 1);
            CHECK(NTH(map, 1)->first == 20); CHECK(NTH(map, 1)->second == 3);
            CHECK(NTH(map, 2)->first == 20); CHECK(NTH(map, 2)->second == 2);
            CHECK(NTH(map, 3)->first == 20); CHECK(NTH(map, 3)->second == 1);
            CHECK(NTH(map, 4)->first == 30); CHECK(NTH(map, 4)->second == 1);
        }
    }

    ///////////////////////////////////////////////////////////////////////////

    PRINT("Test operator=(const container&)");
    {
        #define CONDITION map1.size() == map2.size()
        {
            sfl::static_flat_multimap<xint, xint, 100, std::less<xint>> map1, map2;

            map1.emplace(10, 1);
            map1.emplace(20, 1);
            map1.emplace(30, 1);

            CHECK(map1.size() == 3);
            CHECK(map1.nth(0)->first == 10); CHECK(map1.nth(0)->second == 1);
            CHECK(map1.nth(1)->first == 20); CHECK(map1.nth(1)->second == 1);
            CHECK(map1.nth(2)->first == 30); CHECK(map1.nth(2)->second == 1);

            map2.emplace(40, 2);
            map2.emplace(50, 2);
            map2.emplace(60, 2);

            CHECK(map2.size() == 3);
            CHECK(map2.nth(0)->first == 40); CHECK(map2.nth(0)->second == 2);
            CHECK(map2.nth(1)->first == 50); CHECK(map2.nth(1)->second == 2);
            CHECK(map2.nth(2)->first == 60); CHECK(map2.nth(2)->second == 2);

            ///////////////////////////////////////////////////////////////////

            CHECK(CONDITION);

            map1 = map2;

            CHECK(map1.size() == 3);
            CHECK(map1.nth(0)->first == 40); CHECK(map1.nth(0)->second == 2);
            CHECK(map1.nth(1)->first == 50); CHECK(map1.nth(1)->second == 2);
            CHECK(map1.nth(2)->first == 60); CHECK(map1.nth(2)->second == 2);

            CHECK(map2.size() == 3);
            CHECK(map2.nth(0)->first == 40); CHECK(map2.nth(0)->second == 2);
            CHECK(map2.nth(1)->first == 50); CHECK(map2.nth(1)->second == 2);
            CHECK(map2.nth(2)->first == 60); CHECK(map2.nth(2)->second == 2);
        }
        #undef CONDITION

        #define CONDITION map1.size() < map2.size()
        {
            sfl::static_flat_multimap<xint, xint, 100, std::less<xint>> map1, map2;

            map1.emplace(10, 1);
            map1.emplace(20, 1);
            map1.emplace(30, 1);

            CHECK(map1.size() == 3);
            CHECK(map1.nth(0)->first == 10); CHECK(map1.nth(0)->second == 1);
            CHECK(map1.nth(1)->first == 20); CHECK(map1.nth(1)->second == 1);
            CHECK(map1.nth(2)->first == 30); CHECK(map1.nth(2)->second == 1);

            map2.emplace(40, 2);
            map2.emplace(50, 2);
            map2.emplace(60, 2);
            map2.emplace(70, 2);
            map2.emplace(80, 2);

            CHECK(map2.size() == 5);
            CHECK(map2.nth(0)->first == 40); CHECK(map2.nth(0)->second == 2);
            CHECK(map2.nth(1)->first == 50); CHECK(map2.nth(1)->second == 2);
            CHECK(map2.nth(2)->first == 60); CHECK(map2.nth(2)->second == 2);
            CHECK(map2.nth(3)->first == 70); CHECK(map2.nth(3)->second == 2);
            CHECK(map2.nth(4)->first == 80); CHECK(map2.nth(4)->second == 2);

            ///////////////////////////////////////////////////////////////////

            CHECK(CONDITION);

            map1 = map2;

            CHECK(map1.size() == 5);
            CHECK(map1.nth(0)->first == 40); CHECK(map1.nth(0)->second == 2);
            CHECK(map1.nth(1)->first == 50); CHECK(map1.nth(1)->second == 2);
            CHECK(map1.nth(2)->first == 60); CHECK(map1.nth(2)->second == 2);
            CHECK(map1.nth(3)->first == 70); CHECK(map1.nth(3)->second == 2);
            CHECK(map1.nth(4)->first == 80); CHECK(map1.nth(4)->second == 2);

            CHECK(map2.size() == 5);
            CHECK(map2.nth(0)->first == 40); CHECK(map2.nth(0)->second == 2);
            CHECK(map2.nth(1)->first == 50); CHECK(map2.nth(1)->second == 2);
            CHECK(map2.nth(2)->first == 60); CHECK(map2.nth(2)->second == 2);
            CHECK(map2.nth(3)->first == 70); CHECK(map2.nth(3)->second == 2);
            CHECK(map2.nth(4)->first == 80); CHECK(map2.nth(4)->second == 2);
        }
        #undef CONDITION

        #define CONDITION map1.size() > map2.size()
        {
            sfl::static_flat_multimap<xint, xint, 100, std::less<xint>> map1, map2;

            map1.emplace(10, 1);
            map1.emplace(20, 1);
            map1.emplace(30, 1);
            map1.emplace(40, 1);
            map1.emplace(50, 1);

            CHECK(map1.size() == 5);
            CHECK(map1.nth(0)->first == 10); CHECK(map1.nth(0)->second == 1);
            CHECK(map1.nth(1)->first == 20); CHECK(map1.nth(1)->second == 1);
            CHECK(map1.nth(2)->first == 30); CHECK(map1.nth(2)->second == 1);
            CHECK(map1.nth(3)->first == 40); CHECK(map1.nth(3)->second == 1);
            CHECK(map1.nth(4)->first == 50); CHECK(map1.nth(4)->second == 1);

            map2.emplace(60, 2);
            map2.emplace(70, 2);
            map2.emplace(80, 2);

            CHECK(map2.size() == 3);
            CHECK(map2.nth(0)->first == 60); CHECK(map2.nth(0)->second == 2);
            CHECK(map2.nth(1)->first == 70); CHECK(map2.nth(1)->second == 2);
            CHECK(map2.nth(2)->first == 80); CHECK(map2.nth(2)->second == 2);

            ///////////////////////////////////////////////////////////////////

            CHECK(CONDITION);

            map1 = map2;

            CHECK(map1.size() == 3);
            CHECK(map1.nth(0)->first == 60); CHECK(map1.nth(0)->second == 2);
            CHECK(map1.nth(1)->first == 70); CHECK(map1.nth(1)->second == 2);
            CHECK(map1.nth(2)->first == 80); CHECK(map1.nth(2)->second == 2);

            CHECK(map2.size() == 3);
            CHECK(map2.nth(0)->first == 60); CHECK(map2.nth(0)->second == 2);
            CHECK(map2.nth(1)->first == 70); CHECK(map2.nth(1)->second == 2);
            CHECK(map2.nth(2)->first == 80); CHECK(map2.nth(2)->second == 2);
        }
        #undef CONDITION
    }

    PRINT("Test operator=(container&&)");
    {
        #define CONDITION map1.size() == map2.size()
        {
            sfl::static_flat_multimap<xint, xint, 100, std::less<xint>> map1, map2;

            map1.emplace(10, 1);
            map1.emplace(20, 1);
            map1.emplace(30, 1);

            CHECK(map1.size() == 3);
            CHECK(map1.nth(0)->first == 10); CHECK(map1.nth(0)->second == 1);
            CHECK(map1.nth(1)->first == 20); CHECK(map1.nth(1)->second == 1);
            CHECK(map1.nth(2)->first == 30); CHECK(map1.nth(2)->second == 1);

            map2.emplace(40, 2);
            map2.emplace(50, 2);
            map2.emplace(60, 2);

            CHECK(map2.size() == 3);
            CHECK(map2.nth(0)->first == 40); CHECK(map2.nth(0)->second == 2);
            CHECK(map2.nth(1)->first == 50); CHECK(map2.nth(1)->second == 2);
            CHECK(map2.nth(2)->first == 60); CHECK(map2.nth(2)->second == 2);

            ///////////////////////////////////////////////////////////////////

            CHECK(CONDITION);

            map1 = std::move(map2);

            CHECK(map1.size() == 3);
            CHECK(map1.nth(0)->first == 40); CHECK(map1.nth(0)->second == 2);
            CHECK(map1.nth(1)->first == 50); CHECK(map1.nth(1)->second == 2);
            CHECK(map1.nth(2)->first == 60); CHECK(map1.nth(2)->second == 2);

            CHECK(map2.size() == 3);
            CHECK(map2.nth(0)->first == -40); CHECK(map2.nth(0)->second == -2);
            CHECK(map2.nth(1)->first == -50); CHECK(map2.nth(1)->second == -2);
            CHECK(map2.nth(2)->first == -60); CHECK(map2.nth(2)->second == -2);
        }
        #undef CONDITION

        #define CONDITION map1.size() < map2.size()
        {
            sfl::static_flat_multimap<xint, xint, 100, std::less<xint>> map1, map2;

            map1.emplace(10, 1);
            map1.emplace(20, 1);
            map1.emplace(30, 1);

            CHECK(map1.size() == 3);
            CHECK(map1.nth(0)->first == 10); CHECK(map1.nth(0)->second == 1);
            CHECK(map1.nth(1)->first == 20); CHECK(map1.nth(1)->second == 1);
            CHECK(map1.nth(2)->first == 30); CHECK(map1.nth(2)->second == 1);

            map2.emplace(40, 2);
            map2.emplace(50, 2);
            map2.emplace(60, 2);
            map2.emplace(70, 2);
            map2.emplace(80, 2);

            CHECK(map2.size() == 5);
            CHECK(map2.nth(0)->first == 40); CHECK(map2.nth(0)->second == 2);
            CHECK(map2.nth(1)->first == 50); CHECK(map2.nth(1)->second == 2);
            CHECK(map2.nth(2)->first == 60); CHECK(map2.nth(2)->second == 2);
            CHECK(map2.nth(3)->first == 70); CHECK(map2.nth(3)->second == 2);
            CHECK(map2.nth(4)->first == 80); CHECK(map2.nth(4)->second == 2);

            ///////////////////////////////////////////////////////////////////

            CHECK(CONDITION);

            map1 = std::move(map2);

            CHECK(map1.size() == 5);
            CHECK(map1.nth(0)->first == 40); CHECK(map1.nth(0)->second == 2);
            CHECK(map1.nth(1)->first == 50); CHECK(map1.nth(1)->second == 2);
            CHECK(map1.nth(2)->first == 60); CHECK(map1.nth(2)->second == 2);
            CHECK(map1.nth(3)->first == 70); CHECK(map1.nth(3)->second == 2);
            CHECK(map1.nth(4)->first == 80); CHECK(map1.nth(4)->second == 2);

            CHECK(map2.size() == 5);
            CHECK(map2.nth(0)->first == -40); CHECK(map2.nth(0)->second == -2);
            CHECK(map2.nth(1)->first == -50); CHECK(map2.nth(1)->second == -2);
            CHECK(map2.nth(2)->first == -60); CHECK(map2.nth(2)->second == -2);
            CHECK(map2.nth(3)->first == -70); CHECK(map2.nth(3)->second == -2);
            CHECK(map2.nth(4)->first == -80); CHECK(map2.nth(4)->second == -2);
        }
        #undef CONDITION

        #define CONDITION map1.size() > map2.size()
        {
            sfl::static_flat_multimap<xint, xint, 100, std::less<xint>> map1, map2;

            map1.emplace(10, 1);
            map1.emplace(20, 1);
            map1.emplace(30, 1);
            map1.emplace(40, 1);
            map1.emplace(50, 1);

            CHECK(map1.size() == 5);
            CHECK(map1.nth(0)->first == 10); CHECK(map1.nth(0)->second == 1);
            CHECK(map1.nth(1)->first == 20); CHECK(map1.nth(1)->second == 1);
            CHECK(map1.nth(2)->first == 30); CHECK(map1.nth(2)->second == 1);
            CHECK(map1.nth(3)->first == 40); CHECK(map1.nth(3)->second == 1);
            CHECK(map1.nth(4)->first == 50); CHECK(map1.nth(4)->second == 1);

            map2.emplace(60, 2);
            map2.emplace(70, 2);
            map2.emplace(80, 2);

            CHECK(map2.size() == 3);
            CHECK(map2.nth(0)->first == 60); CHECK(map2.nth(0)->second == 2);
            CHECK(map2.nth(1)->first == 70); CHECK(map2.nth(1)->second == 2);
            CHECK(map2.nth(2)->first == 80); CHECK(map2.nth(2)->second == 2);

            ///////////////////////////////////////////////////////////////////

            CHECK(CONDITION);

            map1 = std::move(map2);

            CHECK(map1.size() == 3);
            CHECK(map1.nth(0)->first == 60); CHECK(map1.nth(0)->second == 2);
            CHECK(map1.nth(1)->first == 70); CHECK(map1.nth(1)->second == 2);
            CHECK(map1.nth(2)->first == 80); CHECK(map1.nth(2)->second == 2);

            CHECK(map2.size() == 3);
            CHECK(map2.nth(0)->first == -60); CHECK(map2.nth(0)->second == -2);
            CHECK(map2.nth(1)->first == -70); CHECK(map2.nth(1)->second == -2);
            CHECK(map2.nth(2)->first == -80); CHECK(map2.nth(2)->second == -2);
        }
        #undef CONDITION
    }

    PRINT("Test operator=(std::initializer_list)");
    {
        #define CONDITION map.size() == ilist.size()
        {
            sfl::static_flat_multimap<xint, xint, 100, std::less<xint>> map;

            map.emplace(10, 1);
            map.emplace(20, 1);
            map.emplace(30, 1);

            CHECK(map.size() == 3);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);

            std::initializer_list<std::pair<xint, xint>> ilist
            {
                {40, 2},
                {50, 2},
                {60, 2}
            };

            ///////////////////////////////////////////////////////////////////

            CHECK(CONDITION);

            map = ilist;

            CHECK(map.size() == 3);
            CHECK(map.nth(0)->first == 40); CHECK(map.nth(0)->second == 2);
            CHECK(map.nth(1)->first == 50); CHECK(map.nth(1)->second == 2);
            CHECK(map.nth(2)->first == 60); CHECK(map.nth(2)->second == 2);
        }
        #undef CONDITION

        #define CONDITION map.size() < ilist.size()
        {
            sfl::static_flat_multimap<xint, xint, 100, std::less<xint>> map;

            map.emplace(10, 1);
            map.emplace(20, 1);
            map.emplace(30, 1);

            CHECK(map.size() == 3);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);

            std::initializer_list<std::pair<xint, xint>> ilist
            {
                {40, 2},
                {50, 2},
                {60, 2},
                {70, 2},
                {80, 2}
            };

            ///////////////////////////////////////////////////////////////////

            CHECK(CONDITION);

            map = ilist;

            CHECK(map.size() == 5);
            CHECK(map.nth(0)->first == 40); CHECK(map.nth(0)->second == 2);
            CHECK(map.nth(1)->first == 50); CHECK(map.nth(1)->second == 2);
            CHECK(map.nth(2)->first == 60); CHECK(map.nth(2)->second == 2);
            CHECK(map.nth(3)->first == 70); CHECK(map.nth(3)->second == 2);
            CHECK(map.nth(4)->first == 80); CHECK(map.nth(4)->second == 2);
        }
        #undef CONDITION

        #define CONDITION map.size() > ilist.size()
        {
            sfl::static_flat_multimap<xint, xint, 100, std::less<xint>> map;

            map.emplace(10, 1);
            map.emplace(20, 1);
            map.emplace(30, 1);
            map.emplace(40, 1);
            map.emplace(50, 1);

            CHECK(map.size() == 5);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
            CHECK(map.nth(3)->first == 40); CHECK(map.nth(3)->second == 1);
            CHECK(map.nth(4)->first == 50); CHECK(map.nth(4)->second == 1);

            std::initializer_list<std::pair<xint, xint>> ilist
            {
                {60, 2},
                {70, 2},
                {80, 2}
            };

            ///////////////////////////////////////////////////////////////////

            CHECK(CONDITION);

            map = ilist;

            CHECK(map.size() == 3);
            CHECK(map.nth(0)->first == 60); CHECK(map.nth(0)->second == 2);
            CHECK(map.nth(1)->first == 70); CHECK(map.nth(1)->second == 2);
            CHECK(map.nth(2)->first == 80); CHECK(map.nth(2)->second == 2);
        }
        #undef CONDITION
    }

    ///////////////////////////////////////////////////////////////////////////////

    PRINT("Test NON-MEMBER comparison operators");
    {
        sfl::static_flat_multimap<xint, xint, 100, std::less<xint>> map1, map2;

        map1.emplace(10, 1);
        map1.emplace(20, 1);
        map1.emplace(30, 1);

        map2.emplace(10, 1);
        map2.emplace(20, 1);
        map2.emplace(30, 1);
        map2.emplace(40, 1);
        map2.emplace(50, 1);

        CHECK((map1 == map1) == true);
        CHECK((map1 == map2) == false);
        CHECK((map2 == map1) == false);
        CHECK((map2 == map2) == true);

        CHECK((map1 != map1) == false);
        CHECK((map1 != map2) == true);
        CHECK((map2 != map1) == true);
        CHECK((map2 != map2) == false);

        CHECK((map1 < map1) == false);
        CHECK((map1 < map2) == true);
        CHECK((map2 < map1) == false);
        CHECK((map2 < map2) == false);

        CHECK((map1 > map1) == false);
        CHECK((map1 > map2) == false);
        CHECK((map2 > map1) == true);
        CHECK((map2 > map2) == false);

        CHECK((map1 <= map1) == true);
        CHECK((map1 <= map2) == true);
        CHECK((map2 <= map1) == false);
        CHECK((map2 <= map2) == true);

        CHECK((map1 >= map1) == true);
        CHECK((map1 >= map2) == false);
        CHECK((map2 >= map1) == true);
        CHECK((map2 >= map2) == true);
    }

    PRINT("Test NON-MEMBER swap(container&)");
    {
        sfl::static_flat_multimap<xint, xint, 100, std::less<xint>> map1, map2;

        map1.emplace(10, 1);
        map1.emplace(20, 1);
        map1.emplace(30, 1);

        map2.emplace(40, 2);
        map2.emplace(50, 2);
        map2.emplace(60, 2);
        map2.emplace(70, 2);
        map2.emplace(80, 2);

        CHECK(map1.size() == 3);
        CHECK(map1.nth(0)->first == 10); CHECK(map1.nth(0)->second == 1);
        CHECK(map1.nth(1)->first == 20); CHECK(map1.nth(1)->second == 1);
        CHECK(map1.nth(2)->first == 30); CHECK(map1.nth(2)->second == 1);

        CHECK(map2.size() == 5);
        CHECK(map2.nth(0)->first == 40); CHECK(map2.nth(0)->second == 2);
        CHECK(map2.nth(1)->first == 50); CHECK(map2.nth(1)->second == 2);
        CHECK(map2.nth(2)->first == 60); CHECK(map2.nth(2)->second == 2);
        CHECK(map2.nth(3)->first == 70); CHECK(map2.nth(3)->second == 2);
        CHECK(map2.nth(4)->first == 80); CHECK(map2.nth(4)->second == 2);

        ///////////////////////////////////////////////////////////////////////////

        swap(map1, map2);

        CHECK(map1.size() == 5);
        CHECK(map1.nth(0)->first == 40); CHECK(map1.nth(0)->second == 2);
        CHECK(map1.nth(1)->first == 50); CHECK(map1.nth(1)->second == 2);
        CHECK(map1.nth(2)->first == 60); CHECK(map1.nth(2)->second == 2);
        CHECK(map1.nth(3)->first == 70); CHECK(map1.nth(3)->second == 2);
        CHECK(map1.nth(4)->first == 80); CHECK(map1.nth(4)->second == 2);

        CHECK(map2.size() == 3);
        CHECK(map2.nth(0)->first == 10); CHECK(map2.nth(0)->second == 1);
        CHECK(map2.nth(1)->first == 20); CHECK(map2.nth(1)->second == 1);
        CHECK(map2.nth(2)->first == 30); CHECK(map2.nth(2)->second == 1);
    }

    PRINT("Test NON-MEMBER erase_if(container&, Predicate)");
    {
        using container_type =
            sfl::static_flat_multimap<xint, xint, 100, std::less<xint>>;

        using const_reference = typename container_type::const_reference;

        ///////////////////////////////////////////////////////////////////////////

        container_type map;

        map.emplace(10, 1);
        map.emplace(20, 1);
        map.emplace(20, 2);
        map.emplace(20, 3);
        map.emplace(30, 1);

        CHECK(map.size() == 5);
        CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
        CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 3);
        CHECK(map.nth(2)->first == 20); CHECK(map.nth(2)->second == 2);
        CHECK(map.nth(3)->first == 20); CHECK(map.nth(3)->second == 1);
        CHECK(map.nth(4)->first == 30); CHECK(map.nth(4)->second == 1);

        ///////////////////////////////////////////////////////////////////////////

        CHECK(erase_if(map, [](const_reference& value){ return value.first == 20; }) == 3);
        CHECK(map.size() == 2);
        CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
        CHECK(map.nth(1)->first == 30); CHECK(map.nth(1)->second == 1);

        ///////////////////////////////////////////////////////////////////////////

        CHECK(erase_if(map, [](const_reference& value){ return value.first == 20; }) == 0);
        CHECK(map.size() == 2);
        CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
        CHECK(map.nth(1)->first == 30); CHECK(map.nth(1)->second == 1);
    }
}

int main()
{
    test_static_flat_multimap();
}
