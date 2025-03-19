#ifndef TPARAM_ALLOCATOR
#error Please define macro TPARAM_ALLOCATOR
#endif

using sfl::test::xint;
using sfl::test::xint_xint;
using sfl::test::xobj;

PRINT("Test PRIVATE member function insert_exactly_at(const_iterator, Args&&...)");
{
    // N == 0
    {
        // Insert at the end
        {
            sfl::small_flat_multiset< xint_xint,
                                      0,
                                      std::less<xint_xint>,
                                      TPARAM_ALLOCATOR<xint_xint> > set;

            using value_type = xint_xint;

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.end(), value_type(10, 1));
                PRINT("<");

                CHECK(res == set.nth(0));
                CHECK(set.size() == 1);
                CHECK(set.capacity() == 1);
                CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.end(), value_type(20, 1));
                PRINT("<");

                CHECK(res == set.nth(1));
                CHECK(set.size() == 2);
                CHECK(set.capacity() == 2);
                CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.end(), value_type(30, 1));
                PRINT("<");

                CHECK(res == set.nth(2));
                CHECK(set.size() == 3);
                CHECK(set.capacity() == 3);
                CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.end(), value_type(40, 1));
                PRINT("<");

                CHECK(res == set.nth(3));
                CHECK(set.size() == 4);
                CHECK(set.capacity() == 4);
                CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
                CHECK(set.nth(3)->first == 40); CHECK(set.nth(3)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.end(), value_type(50, 1));
                PRINT("<");

                CHECK(res == set.nth(4));
                CHECK(set.size() == 5);
                CHECK(set.capacity() == 6);
                CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
                CHECK(set.nth(3)->first == 40); CHECK(set.nth(3)->second == 1);
                CHECK(set.nth(4)->first == 50); CHECK(set.nth(4)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.end(), value_type(60, 1));
                PRINT("<");

                CHECK(res == set.nth(5));
                CHECK(set.size() == 6);
                CHECK(set.capacity() == 6);
                CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
                CHECK(set.nth(3)->first == 40); CHECK(set.nth(3)->second == 1);
                CHECK(set.nth(4)->first == 50); CHECK(set.nth(4)->second == 1);
                CHECK(set.nth(5)->first == 60); CHECK(set.nth(5)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.end(), value_type(70, 1));
                PRINT("<");

                CHECK(res == set.nth(6));
                CHECK(set.size() == 7);
                CHECK(set.capacity() == 9);
                CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
                CHECK(set.nth(3)->first == 40); CHECK(set.nth(3)->second == 1);
                CHECK(set.nth(4)->first == 50); CHECK(set.nth(4)->second == 1);
                CHECK(set.nth(5)->first == 60); CHECK(set.nth(5)->second == 1);
                CHECK(set.nth(6)->first == 70); CHECK(set.nth(6)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.end(), value_type(80, 1));
                PRINT("<");

                CHECK(res == set.nth(7));
                CHECK(set.size() == 8);
                CHECK(set.capacity() == 9);
                CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
                CHECK(set.nth(3)->first == 40); CHECK(set.nth(3)->second == 1);
                CHECK(set.nth(4)->first == 50); CHECK(set.nth(4)->second == 1);
                CHECK(set.nth(5)->first == 60); CHECK(set.nth(5)->second == 1);
                CHECK(set.nth(6)->first == 70); CHECK(set.nth(6)->second == 1);
                CHECK(set.nth(7)->first == 80); CHECK(set.nth(7)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.end(), value_type(90, 1));
                PRINT("<");

                CHECK(res == set.nth(8));
                CHECK(set.size() == 9);
                CHECK(set.capacity() == 9);
                CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
                CHECK(set.nth(3)->first == 40); CHECK(set.nth(3)->second == 1);
                CHECK(set.nth(4)->first == 50); CHECK(set.nth(4)->second == 1);
                CHECK(set.nth(5)->first == 60); CHECK(set.nth(5)->second == 1);
                CHECK(set.nth(6)->first == 70); CHECK(set.nth(6)->second == 1);
                CHECK(set.nth(7)->first == 80); CHECK(set.nth(7)->second == 1);
                CHECK(set.nth(8)->first == 90); CHECK(set.nth(8)->second == 1);
            }
        }

        // Insert at the begin
        {
            sfl::small_flat_multiset< xint_xint,
                                      0,
                                      std::less<xint_xint>,
                                      TPARAM_ALLOCATOR<xint_xint> > set;

            using value_type = xint_xint;

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.begin(), value_type(90, 1));
                PRINT("<");

                CHECK(res == set.nth(0));
                CHECK(set.size() == 1);
                CHECK(set.capacity() == 1);
                CHECK(set.nth(0)->first == 90); CHECK(set.nth(0)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.begin(), value_type(80, 1));
                PRINT("<");

                CHECK(res == set.nth(0));
                CHECK(set.size() == 2);
                CHECK(set.capacity() == 2);
                CHECK(set.nth(0)->first == 80); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 90); CHECK(set.nth(1)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.begin(), value_type(70, 1));
                PRINT("<");

                CHECK(res == set.nth(0));
                CHECK(set.size() == 3);
                CHECK(set.capacity() == 3);
                CHECK(set.nth(0)->first == 70); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 80); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 90); CHECK(set.nth(2)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.begin(), value_type(60, 1));
                PRINT("<");

                CHECK(res == set.nth(0));
                CHECK(set.size() == 4);
                CHECK(set.capacity() == 4);
                CHECK(set.nth(0)->first == 60); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 70); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 80); CHECK(set.nth(2)->second == 1);
                CHECK(set.nth(3)->first == 90); CHECK(set.nth(3)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.begin(), value_type(50, 1));
                PRINT("<");

                CHECK(res == set.nth(0));
                CHECK(set.size() == 5);
                CHECK(set.capacity() == 6);
                CHECK(set.nth(0)->first == 50); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 60); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 70); CHECK(set.nth(2)->second == 1);
                CHECK(set.nth(3)->first == 80); CHECK(set.nth(3)->second == 1);
                CHECK(set.nth(4)->first == 90); CHECK(set.nth(4)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.begin(), value_type(40, 1));
                PRINT("<");

                CHECK(res == set.nth(0));
                CHECK(set.size() == 6);
                CHECK(set.capacity() == 6);
                CHECK(set.nth(0)->first == 40); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 50); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 60); CHECK(set.nth(2)->second == 1);
                CHECK(set.nth(3)->first == 70); CHECK(set.nth(3)->second == 1);
                CHECK(set.nth(4)->first == 80); CHECK(set.nth(4)->second == 1);
                CHECK(set.nth(5)->first == 90); CHECK(set.nth(5)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.begin(), value_type(30, 1));
                PRINT("<");

                CHECK(res == set.nth(0));
                CHECK(set.size() == 7);
                CHECK(set.capacity() == 9);
                CHECK(set.nth(0)->first == 30); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 40); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 50); CHECK(set.nth(2)->second == 1);
                CHECK(set.nth(3)->first == 60); CHECK(set.nth(3)->second == 1);
                CHECK(set.nth(4)->first == 70); CHECK(set.nth(4)->second == 1);
                CHECK(set.nth(5)->first == 80); CHECK(set.nth(5)->second == 1);
                CHECK(set.nth(6)->first == 90); CHECK(set.nth(6)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.begin(), value_type(20, 1));
                PRINT("<");

                CHECK(res == set.nth(0));
                CHECK(set.size() == 8);
                CHECK(set.capacity() == 9);
                CHECK(set.nth(0)->first == 20); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 30); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 40); CHECK(set.nth(2)->second == 1);
                CHECK(set.nth(3)->first == 50); CHECK(set.nth(3)->second == 1);
                CHECK(set.nth(4)->first == 60); CHECK(set.nth(4)->second == 1);
                CHECK(set.nth(5)->first == 70); CHECK(set.nth(5)->second == 1);
                CHECK(set.nth(6)->first == 80); CHECK(set.nth(6)->second == 1);
                CHECK(set.nth(7)->first == 90); CHECK(set.nth(7)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.begin(), value_type(10, 1));
                PRINT("<");

                CHECK(res == set.nth(0));
                CHECK(set.size() == 9);
                CHECK(set.capacity() == 9);
                CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
                CHECK(set.nth(3)->first == 40); CHECK(set.nth(3)->second == 1);
                CHECK(set.nth(4)->first == 50); CHECK(set.nth(4)->second == 1);
                CHECK(set.nth(5)->first == 60); CHECK(set.nth(5)->second == 1);
                CHECK(set.nth(6)->first == 70); CHECK(set.nth(6)->second == 1);
                CHECK(set.nth(7)->first == 80); CHECK(set.nth(7)->second == 1);
                CHECK(set.nth(8)->first == 90); CHECK(set.nth(8)->second == 1);
            }
        }

        // Insert near the end
        {
            sfl::small_flat_multiset< xint_xint,
                                      0,
                                      std::less<xint_xint>,
                                      TPARAM_ALLOCATOR<xint_xint> > set;

            using value_type = xint_xint;

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.end(), value_type(90, 1));
                PRINT("<");

                CHECK(res == set.nth(0));
                CHECK(set.size() == 1);
                CHECK(set.capacity() == 1);
                CHECK(set.nth(0)->first == 90); CHECK(set.nth(0)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.end() - 1, value_type(10, 1));
                PRINT("<");

                CHECK(res == set.nth(0));
                CHECK(set.size() == 2);
                CHECK(set.capacity() == 2);
                CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 90); CHECK(set.nth(1)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.end() - 1, value_type(20, 1));
                PRINT("<");

                CHECK(res == set.nth(1));
                CHECK(set.size() == 3);
                CHECK(set.capacity() == 3);
                CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 90); CHECK(set.nth(2)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.end() - 1, value_type(30, 1));
                PRINT("<");

                CHECK(res == set.nth(2));
                CHECK(set.size() == 4);
                CHECK(set.capacity() == 4);
                CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
                CHECK(set.nth(3)->first == 90); CHECK(set.nth(3)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.end() - 1, value_type(40, 1));
                PRINT("<");

                CHECK(res == set.nth(3));
                CHECK(set.size() == 5);
                CHECK(set.capacity() == 6);
                CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
                CHECK(set.nth(3)->first == 40); CHECK(set.nth(3)->second == 1);
                CHECK(set.nth(4)->first == 90); CHECK(set.nth(4)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.end() - 1, value_type(50, 1));
                PRINT("<");

                CHECK(res == set.nth(4));
                CHECK(set.size() == 6);
                CHECK(set.capacity() == 6);
                CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
                CHECK(set.nth(3)->first == 40); CHECK(set.nth(3)->second == 1);
                CHECK(set.nth(4)->first == 50); CHECK(set.nth(4)->second == 1);
                CHECK(set.nth(5)->first == 90); CHECK(set.nth(5)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.end() - 1, value_type(60, 1));
                PRINT("<");

                CHECK(res == set.nth(5));
                CHECK(set.size() == 7);
                CHECK(set.capacity() == 9);
                CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
                CHECK(set.nth(3)->first == 40); CHECK(set.nth(3)->second == 1);
                CHECK(set.nth(4)->first == 50); CHECK(set.nth(4)->second == 1);
                CHECK(set.nth(5)->first == 60); CHECK(set.nth(5)->second == 1);
                CHECK(set.nth(6)->first == 90); CHECK(set.nth(6)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.end() - 1, value_type(70, 1));
                PRINT("<");

                CHECK(res == set.nth(6));
                CHECK(set.size() == 8);
                CHECK(set.capacity() == 9);
                CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
                CHECK(set.nth(3)->first == 40); CHECK(set.nth(3)->second == 1);
                CHECK(set.nth(4)->first == 50); CHECK(set.nth(4)->second == 1);
                CHECK(set.nth(5)->first == 60); CHECK(set.nth(5)->second == 1);
                CHECK(set.nth(6)->first == 70); CHECK(set.nth(6)->second == 1);
                CHECK(set.nth(7)->first == 90); CHECK(set.nth(7)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.end() - 1, value_type(80, 1));
                PRINT("<");

                CHECK(res == set.nth(7));
                CHECK(set.size() == 9);
                CHECK(set.capacity() == 9);
                CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
                CHECK(set.nth(3)->first == 40); CHECK(set.nth(3)->second == 1);
                CHECK(set.nth(4)->first == 50); CHECK(set.nth(4)->second == 1);
                CHECK(set.nth(5)->first == 60); CHECK(set.nth(5)->second == 1);
                CHECK(set.nth(6)->first == 70); CHECK(set.nth(6)->second == 1);
                CHECK(set.nth(7)->first == 80); CHECK(set.nth(7)->second == 1);
                CHECK(set.nth(8)->first == 90); CHECK(set.nth(8)->second == 1);
            }
        }

        // Insert near the begin
        {
            sfl::small_flat_multiset< xint_xint,
                                      0,
                                      std::less<xint_xint>,
                                      TPARAM_ALLOCATOR<xint_xint> > set;

            using value_type = xint_xint;

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.end(), value_type(10, 1));
                PRINT("<");

                CHECK(res == set.nth(0));
                CHECK(set.size() == 1);
                CHECK(set.capacity() == 1);
                CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.end(), value_type(90, 1));
                PRINT("<");

                CHECK(res == set.nth(1));
                CHECK(set.size() == 2);
                CHECK(set.capacity() == 2);
                CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 90); CHECK(set.nth(1)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.nth(1), value_type(80, 1));
                PRINT("<");

                CHECK(res == set.nth(1));
                CHECK(set.size() == 3);
                CHECK(set.capacity() == 3);
                CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 80); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 90); CHECK(set.nth(2)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.nth(1), value_type(70, 1));
                PRINT("<");

                CHECK(res == set.nth(1));
                CHECK(set.size() == 4);
                CHECK(set.capacity() == 4);
                CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 70); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 80); CHECK(set.nth(2)->second == 1);
                CHECK(set.nth(3)->first == 90); CHECK(set.nth(3)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.nth(1), value_type(60, 1));
                PRINT("<");

                CHECK(res == set.nth(1));
                CHECK(set.size() == 5);
                CHECK(set.capacity() == 6);
                CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 60); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 70); CHECK(set.nth(2)->second == 1);
                CHECK(set.nth(3)->first == 80); CHECK(set.nth(3)->second == 1);
                CHECK(set.nth(4)->first == 90); CHECK(set.nth(4)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.nth(1), value_type(50, 1));
                PRINT("<");

                CHECK(res == set.nth(1));
                CHECK(set.size() == 6);
                CHECK(set.capacity() == 6);
                CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 50); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 60); CHECK(set.nth(2)->second == 1);
                CHECK(set.nth(3)->first == 70); CHECK(set.nth(3)->second == 1);
                CHECK(set.nth(4)->first == 80); CHECK(set.nth(4)->second == 1);
                CHECK(set.nth(5)->first == 90); CHECK(set.nth(5)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.nth(1), value_type(40, 1));
                PRINT("<");

                CHECK(res == set.nth(1));
                CHECK(set.size() == 7);
                CHECK(set.capacity() == 9);
                CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 40); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 50); CHECK(set.nth(2)->second == 1);
                CHECK(set.nth(3)->first == 60); CHECK(set.nth(3)->second == 1);
                CHECK(set.nth(4)->first == 70); CHECK(set.nth(4)->second == 1);
                CHECK(set.nth(5)->first == 80); CHECK(set.nth(5)->second == 1);
                CHECK(set.nth(6)->first == 90); CHECK(set.nth(6)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.nth(1), value_type(30, 1));
                PRINT("<");

                CHECK(res == set.nth(1));
                CHECK(set.size() == 8);
                CHECK(set.capacity() == 9);
                CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 30); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 40); CHECK(set.nth(2)->second == 1);
                CHECK(set.nth(3)->first == 50); CHECK(set.nth(3)->second == 1);
                CHECK(set.nth(4)->first == 60); CHECK(set.nth(4)->second == 1);
                CHECK(set.nth(5)->first == 70); CHECK(set.nth(5)->second == 1);
                CHECK(set.nth(6)->first == 80); CHECK(set.nth(6)->second == 1);
                CHECK(set.nth(7)->first == 90); CHECK(set.nth(7)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.nth(1), value_type(20, 1));
                PRINT("<");

                CHECK(res == set.nth(1));
                CHECK(set.size() == 9);
                CHECK(set.capacity() == 9);
                CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
                CHECK(set.nth(3)->first == 40); CHECK(set.nth(3)->second == 1);
                CHECK(set.nth(4)->first == 50); CHECK(set.nth(4)->second == 1);
                CHECK(set.nth(5)->first == 60); CHECK(set.nth(5)->second == 1);
                CHECK(set.nth(6)->first == 70); CHECK(set.nth(6)->second == 1);
                CHECK(set.nth(7)->first == 80); CHECK(set.nth(7)->second == 1);
                CHECK(set.nth(8)->first == 90); CHECK(set.nth(8)->second == 1);
            }
        }
    }

    // N == 3
    {
        // Insert at the end
        {
            sfl::small_flat_multiset< xint_xint,
                                      3,
                                      std::less<xint_xint>,
                                      TPARAM_ALLOCATOR<xint_xint> > set;

            using value_type = xint_xint;

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.end(), value_type(10, 1));
                PRINT("<");

                CHECK(res == set.nth(0));
                CHECK(set.size() == 1);
                CHECK(set.capacity() == 3);
                CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.end(), value_type(20, 1));
                PRINT("<");

                CHECK(res == set.nth(1));
                CHECK(set.size() == 2);
                CHECK(set.capacity() == 3);
                CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.end(), value_type(30, 1));
                PRINT("<");

                CHECK(res == set.nth(2));
                CHECK(set.size() == 3);
                CHECK(set.capacity() == 3);
                CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.end(), value_type(40, 1));
                PRINT("<");

                CHECK(res == set.nth(3));
                CHECK(set.size() == 4);
                CHECK(set.capacity() == 4);
                CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
                CHECK(set.nth(3)->first == 40); CHECK(set.nth(3)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.end(), value_type(50, 1));
                PRINT("<");

                CHECK(res == set.nth(4));
                CHECK(set.size() == 5);
                CHECK(set.capacity() == 6);
                CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
                CHECK(set.nth(3)->first == 40); CHECK(set.nth(3)->second == 1);
                CHECK(set.nth(4)->first == 50); CHECK(set.nth(4)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.end(), value_type(60, 1));
                PRINT("<");

                CHECK(res == set.nth(5));
                CHECK(set.size() == 6);
                CHECK(set.capacity() == 6);
                CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
                CHECK(set.nth(3)->first == 40); CHECK(set.nth(3)->second == 1);
                CHECK(set.nth(4)->first == 50); CHECK(set.nth(4)->second == 1);
                CHECK(set.nth(5)->first == 60); CHECK(set.nth(5)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.end(), value_type(70, 1));
                PRINT("<");

                CHECK(res == set.nth(6));
                CHECK(set.size() == 7);
                CHECK(set.capacity() == 9);
                CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
                CHECK(set.nth(3)->first == 40); CHECK(set.nth(3)->second == 1);
                CHECK(set.nth(4)->first == 50); CHECK(set.nth(4)->second == 1);
                CHECK(set.nth(5)->first == 60); CHECK(set.nth(5)->second == 1);
                CHECK(set.nth(6)->first == 70); CHECK(set.nth(6)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.end(), value_type(80, 1));
                PRINT("<");

                CHECK(res == set.nth(7));
                CHECK(set.size() == 8);
                CHECK(set.capacity() == 9);
                CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
                CHECK(set.nth(3)->first == 40); CHECK(set.nth(3)->second == 1);
                CHECK(set.nth(4)->first == 50); CHECK(set.nth(4)->second == 1);
                CHECK(set.nth(5)->first == 60); CHECK(set.nth(5)->second == 1);
                CHECK(set.nth(6)->first == 70); CHECK(set.nth(6)->second == 1);
                CHECK(set.nth(7)->first == 80); CHECK(set.nth(7)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.end(), value_type(90, 1));
                PRINT("<");

                CHECK(res == set.nth(8));
                CHECK(set.size() == 9);
                CHECK(set.capacity() == 9);
                CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
                CHECK(set.nth(3)->first == 40); CHECK(set.nth(3)->second == 1);
                CHECK(set.nth(4)->first == 50); CHECK(set.nth(4)->second == 1);
                CHECK(set.nth(5)->first == 60); CHECK(set.nth(5)->second == 1);
                CHECK(set.nth(6)->first == 70); CHECK(set.nth(6)->second == 1);
                CHECK(set.nth(7)->first == 80); CHECK(set.nth(7)->second == 1);
                CHECK(set.nth(8)->first == 90); CHECK(set.nth(8)->second == 1);
            }
        }

        // Insert at the begin
        {
            sfl::small_flat_multiset< xint_xint,
                                      3,
                                      std::less<xint_xint>,
                                      TPARAM_ALLOCATOR<xint_xint> > set;

            using value_type = xint_xint;

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.begin(), value_type(90, 1));
                PRINT("<");

                CHECK(res == set.nth(0));
                CHECK(set.size() == 1);
                CHECK(set.capacity() == 3);
                CHECK(set.nth(0)->first == 90); CHECK(set.nth(0)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.begin(), value_type(80, 1));
                PRINT("<");

                CHECK(res == set.nth(0));
                CHECK(set.size() == 2);
                CHECK(set.capacity() == 3);
                CHECK(set.nth(0)->first == 80); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 90); CHECK(set.nth(1)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.begin(), value_type(70, 1));
                PRINT("<");

                CHECK(res == set.nth(0));
                CHECK(set.size() == 3);
                CHECK(set.capacity() == 3);
                CHECK(set.nth(0)->first == 70); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 80); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 90); CHECK(set.nth(2)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.begin(), value_type(60, 1));
                PRINT("<");

                CHECK(res == set.nth(0));
                CHECK(set.size() == 4);
                CHECK(set.capacity() == 4);
                CHECK(set.nth(0)->first == 60); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 70); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 80); CHECK(set.nth(2)->second == 1);
                CHECK(set.nth(3)->first == 90); CHECK(set.nth(3)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.begin(), value_type(50, 1));
                PRINT("<");

                CHECK(res == set.nth(0));
                CHECK(set.size() == 5);
                CHECK(set.capacity() == 6);
                CHECK(set.nth(0)->first == 50); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 60); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 70); CHECK(set.nth(2)->second == 1);
                CHECK(set.nth(3)->first == 80); CHECK(set.nth(3)->second == 1);
                CHECK(set.nth(4)->first == 90); CHECK(set.nth(4)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.begin(), value_type(40, 1));
                PRINT("<");

                CHECK(res == set.nth(0));
                CHECK(set.size() == 6);
                CHECK(set.capacity() == 6);
                CHECK(set.nth(0)->first == 40); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 50); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 60); CHECK(set.nth(2)->second == 1);
                CHECK(set.nth(3)->first == 70); CHECK(set.nth(3)->second == 1);
                CHECK(set.nth(4)->first == 80); CHECK(set.nth(4)->second == 1);
                CHECK(set.nth(5)->first == 90); CHECK(set.nth(5)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.begin(), value_type(30, 1));
                PRINT("<");

                CHECK(res == set.nth(0));
                CHECK(set.size() == 7);
                CHECK(set.capacity() == 9);
                CHECK(set.nth(0)->first == 30); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 40); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 50); CHECK(set.nth(2)->second == 1);
                CHECK(set.nth(3)->first == 60); CHECK(set.nth(3)->second == 1);
                CHECK(set.nth(4)->first == 70); CHECK(set.nth(4)->second == 1);
                CHECK(set.nth(5)->first == 80); CHECK(set.nth(5)->second == 1);
                CHECK(set.nth(6)->first == 90); CHECK(set.nth(6)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.begin(), value_type(20, 1));
                PRINT("<");

                CHECK(res == set.nth(0));
                CHECK(set.size() == 8);
                CHECK(set.capacity() == 9);
                CHECK(set.nth(0)->first == 20); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 30); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 40); CHECK(set.nth(2)->second == 1);
                CHECK(set.nth(3)->first == 50); CHECK(set.nth(3)->second == 1);
                CHECK(set.nth(4)->first == 60); CHECK(set.nth(4)->second == 1);
                CHECK(set.nth(5)->first == 70); CHECK(set.nth(5)->second == 1);
                CHECK(set.nth(6)->first == 80); CHECK(set.nth(6)->second == 1);
                CHECK(set.nth(7)->first == 90); CHECK(set.nth(7)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.begin(), value_type(10, 1));
                PRINT("<");

                CHECK(res == set.nth(0));
                CHECK(set.size() == 9);
                CHECK(set.capacity() == 9);
                CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
                CHECK(set.nth(3)->first == 40); CHECK(set.nth(3)->second == 1);
                CHECK(set.nth(4)->first == 50); CHECK(set.nth(4)->second == 1);
                CHECK(set.nth(5)->first == 60); CHECK(set.nth(5)->second == 1);
                CHECK(set.nth(6)->first == 70); CHECK(set.nth(6)->second == 1);
                CHECK(set.nth(7)->first == 80); CHECK(set.nth(7)->second == 1);
                CHECK(set.nth(8)->first == 90); CHECK(set.nth(8)->second == 1);
            }
        }

        // Insert near the end
        {
            sfl::small_flat_multiset< xint_xint,
                                      3,
                                      std::less<xint_xint>,
                                      TPARAM_ALLOCATOR<xint_xint> > set;

            using value_type = xint_xint;

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.end(), value_type(90, 1));
                PRINT("<");

                CHECK(res == set.nth(0));
                CHECK(set.size() == 1);
                CHECK(set.capacity() == 3);
                CHECK(set.nth(0)->first == 90); CHECK(set.nth(0)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.end() - 1, value_type(10, 1));
                PRINT("<");

                CHECK(res == set.nth(0));
                CHECK(set.size() == 2);
                CHECK(set.capacity() == 3);
                CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 90); CHECK(set.nth(1)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.end() - 1, value_type(20, 1));
                PRINT("<");

                CHECK(res == set.nth(1));
                CHECK(set.size() == 3);
                CHECK(set.capacity() == 3);
                CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 90); CHECK(set.nth(2)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.end() - 1, value_type(30, 1));
                PRINT("<");

                CHECK(res == set.nth(2));
                CHECK(set.size() == 4);
                CHECK(set.capacity() == 4);
                CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
                CHECK(set.nth(3)->first == 90); CHECK(set.nth(3)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.end() - 1, value_type(40, 1));
                PRINT("<");

                CHECK(res == set.nth(3));
                CHECK(set.size() == 5);
                CHECK(set.capacity() == 6);
                CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
                CHECK(set.nth(3)->first == 40); CHECK(set.nth(3)->second == 1);
                CHECK(set.nth(4)->first == 90); CHECK(set.nth(4)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.end() - 1, value_type(50, 1));
                PRINT("<");

                CHECK(res == set.nth(4));
                CHECK(set.size() == 6);
                CHECK(set.capacity() == 6);
                CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
                CHECK(set.nth(3)->first == 40); CHECK(set.nth(3)->second == 1);
                CHECK(set.nth(4)->first == 50); CHECK(set.nth(4)->second == 1);
                CHECK(set.nth(5)->first == 90); CHECK(set.nth(5)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.end() - 1, value_type(60, 1));
                PRINT("<");

                CHECK(res == set.nth(5));
                CHECK(set.size() == 7);
                CHECK(set.capacity() == 9);
                CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
                CHECK(set.nth(3)->first == 40); CHECK(set.nth(3)->second == 1);
                CHECK(set.nth(4)->first == 50); CHECK(set.nth(4)->second == 1);
                CHECK(set.nth(5)->first == 60); CHECK(set.nth(5)->second == 1);
                CHECK(set.nth(6)->first == 90); CHECK(set.nth(6)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.end() - 1, value_type(70, 1));
                PRINT("<");

                CHECK(res == set.nth(6));
                CHECK(set.size() == 8);
                CHECK(set.capacity() == 9);
                CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
                CHECK(set.nth(3)->first == 40); CHECK(set.nth(3)->second == 1);
                CHECK(set.nth(4)->first == 50); CHECK(set.nth(4)->second == 1);
                CHECK(set.nth(5)->first == 60); CHECK(set.nth(5)->second == 1);
                CHECK(set.nth(6)->first == 70); CHECK(set.nth(6)->second == 1);
                CHECK(set.nth(7)->first == 90); CHECK(set.nth(7)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.end() - 1, value_type(80, 1));
                PRINT("<");

                CHECK(res == set.nth(7));
                CHECK(set.size() == 9);
                CHECK(set.capacity() == 9);
                CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
                CHECK(set.nth(3)->first == 40); CHECK(set.nth(3)->second == 1);
                CHECK(set.nth(4)->first == 50); CHECK(set.nth(4)->second == 1);
                CHECK(set.nth(5)->first == 60); CHECK(set.nth(5)->second == 1);
                CHECK(set.nth(6)->first == 70); CHECK(set.nth(6)->second == 1);
                CHECK(set.nth(7)->first == 80); CHECK(set.nth(7)->second == 1);
                CHECK(set.nth(8)->first == 90); CHECK(set.nth(8)->second == 1);
            }
        }

        // Insert near the begin
        {
            sfl::small_flat_multiset< xint_xint,
                                      3,
                                      std::less<xint_xint>,
                                      TPARAM_ALLOCATOR<xint_xint> > set;

            using value_type = xint_xint;

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.end(), value_type(10, 1));
                PRINT("<");

                CHECK(res == set.nth(0));
                CHECK(set.size() == 1);
                CHECK(set.capacity() == 3);
                CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.end(), value_type(90, 1));
                PRINT("<");

                CHECK(res == set.nth(1));
                CHECK(set.size() == 2);
                CHECK(set.capacity() == 3);
                CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 90); CHECK(set.nth(1)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.nth(1), value_type(80, 1));
                PRINT("<");

                CHECK(res == set.nth(1));
                CHECK(set.size() == 3);
                CHECK(set.capacity() == 3);
                CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 80); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 90); CHECK(set.nth(2)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.nth(1), value_type(70, 1));
                PRINT("<");

                CHECK(res == set.nth(1));
                CHECK(set.size() == 4);
                CHECK(set.capacity() == 4);
                CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 70); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 80); CHECK(set.nth(2)->second == 1);
                CHECK(set.nth(3)->first == 90); CHECK(set.nth(3)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.nth(1), value_type(60, 1));
                PRINT("<");

                CHECK(res == set.nth(1));
                CHECK(set.size() == 5);
                CHECK(set.capacity() == 6);
                CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 60); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 70); CHECK(set.nth(2)->second == 1);
                CHECK(set.nth(3)->first == 80); CHECK(set.nth(3)->second == 1);
                CHECK(set.nth(4)->first == 90); CHECK(set.nth(4)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.nth(1), value_type(50, 1));
                PRINT("<");

                CHECK(res == set.nth(1));
                CHECK(set.size() == 6);
                CHECK(set.capacity() == 6);
                CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 50); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 60); CHECK(set.nth(2)->second == 1);
                CHECK(set.nth(3)->first == 70); CHECK(set.nth(3)->second == 1);
                CHECK(set.nth(4)->first == 80); CHECK(set.nth(4)->second == 1);
                CHECK(set.nth(5)->first == 90); CHECK(set.nth(5)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.nth(1), value_type(40, 1));
                PRINT("<");

                CHECK(res == set.nth(1));
                CHECK(set.size() == 7);
                CHECK(set.capacity() == 9);
                CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 40); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 50); CHECK(set.nth(2)->second == 1);
                CHECK(set.nth(3)->first == 60); CHECK(set.nth(3)->second == 1);
                CHECK(set.nth(4)->first == 70); CHECK(set.nth(4)->second == 1);
                CHECK(set.nth(5)->first == 80); CHECK(set.nth(5)->second == 1);
                CHECK(set.nth(6)->first == 90); CHECK(set.nth(6)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.nth(1), value_type(30, 1));
                PRINT("<");

                CHECK(res == set.nth(1));
                CHECK(set.size() == 8);
                CHECK(set.capacity() == 9);
                CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 30); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 40); CHECK(set.nth(2)->second == 1);
                CHECK(set.nth(3)->first == 50); CHECK(set.nth(3)->second == 1);
                CHECK(set.nth(4)->first == 60); CHECK(set.nth(4)->second == 1);
                CHECK(set.nth(5)->first == 70); CHECK(set.nth(5)->second == 1);
                CHECK(set.nth(6)->first == 80); CHECK(set.nth(6)->second == 1);
                CHECK(set.nth(7)->first == 90); CHECK(set.nth(7)->second == 1);
            }

            {
                PRINT(">");
                const auto res = set.insert_exactly_at(set.nth(1), value_type(20, 1));
                PRINT("<");

                CHECK(res == set.nth(1));
                CHECK(set.size() == 9);
                CHECK(set.capacity() == 9);
                CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
                CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
                CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
                CHECK(set.nth(3)->first == 40); CHECK(set.nth(3)->second == 1);
                CHECK(set.nth(4)->first == 50); CHECK(set.nth(4)->second == 1);
                CHECK(set.nth(5)->first == 60); CHECK(set.nth(5)->second == 1);
                CHECK(set.nth(6)->first == 70); CHECK(set.nth(6)->second == 1);
                CHECK(set.nth(7)->first == 80); CHECK(set.nth(7)->second == 1);
                CHECK(set.nth(8)->first == 90); CHECK(set.nth(8)->second == 1);
            }
        }
    }
}

PRINT("Test PRIVATE member function is_insert_hint_good(const_iterator, const Value&)");
{
    sfl::small_flat_multiset< xint_xint,
                              100,
                              std::less<xint_xint>,
                              TPARAM_ALLOCATOR<xint_xint> > set;

    using value_type = xint_xint;

    ///////////////////////////////////////////////////////////////////////////

    set.insert_exactly_at(set.end(), value_type(20, 1));
    set.insert_exactly_at(set.end(), value_type(40, 1));
    set.insert_exactly_at(set.end(), value_type(40, 2));
    set.insert_exactly_at(set.end(), value_type(40, 3));
    set.insert_exactly_at(set.end(), value_type(60, 1));

    CHECK(set.size() == 5);
    CHECK(set.nth(0)->first == 20); CHECK(set.nth(0)->second == 1);
    CHECK(set.nth(1)->first == 40); CHECK(set.nth(1)->second == 1);
    CHECK(set.nth(2)->first == 40); CHECK(set.nth(2)->second == 2);
    CHECK(set.nth(3)->first == 40); CHECK(set.nth(3)->second == 3);
    CHECK(set.nth(4)->first == 60); CHECK(set.nth(4)->second == 1);

    ///////////////////////////////////////////////////////////////////////////

    CHECK(set.is_insert_hint_good(set.nth(0), value_type(20, 1)) == true);
    CHECK(set.is_insert_hint_good(set.nth(1), value_type(20, 1)) == true);
    CHECK(set.is_insert_hint_good(set.nth(2), value_type(20, 1)) == false);
    CHECK(set.is_insert_hint_good(set.nth(3), value_type(20, 1)) == false);
    CHECK(set.is_insert_hint_good(set.nth(4), value_type(20, 1)) == false);
    CHECK(set.is_insert_hint_good(set.nth(5), value_type(20, 1)) == false);

    CHECK(set.is_insert_hint_good(set.nth(0), value_type(40, 1)) == false);
    CHECK(set.is_insert_hint_good(set.nth(1), value_type(40, 1)) == true);
    CHECK(set.is_insert_hint_good(set.nth(2), value_type(40, 1)) == true);
    CHECK(set.is_insert_hint_good(set.nth(3), value_type(40, 1)) == true);
    CHECK(set.is_insert_hint_good(set.nth(4), value_type(40, 1)) == true);
    CHECK(set.is_insert_hint_good(set.nth(5), value_type(40, 1)) == false);

    CHECK(set.is_insert_hint_good(set.nth(0), value_type(60, 1)) == false);
    CHECK(set.is_insert_hint_good(set.nth(1), value_type(60, 1)) == false);
    CHECK(set.is_insert_hint_good(set.nth(2), value_type(60, 1)) == false);
    CHECK(set.is_insert_hint_good(set.nth(3), value_type(60, 1)) == false);
    CHECK(set.is_insert_hint_good(set.nth(4), value_type(60, 1)) == true);
    CHECK(set.is_insert_hint_good(set.nth(5), value_type(60, 1)) == true);

    ///////////////////////////////////////////////////////////////////////////

    CHECK(set.is_insert_hint_good(set.nth(0), value_type(10, 1)) == true);
    CHECK(set.is_insert_hint_good(set.nth(1), value_type(10, 1)) == false);
    CHECK(set.is_insert_hint_good(set.nth(2), value_type(10, 1)) == false);
    CHECK(set.is_insert_hint_good(set.nth(3), value_type(10, 1)) == false);
    CHECK(set.is_insert_hint_good(set.nth(4), value_type(10, 1)) == false);
    CHECK(set.is_insert_hint_good(set.nth(5), value_type(10, 1)) == false);

    CHECK(set.is_insert_hint_good(set.nth(0), value_type(30, 1)) == false);
    CHECK(set.is_insert_hint_good(set.nth(1), value_type(30, 1)) == true);
    CHECK(set.is_insert_hint_good(set.nth(2), value_type(30, 1)) == false);
    CHECK(set.is_insert_hint_good(set.nth(3), value_type(30, 1)) == false);
    CHECK(set.is_insert_hint_good(set.nth(4), value_type(30, 1)) == false);
    CHECK(set.is_insert_hint_good(set.nth(5), value_type(30, 1)) == false);

    CHECK(set.is_insert_hint_good(set.nth(0), value_type(50, 1)) == false);
    CHECK(set.is_insert_hint_good(set.nth(1), value_type(50, 1)) == false);
    CHECK(set.is_insert_hint_good(set.nth(2), value_type(50, 1)) == false);
    CHECK(set.is_insert_hint_good(set.nth(3), value_type(50, 1)) == false);
    CHECK(set.is_insert_hint_good(set.nth(4), value_type(50, 1)) == true);
    CHECK(set.is_insert_hint_good(set.nth(5), value_type(50, 1)) == false);

    CHECK(set.is_insert_hint_good(set.nth(0), value_type(70, 1)) == false);
    CHECK(set.is_insert_hint_good(set.nth(1), value_type(70, 1)) == false);
    CHECK(set.is_insert_hint_good(set.nth(2), value_type(70, 1)) == false);
    CHECK(set.is_insert_hint_good(set.nth(3), value_type(70, 1)) == false);
    CHECK(set.is_insert_hint_good(set.nth(4), value_type(70, 1)) == false);
    CHECK(set.is_insert_hint_good(set.nth(5), value_type(70, 1)) == true);
}

///////////////////////////////////////////////////////////////////////////////

PRINT("Test begin, end, cbegin, cend, rbegin, rend, crbegin, crend, nth, index_of");
{
    sfl::small_flat_multiset< xint_xint,
                              100,
                              std::less<xint_xint>,
                              TPARAM_ALLOCATOR<xint_xint> > set;

    using value_type = xint_xint;

    set.insert_exactly_at(set.end(), value_type(20, 1));
    set.insert_exactly_at(set.end(), value_type(40, 1));
    set.insert_exactly_at(set.end(), value_type(60, 1));

    CHECK(set.size() == 3);
    CHECK(set.nth(0)->first == 20); CHECK(set.nth(0)->second == 1);
    CHECK(set.nth(1)->first == 40); CHECK(set.nth(1)->second == 1);
    CHECK(set.nth(2)->first == 60); CHECK(set.nth(2)->second == 1);

    ///////////////////////////////////////////////////////////////////////

    auto it = set.begin();
    CHECK(it->first == 20); CHECK(it->second == 1); ++it;
    CHECK(it->first == 40); CHECK(it->second == 1); ++it;
    CHECK(it->first == 60); CHECK(it->second == 1); ++it;
    CHECK(it == set.end());

    ///////////////////////////////////////////////////////////////////////

    auto cit = set.cbegin();
    CHECK(cit->first == 20); CHECK(cit->second == 1); ++cit;
    CHECK(cit->first == 40); CHECK(cit->second == 1); ++cit;
    CHECK(cit->first == 60); CHECK(cit->second == 1); ++cit;
    CHECK(cit == set.cend());

    ///////////////////////////////////////////////////////////////////////

    auto rit = set.rbegin();
    CHECK(rit->first == 60); CHECK(rit->second == 1); ++rit;
    CHECK(rit->first == 40); CHECK(rit->second == 1); ++rit;
    CHECK(rit->first == 20); CHECK(rit->second == 1); ++rit;
    CHECK(rit == set.rend());

    ///////////////////////////////////////////////////////////////////////

    auto crit = set.crbegin();
    CHECK(crit->first == 60); CHECK(crit->second == 1); ++crit;
    CHECK(crit->first == 40); CHECK(crit->second == 1); ++crit;
    CHECK(crit->first == 20); CHECK(crit->second == 1); ++crit;
    CHECK(crit == set.crend());

    ///////////////////////////////////////////////////////////////////////

    CHECK(set.nth(0)->first == 20); CHECK(set.nth(0)->second == 1);
    CHECK(set.nth(1)->first == 40); CHECK(set.nth(1)->second == 1);
    CHECK(set.nth(2)->first == 60); CHECK(set.nth(2)->second == 1);
    CHECK(set.nth(3) == set.end());

    ///////////////////////////////////////////////////////////////////////

    CHECK(std::next(set.begin(), 0) == set.nth(0));
    CHECK(std::next(set.begin(), 1) == set.nth(1));
    CHECK(std::next(set.begin(), 2) == set.nth(2));
    CHECK(std::next(set.begin(), 3) == set.nth(3));

    ///////////////////////////////////////////////////////////////////////

    CHECK(std::next(set.cbegin(), 0) == set.nth(0));
    CHECK(std::next(set.cbegin(), 1) == set.nth(1));
    CHECK(std::next(set.cbegin(), 2) == set.nth(2));
    CHECK(std::next(set.cbegin(), 3) == set.nth(3));

    ///////////////////////////////////////////////////////////////////////

    CHECK(set.nth(0) < set.nth(1));
    CHECK(set.nth(0) < set.nth(2));
    CHECK(set.nth(0) < set.nth(3));

    CHECK(set.nth(1) < set.nth(2));
    CHECK(set.nth(1) < set.nth(3));

    CHECK(set.nth(2) < set.nth(3));

    ///////////////////////////////////////////////////////////////////////

    CHECK(set.index_of(set.nth(0)) == 0);
    CHECK(set.index_of(set.nth(1)) == 1);
    CHECK(set.index_of(set.nth(2)) == 2);
    CHECK(set.index_of(set.nth(3)) == 3);
}

///////////////////////////////////////////////////////////////////////////////

PRINT("Test static_capacity");
{
    CHECK((sfl::small_flat_multiset< xint_xint,
                                     100,
                                     std::less<xint_xint>,
                                     TPARAM_ALLOCATOR<xint_xint> >::static_capacity == 100));
}

///////////////////////////////////////////////////////////////////////////////

PRINT("Test get_allocator()");
{
    sfl::small_flat_multiset< xint_xint,
                              100,
                              std::less<xint_xint>,
                              TPARAM_ALLOCATOR<xint_xint> > set;

    auto alloc = set.get_allocator();

    (void)alloc;
}

///////////////////////////////////////////////////////////////////////////////

PRINT("Test key_comp()");
{
    {
        sfl::small_flat_multiset< xint,
                                  100,
                                  std::less<xint>,
                                  TPARAM_ALLOCATOR<xint> > set;

        auto key_comp = set.key_comp();

        CHECK(key_comp(10, 10) == false);
        CHECK(key_comp(10, 20) == true);
        CHECK(key_comp(20, 10) == false);
        CHECK(key_comp(20, 20) == false);
    }

    {
        sfl::small_flat_multiset< xobj,
                                  100,
                                  xobj::less,
                                  TPARAM_ALLOCATOR<xobj> > set;

        auto key_comp = set.key_comp();

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
        sfl::small_flat_multiset< xint,
                                  100,
                                  std::less<xint>,
                                  TPARAM_ALLOCATOR<xint> > set;

        auto value_comp = set.value_comp();

        CHECK(value_comp(10, 10) == false);
        CHECK(value_comp(10, 20) == true);
        CHECK(value_comp(20, 10) == false);
        CHECK(value_comp(20, 20) == false);
    }

    {
        sfl::small_flat_multiset< xobj,
                                  100,
                                  xobj::less,
                                  TPARAM_ALLOCATOR<xobj> > set;

        auto value_comp = set.value_comp();

        CHECK(value_comp(xobj(10), 10) == false);
        CHECK(value_comp(xobj(10), 20) == true);
        CHECK(value_comp(xobj(20), 10) == false);
        CHECK(value_comp(xobj(20), 20) == false);
    }
}

///////////////////////////////////////////////////////////////////////////////

PRINT("Test lower_bound, upper_bound, equal_range, find, count, contains");
{
    // xint
    {
        sfl::small_flat_multiset< xint,
                                  100,
                                  std::less<xint>,
                                  TPARAM_ALLOCATOR<xint> > set;

        using value_type = xint;

        set.insert_exactly_at(set.end(), value_type(20));
        set.insert_exactly_at(set.end(), value_type(40));
        set.insert_exactly_at(set.end(), value_type(60));

        CHECK(set.size() == 3);
        CHECK(*set.nth(0) == 20);
        CHECK(*set.nth(1) == 40);
        CHECK(*set.nth(2) == 60);

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.lower_bound(10) == set.nth(0));
        CHECK(set.lower_bound(20) == set.nth(0));
        CHECK(set.lower_bound(30) == set.nth(1));
        CHECK(set.lower_bound(40) == set.nth(1));
        CHECK(set.lower_bound(50) == set.nth(2));
        CHECK(set.lower_bound(60) == set.nth(2));
        CHECK(set.lower_bound(70) == set.nth(3));

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.upper_bound(10) == set.nth(0));
        CHECK(set.upper_bound(20) == set.nth(1));
        CHECK(set.upper_bound(30) == set.nth(1));
        CHECK(set.upper_bound(40) == set.nth(2));
        CHECK(set.upper_bound(50) == set.nth(2));
        CHECK(set.upper_bound(60) == set.nth(3));
        CHECK(set.upper_bound(70) == set.nth(3));

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.equal_range(10) == std::make_pair(set.nth(0), set.nth(0)));
        CHECK(set.equal_range(20) == std::make_pair(set.nth(0), set.nth(1)));
        CHECK(set.equal_range(30) == std::make_pair(set.nth(1), set.nth(1)));
        CHECK(set.equal_range(40) == std::make_pair(set.nth(1), set.nth(2)));
        CHECK(set.equal_range(50) == std::make_pair(set.nth(2), set.nth(2)));
        CHECK(set.equal_range(60) == std::make_pair(set.nth(2), set.nth(3)));
        CHECK(set.equal_range(70) == std::make_pair(set.nth(3), set.nth(3)));

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.find(10) == set.end());
        CHECK(set.find(20) == set.nth(0));
        CHECK(set.find(30) == set.end());
        CHECK(set.find(40) == set.nth(1));
        CHECK(set.find(50) == set.end());
        CHECK(set.find(60) == set.nth(2));
        CHECK(set.find(70) == set.end());

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.count(10) == 0);
        CHECK(set.count(20) == 1);
        CHECK(set.count(30) == 0);
        CHECK(set.count(40) == 1);
        CHECK(set.count(50) == 0);
        CHECK(set.count(60) == 1);
        CHECK(set.count(70) == 0);

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.contains(10) == false);
        CHECK(set.contains(20) == true);
        CHECK(set.contains(30) == false);
        CHECK(set.contains(40) == true);
        CHECK(set.contains(50) == false);
        CHECK(set.contains(60) == true);
        CHECK(set.contains(70) == false);
    }

    // xobj
    {
        sfl::small_flat_multiset< xobj,
                                  100,
                                  xobj::less,
                                  TPARAM_ALLOCATOR<xobj> > set;

        using value_type = xobj;

        set.insert_exactly_at(set.end(), value_type(20));
        set.insert_exactly_at(set.end(), value_type(40));
        set.insert_exactly_at(set.end(), value_type(60));

        CHECK(set.size() == 3);
        CHECK(set.nth(0)->value() == 20);
        CHECK(set.nth(1)->value() == 40);
        CHECK(set.nth(2)->value() == 60);

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.lower_bound(10) == set.nth(0));
        CHECK(set.lower_bound(20) == set.nth(0));
        CHECK(set.lower_bound(30) == set.nth(1));
        CHECK(set.lower_bound(40) == set.nth(1));
        CHECK(set.lower_bound(50) == set.nth(2));
        CHECK(set.lower_bound(60) == set.nth(2));
        CHECK(set.lower_bound(70) == set.nth(3));

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.upper_bound(10) == set.nth(0));
        CHECK(set.upper_bound(20) == set.nth(1));
        CHECK(set.upper_bound(30) == set.nth(1));
        CHECK(set.upper_bound(40) == set.nth(2));
        CHECK(set.upper_bound(50) == set.nth(2));
        CHECK(set.upper_bound(60) == set.nth(3));
        CHECK(set.upper_bound(70) == set.nth(3));

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.equal_range(10) == std::make_pair(set.nth(0), set.nth(0)));
        CHECK(set.equal_range(20) == std::make_pair(set.nth(0), set.nth(1)));
        CHECK(set.equal_range(30) == std::make_pair(set.nth(1), set.nth(1)));
        CHECK(set.equal_range(40) == std::make_pair(set.nth(1), set.nth(2)));
        CHECK(set.equal_range(50) == std::make_pair(set.nth(2), set.nth(2)));
        CHECK(set.equal_range(60) == std::make_pair(set.nth(2), set.nth(3)));
        CHECK(set.equal_range(70) == std::make_pair(set.nth(3), set.nth(3)));

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.find(10) == set.end());
        CHECK(set.find(20) == set.nth(0));
        CHECK(set.find(30) == set.end());
        CHECK(set.find(40) == set.nth(1));
        CHECK(set.find(50) == set.end());
        CHECK(set.find(60) == set.nth(2));
        CHECK(set.find(70) == set.end());

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.count(10) == 0);
        CHECK(set.count(20) == 1);
        CHECK(set.count(30) == 0);
        CHECK(set.count(40) == 1);
        CHECK(set.count(50) == 0);
        CHECK(set.count(60) == 1);
        CHECK(set.count(70) == 0);

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.contains(10) == false);
        CHECK(set.contains(20) == true);
        CHECK(set.contains(30) == false);
        CHECK(set.contains(40) == true);
        CHECK(set.contains(50) == false);
        CHECK(set.contains(60) == true);
        CHECK(set.contains(70) == false);
    }
}

///////////////////////////////////////////////////////////////////////////////

PRINT("Test clear()");
{
    sfl::small_flat_multiset< xint_xint,
                              100,
                              std::less<xint_xint>,
                              TPARAM_ALLOCATOR<xint_xint> > set;

    using value_type = xint_xint;

    CHECK(set.size() == 0);

    set.insert_exactly_at(set.end(), value_type(10, 1));
    set.insert_exactly_at(set.end(), value_type(20, 1));
    set.insert_exactly_at(set.end(), value_type(30, 1));

    CHECK(set.size() == 3);
    CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
    CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
    CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);

    set.clear();

    CHECK(set.size() == 0);

    set.insert_exactly_at(set.end(), value_type(40, 2));
    set.insert_exactly_at(set.end(), value_type(50, 2));
    set.insert_exactly_at(set.end(), value_type(60, 2));

    CHECK(set.size() == 3);
    CHECK(set.nth(0)->first == 40); CHECK(set.nth(0)->second == 2);
    CHECK(set.nth(1)->first == 50); CHECK(set.nth(1)->second == 2);
    CHECK(set.nth(2)->first == 60); CHECK(set.nth(2)->second == 2);

    set.clear();

    CHECK(set.size() == 0);
}

PRINT("Test emplace(Args&&...)");
{
    sfl::small_flat_multiset< xint_xint,
                              100,
                              std::less<xint_xint>,
                              TPARAM_ALLOCATOR<xint_xint> > set;

    {
        CHECK(set.emplace(20, 1) == set.nth(0));
        CHECK(set.emplace(40, 1) == set.nth(1));
        CHECK(set.emplace(60, 1) == set.nth(2));

        CHECK(set.emplace(10, 1) == set.nth(0));
        CHECK(set.emplace(30, 1) == set.nth(2));
        CHECK(set.emplace(50, 1) == set.nth(4));

        CHECK(set.size() == 6);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
        CHECK(set.nth(3)->first == 40); CHECK(set.nth(3)->second == 1);
        CHECK(set.nth(4)->first == 50); CHECK(set.nth(4)->second == 1);
        CHECK(set.nth(5)->first == 60); CHECK(set.nth(5)->second == 1);
    }

    {
        CHECK(set.emplace(20, 2) == set.nth(1));
        CHECK(set.emplace(40, 2) == set.nth(4));
        CHECK(set.emplace(60, 2) == set.nth(7));

        CHECK(set.emplace(10, 2) == set.nth(0));
        CHECK(set.emplace(30, 2) == set.nth(4));
        CHECK(set.emplace(50, 2) == set.nth(8));

        CHECK(set.size() == 12);
        CHECK(set.nth( 0)->first == 10); CHECK(set.nth( 0)->second == 2);
        CHECK(set.nth( 1)->first == 10); CHECK(set.nth( 1)->second == 1);
        CHECK(set.nth( 2)->first == 20); CHECK(set.nth( 2)->second == 2);
        CHECK(set.nth( 3)->first == 20); CHECK(set.nth( 3)->second == 1);
        CHECK(set.nth( 4)->first == 30); CHECK(set.nth( 4)->second == 2);
        CHECK(set.nth( 5)->first == 30); CHECK(set.nth( 5)->second == 1);
        CHECK(set.nth( 6)->first == 40); CHECK(set.nth( 6)->second == 2);
        CHECK(set.nth( 7)->first == 40); CHECK(set.nth( 7)->second == 1);
        CHECK(set.nth( 8)->first == 50); CHECK(set.nth( 8)->second == 2);
        CHECK(set.nth( 9)->first == 50); CHECK(set.nth( 9)->second == 1);
        CHECK(set.nth(10)->first == 60); CHECK(set.nth(10)->second == 2);
        CHECK(set.nth(11)->first == 60); CHECK(set.nth(11)->second == 1);
    }
}

PRINT("Test emplace_hint(const_iterator, Args&&...)");
{
    sfl::small_flat_multiset< xint_xint,
                              100,
                              std::less<xint_xint>,
                              TPARAM_ALLOCATOR<xint_xint> > set;

    {
        CHECK(set.emplace_hint(set.begin(), 20, 1) == set.nth(0));
        CHECK(set.emplace_hint(set.begin(), 40, 1) == set.nth(1));
        CHECK(set.emplace_hint(set.begin(), 60, 1) == set.nth(2));

        CHECK(set.emplace_hint(set.begin(), 10, 1) == set.nth(0));
        CHECK(set.emplace_hint(set.begin(), 30, 1) == set.nth(2));
        CHECK(set.emplace_hint(set.begin(), 50, 1) == set.nth(4));

        CHECK(set.size() == 6);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
        CHECK(set.nth(3)->first == 40); CHECK(set.nth(3)->second == 1);
        CHECK(set.nth(4)->first == 50); CHECK(set.nth(4)->second == 1);
        CHECK(set.nth(5)->first == 60); CHECK(set.nth(5)->second == 1);
    }

    {
        CHECK(set.emplace_hint(set.begin(), 20, 2) == set.nth(1));
        CHECK(set.emplace_hint(set.begin(), 40, 2) == set.nth(4));
        CHECK(set.emplace_hint(set.begin(), 60, 2) == set.nth(7));

        CHECK(set.emplace_hint(set.begin(), 10, 2) == set.nth(0));
        CHECK(set.emplace_hint(set.begin(), 30, 2) == set.nth(4));
        CHECK(set.emplace_hint(set.begin(), 50, 2) == set.nth(8));

        CHECK(set.size() == 12);
        CHECK(set.nth( 0)->first == 10); CHECK(set.nth( 0)->second == 2);
        CHECK(set.nth( 1)->first == 10); CHECK(set.nth( 1)->second == 1);
        CHECK(set.nth( 2)->first == 20); CHECK(set.nth( 2)->second == 2);
        CHECK(set.nth( 3)->first == 20); CHECK(set.nth( 3)->second == 1);
        CHECK(set.nth( 4)->first == 30); CHECK(set.nth( 4)->second == 2);
        CHECK(set.nth( 5)->first == 30); CHECK(set.nth( 5)->second == 1);
        CHECK(set.nth( 6)->first == 40); CHECK(set.nth( 6)->second == 2);
        CHECK(set.nth( 7)->first == 40); CHECK(set.nth( 7)->second == 1);
        CHECK(set.nth( 8)->first == 50); CHECK(set.nth( 8)->second == 2);
        CHECK(set.nth( 9)->first == 50); CHECK(set.nth( 9)->second == 1);
        CHECK(set.nth(10)->first == 60); CHECK(set.nth(10)->second == 2);
        CHECK(set.nth(11)->first == 60); CHECK(set.nth(11)->second == 1);
    }
}

PRINT("Test insert(const value_type&)");
{
    sfl::small_flat_multiset< xint_xint,
                              100,
                              std::less<xint_xint>,
                              TPARAM_ALLOCATOR<xint_xint> > set;

    using value_type = xint_xint;

    {
        value_type value_20_1(20, 1);
        value_type value_40_1(40, 1);
        value_type value_60_1(60, 1);

        value_type value_10_1(10, 1);
        value_type value_30_1(30, 1);
        value_type value_50_1(50, 1);

        CHECK(set.insert(value_20_1) == set.nth(0));
        CHECK(set.insert(value_40_1) == set.nth(1));
        CHECK(set.insert(value_60_1) == set.nth(2));

        CHECK(set.insert(value_10_1) == set.nth(0));
        CHECK(set.insert(value_30_1) == set.nth(2));
        CHECK(set.insert(value_50_1) == set.nth(4));

        CHECK(set.size() == 6);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
        CHECK(set.nth(3)->first == 40); CHECK(set.nth(3)->second == 1);
        CHECK(set.nth(4)->first == 50); CHECK(set.nth(4)->second == 1);
        CHECK(set.nth(5)->first == 60); CHECK(set.nth(5)->second == 1);

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

        CHECK(set.insert(value_20_2) == set.nth(1));
        CHECK(set.insert(value_40_2) == set.nth(4));
        CHECK(set.insert(value_60_2) == set.nth(7));

        CHECK(set.insert(value_10_2) == set.nth(0));
        CHECK(set.insert(value_30_2) == set.nth(4));
        CHECK(set.insert(value_50_2) == set.nth(8));

        CHECK(set.size() == 12);
        CHECK(set.nth( 0)->first == 10); CHECK(set.nth( 0)->second == 2);
        CHECK(set.nth( 1)->first == 10); CHECK(set.nth( 1)->second == 1);
        CHECK(set.nth( 2)->first == 20); CHECK(set.nth( 2)->second == 2);
        CHECK(set.nth( 3)->first == 20); CHECK(set.nth( 3)->second == 1);
        CHECK(set.nth( 4)->first == 30); CHECK(set.nth( 4)->second == 2);
        CHECK(set.nth( 5)->first == 30); CHECK(set.nth( 5)->second == 1);
        CHECK(set.nth( 6)->first == 40); CHECK(set.nth( 6)->second == 2);
        CHECK(set.nth( 7)->first == 40); CHECK(set.nth( 7)->second == 1);
        CHECK(set.nth( 8)->first == 50); CHECK(set.nth( 8)->second == 2);
        CHECK(set.nth( 9)->first == 50); CHECK(set.nth( 9)->second == 1);
        CHECK(set.nth(10)->first == 60); CHECK(set.nth(10)->second == 2);
        CHECK(set.nth(11)->first == 60); CHECK(set.nth(11)->second == 1);

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
    sfl::small_flat_multiset< xint_xint,
                              100,
                              std::less<xint_xint>,
                              TPARAM_ALLOCATOR<xint_xint> > set;

    using value_type = xint_xint;

    {
        value_type value_20_1(20, 1);
        value_type value_40_1(40, 1);
        value_type value_60_1(60, 1);

        value_type value_10_1(10, 1);
        value_type value_30_1(30, 1);
        value_type value_50_1(50, 1);

        CHECK(set.insert(std::move(value_20_1)) == set.nth(0));
        CHECK(set.insert(std::move(value_40_1)) == set.nth(1));
        CHECK(set.insert(std::move(value_60_1)) == set.nth(2));

        CHECK(set.insert(std::move(value_10_1)) == set.nth(0));
        CHECK(set.insert(std::move(value_30_1)) == set.nth(2));
        CHECK(set.insert(std::move(value_50_1)) == set.nth(4));

        CHECK(set.size() == 6);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
        CHECK(set.nth(3)->first == 40); CHECK(set.nth(3)->second == 1);
        CHECK(set.nth(4)->first == 50); CHECK(set.nth(4)->second == 1);
        CHECK(set.nth(5)->first == 60); CHECK(set.nth(5)->second == 1);

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

        CHECK(set.insert(std::move(value_20_2)) == set.nth(1));
        CHECK(set.insert(std::move(value_40_2)) == set.nth(4));
        CHECK(set.insert(std::move(value_60_2)) == set.nth(7));

        CHECK(set.insert(std::move(value_10_2)) == set.nth(0));
        CHECK(set.insert(std::move(value_30_2)) == set.nth(4));
        CHECK(set.insert(std::move(value_50_2)) == set.nth(8));

        CHECK(set.size() == 12);
        CHECK(set.nth( 0)->first == 10); CHECK(set.nth( 0)->second == 2);
        CHECK(set.nth( 1)->first == 10); CHECK(set.nth( 1)->second == 1);
        CHECK(set.nth( 2)->first == 20); CHECK(set.nth( 2)->second == 2);
        CHECK(set.nth( 3)->first == 20); CHECK(set.nth( 3)->second == 1);
        CHECK(set.nth( 4)->first == 30); CHECK(set.nth( 4)->second == 2);
        CHECK(set.nth( 5)->first == 30); CHECK(set.nth( 5)->second == 1);
        CHECK(set.nth( 6)->first == 40); CHECK(set.nth( 6)->second == 2);
        CHECK(set.nth( 7)->first == 40); CHECK(set.nth( 7)->second == 1);
        CHECK(set.nth( 8)->first == 50); CHECK(set.nth( 8)->second == 2);
        CHECK(set.nth( 9)->first == 50); CHECK(set.nth( 9)->second == 1);
        CHECK(set.nth(10)->first == 60); CHECK(set.nth(10)->second == 2);
        CHECK(set.nth(11)->first == 60); CHECK(set.nth(11)->second == 1);

        CHECK(value_20_2.first == -20); CHECK(value_20_2.second == -2);
        CHECK(value_40_2.first == -40); CHECK(value_40_2.second == -2);
        CHECK(value_60_2.first == -60); CHECK(value_60_2.second == -2);

        CHECK(value_10_2.first == -10); CHECK(value_10_2.second == -2);
        CHECK(value_30_2.first == -30); CHECK(value_30_2.second == -2);
        CHECK(value_50_2.first == -50); CHECK(value_50_2.second == -2);
    }
}

PRINT("Test insert(const_iterator, const value_type&)");
{
    sfl::small_flat_multiset< xint_xint,
                              100,
                              std::less<xint_xint>,
                              TPARAM_ALLOCATOR<xint_xint> > set;

    using value_type = xint_xint;

    {
        value_type value_20_1(20, 1);
        value_type value_40_1(40, 1);
        value_type value_60_1(60, 1);

        value_type value_10_1(10, 1);
        value_type value_30_1(30, 1);
        value_type value_50_1(50, 1);

        CHECK(set.insert(set.begin(), value_20_1) == set.nth(0));
        CHECK(set.insert(set.begin(), value_40_1) == set.nth(1));
        CHECK(set.insert(set.begin(), value_60_1) == set.nth(2));

        CHECK(set.insert(set.begin(), value_10_1) == set.nth(0));
        CHECK(set.insert(set.begin(), value_30_1) == set.nth(2));
        CHECK(set.insert(set.begin(), value_50_1) == set.nth(4));

        CHECK(set.size() == 6);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
        CHECK(set.nth(3)->first == 40); CHECK(set.nth(3)->second == 1);
        CHECK(set.nth(4)->first == 50); CHECK(set.nth(4)->second == 1);
        CHECK(set.nth(5)->first == 60); CHECK(set.nth(5)->second == 1);

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

        CHECK(set.insert(set.begin(), value_20_2) == set.nth(1));
        CHECK(set.insert(set.begin(), value_40_2) == set.nth(4));
        CHECK(set.insert(set.begin(), value_60_2) == set.nth(7));

        CHECK(set.insert(set.begin(), value_10_2) == set.nth(0));
        CHECK(set.insert(set.begin(), value_30_2) == set.nth(4));
        CHECK(set.insert(set.begin(), value_50_2) == set.nth(8));

        CHECK(set.size() == 12);
        CHECK(set.nth( 0)->first == 10); CHECK(set.nth( 0)->second == 2);
        CHECK(set.nth( 1)->first == 10); CHECK(set.nth( 1)->second == 1);
        CHECK(set.nth( 2)->first == 20); CHECK(set.nth( 2)->second == 2);
        CHECK(set.nth( 3)->first == 20); CHECK(set.nth( 3)->second == 1);
        CHECK(set.nth( 4)->first == 30); CHECK(set.nth( 4)->second == 2);
        CHECK(set.nth( 5)->first == 30); CHECK(set.nth( 5)->second == 1);
        CHECK(set.nth( 6)->first == 40); CHECK(set.nth( 6)->second == 2);
        CHECK(set.nth( 7)->first == 40); CHECK(set.nth( 7)->second == 1);
        CHECK(set.nth( 8)->first == 50); CHECK(set.nth( 8)->second == 2);
        CHECK(set.nth( 9)->first == 50); CHECK(set.nth( 9)->second == 1);
        CHECK(set.nth(10)->first == 60); CHECK(set.nth(10)->second == 2);
        CHECK(set.nth(11)->first == 60); CHECK(set.nth(11)->second == 1);

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
    sfl::small_flat_multiset< xint_xint,
                              100,
                              std::less<xint_xint>,
                              TPARAM_ALLOCATOR<xint_xint> > set;

    using value_type = xint_xint;

    {
        value_type value_20_1(20, 1);
        value_type value_40_1(40, 1);
        value_type value_60_1(60, 1);

        value_type value_10_1(10, 1);
        value_type value_30_1(30, 1);
        value_type value_50_1(50, 1);

        CHECK(set.insert(set.begin(), std::move(value_20_1)) == set.nth(0));
        CHECK(set.insert(set.begin(), std::move(value_40_1)) == set.nth(1));
        CHECK(set.insert(set.begin(), std::move(value_60_1)) == set.nth(2));

        CHECK(set.insert(set.begin(), std::move(value_10_1)) == set.nth(0));
        CHECK(set.insert(set.begin(), std::move(value_30_1)) == set.nth(2));
        CHECK(set.insert(set.begin(), std::move(value_50_1)) == set.nth(4));

        CHECK(set.size() == 6);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
        CHECK(set.nth(3)->first == 40); CHECK(set.nth(3)->second == 1);
        CHECK(set.nth(4)->first == 50); CHECK(set.nth(4)->second == 1);
        CHECK(set.nth(5)->first == 60); CHECK(set.nth(5)->second == 1);

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

        CHECK(set.insert(set.begin(), std::move(value_20_2)) == set.nth(1));
        CHECK(set.insert(set.begin(), std::move(value_40_2)) == set.nth(4));
        CHECK(set.insert(set.begin(), std::move(value_60_2)) == set.nth(7));

        CHECK(set.insert(set.begin(), std::move(value_10_2)) == set.nth(0));
        CHECK(set.insert(set.begin(), std::move(value_30_2)) == set.nth(4));
        CHECK(set.insert(set.begin(), std::move(value_50_2)) == set.nth(8));

        CHECK(set.size() == 12);
        CHECK(set.nth( 0)->first == 10); CHECK(set.nth( 0)->second == 2);
        CHECK(set.nth( 1)->first == 10); CHECK(set.nth( 1)->second == 1);
        CHECK(set.nth( 2)->first == 20); CHECK(set.nth( 2)->second == 2);
        CHECK(set.nth( 3)->first == 20); CHECK(set.nth( 3)->second == 1);
        CHECK(set.nth( 4)->first == 30); CHECK(set.nth( 4)->second == 2);
        CHECK(set.nth( 5)->first == 30); CHECK(set.nth( 5)->second == 1);
        CHECK(set.nth( 6)->first == 40); CHECK(set.nth( 6)->second == 2);
        CHECK(set.nth( 7)->first == 40); CHECK(set.nth( 7)->second == 1);
        CHECK(set.nth( 8)->first == 50); CHECK(set.nth( 8)->second == 2);
        CHECK(set.nth( 9)->first == 50); CHECK(set.nth( 9)->second == 1);
        CHECK(set.nth(10)->first == 60); CHECK(set.nth(10)->second == 2);
        CHECK(set.nth(11)->first == 60); CHECK(set.nth(11)->second == 1);

        CHECK(value_20_2.first == -20); CHECK(value_20_2.second == -2);
        CHECK(value_40_2.first == -40); CHECK(value_40_2.second == -2);
        CHECK(value_60_2.first == -60); CHECK(value_60_2.second == -2);

        CHECK(value_10_2.first == -10); CHECK(value_10_2.second == -2);
        CHECK(value_30_2.first == -30); CHECK(value_30_2.second == -2);
        CHECK(value_50_2.first == -50); CHECK(value_50_2.second == -2);
    }
}

PRINT("Test insert(InputIt, InputIt)");
{
    sfl::small_flat_multiset< xint_xint,
                              100,
                              std::less<xint_xint>,
                              TPARAM_ALLOCATOR<xint_xint> > set;

    {
        std::vector<xint_xint> data1
        (
            {
                {20, 1},
                {40, 1},
                {60, 1}
            }
        );

        std::vector<xint_xint> data2
        (
            {
                {10, 1},
                {30, 1},
                {50, 1}
            }
        );

        set.insert(data1.begin(), data1.end());
        set.insert(data2.begin(), data2.end());

        CHECK(set.size() == 6);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
        CHECK(set.nth(3)->first == 40); CHECK(set.nth(3)->second == 1);
        CHECK(set.nth(4)->first == 50); CHECK(set.nth(4)->second == 1);
        CHECK(set.nth(5)->first == 60); CHECK(set.nth(5)->second == 1);
    }

    {
        std::vector<xint_xint> data1
        (
            {
                {20, 2},
                {40, 2},
                {60, 2}
            }
        );

        std::vector<xint_xint> data2
        (
            {
                {10, 2},
                {30, 2},
                {50, 2}
            }
        );

        set.insert(data1.begin(), data1.end());
        set.insert(data2.begin(), data2.end());

        CHECK(set.size() == 12);
        CHECK(set.nth( 0)->first == 10); CHECK(set.nth( 0)->second == 2);
        CHECK(set.nth( 1)->first == 10); CHECK(set.nth( 1)->second == 1);
        CHECK(set.nth( 2)->first == 20); CHECK(set.nth( 2)->second == 2);
        CHECK(set.nth( 3)->first == 20); CHECK(set.nth( 3)->second == 1);
        CHECK(set.nth( 4)->first == 30); CHECK(set.nth( 4)->second == 2);
        CHECK(set.nth( 5)->first == 30); CHECK(set.nth( 5)->second == 1);
        CHECK(set.nth( 6)->first == 40); CHECK(set.nth( 6)->second == 2);
        CHECK(set.nth( 7)->first == 40); CHECK(set.nth( 7)->second == 1);
        CHECK(set.nth( 8)->first == 50); CHECK(set.nth( 8)->second == 2);
        CHECK(set.nth( 9)->first == 50); CHECK(set.nth( 9)->second == 1);
        CHECK(set.nth(10)->first == 60); CHECK(set.nth(10)->second == 2);
        CHECK(set.nth(11)->first == 60); CHECK(set.nth(11)->second == 1);
    }
}

PRINT("Test insert(std::initializer_list)");
{
    sfl::small_flat_multiset< xint_xint,
                              100,
                              std::less<xint_xint>,
                              TPARAM_ALLOCATOR<xint_xint> > set;

    {
        std::initializer_list<xint_xint> ilist1
        {
            {20, 1},
            {40, 1},
            {60, 1}
        };

        std::initializer_list<xint_xint> ilist2
        {
            {10, 1},
            {30, 1},
            {50, 1}
        };

        set.insert(ilist1);
        set.insert(ilist2);

        CHECK(set.size() == 6);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
        CHECK(set.nth(3)->first == 40); CHECK(set.nth(3)->second == 1);
        CHECK(set.nth(4)->first == 50); CHECK(set.nth(4)->second == 1);
        CHECK(set.nth(5)->first == 60); CHECK(set.nth(5)->second == 1);
    }

    {
        std::initializer_list<xint_xint> ilist1
        {
            {20, 2},
            {40, 2},
            {60, 2}
        };

        std::initializer_list<xint_xint> ilist2
        {
            {10, 2},
            {30, 2},
            {50, 2}
        };

        set.insert(ilist1);
        set.insert(ilist2);

        CHECK(set.size() == 12);
        CHECK(set.nth( 0)->first == 10); CHECK(set.nth( 0)->second == 2);
        CHECK(set.nth( 1)->first == 10); CHECK(set.nth( 1)->second == 1);
        CHECK(set.nth( 2)->first == 20); CHECK(set.nth( 2)->second == 2);
        CHECK(set.nth( 3)->first == 20); CHECK(set.nth( 3)->second == 1);
        CHECK(set.nth( 4)->first == 30); CHECK(set.nth( 4)->second == 2);
        CHECK(set.nth( 5)->first == 30); CHECK(set.nth( 5)->second == 1);
        CHECK(set.nth( 6)->first == 40); CHECK(set.nth( 6)->second == 2);
        CHECK(set.nth( 7)->first == 40); CHECK(set.nth( 7)->second == 1);
        CHECK(set.nth( 8)->first == 50); CHECK(set.nth( 8)->second == 2);
        CHECK(set.nth( 9)->first == 50); CHECK(set.nth( 9)->second == 1);
        CHECK(set.nth(10)->first == 60); CHECK(set.nth(10)->second == 2);
        CHECK(set.nth(11)->first == 60); CHECK(set.nth(11)->second == 1);
    }
}

PRINT("Test insert_range(Range&&");
{
    // Input iterator (exactly)
    {
        std::istringstream iss("10 20 30 20 20");

        sfl::small_flat_multiset<xint, 32, std::less<xint>, TPARAM_ALLOCATOR<xint>> set;

        #if SFL_CPP_VERSION >= SFL_CPP_20
        set.insert_range(std::views::istream<int>(iss));
        #else
        set.insert_range(sfl::test::istream_view<int>(iss));
        #endif

        CHECK(set.empty() == false);
        CHECK(set.size() == 5);
        CHECK(set.max_size() > 0);
        CHECK(*NTH(set, 0) == 10);
        CHECK(*NTH(set, 1) == 20);
        CHECK(*NTH(set, 2) == 20);
        CHECK(*NTH(set, 3) == 20);
        CHECK(*NTH(set, 4) == 30);
    }

    // Forward iterator
    {
        std::vector<xint> data({10, 20, 30, 20, 20});

        sfl::small_flat_multiset<xint, 32, std::less<xint>, TPARAM_ALLOCATOR<xint>> set;

        #if SFL_CPP_VERSION >= SFL_CPP_20
        set.insert_range(std::views::all(data));
        #else
        set.insert_range(data);
        #endif

        CHECK(set.empty() == false);
        CHECK(set.size() == 5);
        CHECK(set.max_size() > 0);
        CHECK(*NTH(set, 0) == 10);
        CHECK(*NTH(set, 1) == 20);
        CHECK(*NTH(set, 2) == 20);
        CHECK(*NTH(set, 3) == 20);
        CHECK(*NTH(set, 4) == 30);
    }
}

PRINT("Test erase(const_iterator)");
{
    // Erase at the end
    {
        sfl::small_flat_multiset< xint_xint,
                                  100,
                                  std::less<xint_xint>,
                                  TPARAM_ALLOCATOR<xint_xint> > set;

        set.emplace(10, 1);
        set.emplace(20, 1);
        set.emplace(30, 1);
        set.emplace(40, 1);
        set.emplace(50, 1);

        CHECK(set.size() == 5);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
        CHECK(set.nth(3)->first == 40); CHECK(set.nth(3)->second == 1);
        CHECK(set.nth(4)->first == 50); CHECK(set.nth(4)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.erase(set.nth(4)) == set.nth(4));
        CHECK(set.size() == 4);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
        CHECK(set.nth(3)->first == 40); CHECK(set.nth(3)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.erase(set.nth(3)) == set.nth(3));
        CHECK(set.size() == 3);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.erase(set.nth(2)) == set.nth(2));
        CHECK(set.size() == 2);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.erase(set.nth(1)) == set.nth(1));
        CHECK(set.size() == 1);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.erase(set.nth(0)) == set.nth(0));
        CHECK(set.size() == 0);
    }

    // Erase at the begin
    {
        sfl::small_flat_multiset< xint_xint,
                                  100,
                                  std::less<xint_xint>,
                                  TPARAM_ALLOCATOR<xint_xint> > set;

        set.emplace(10, 1);
        set.emplace(20, 1);
        set.emplace(30, 1);
        set.emplace(40, 1);
        set.emplace(50, 1);

        CHECK(set.size() == 5);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
        CHECK(set.nth(3)->first == 40); CHECK(set.nth(3)->second == 1);
        CHECK(set.nth(4)->first == 50); CHECK(set.nth(4)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.erase(set.nth(0)) == set.nth(0));
        CHECK(set.size() == 4);
        CHECK(set.nth(0)->first == 20); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 30); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 40); CHECK(set.nth(2)->second == 1);
        CHECK(set.nth(3)->first == 50); CHECK(set.nth(3)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.erase(set.nth(0)) == set.nth(0));
        CHECK(set.size() == 3);
        CHECK(set.nth(0)->first == 30); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 40); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 50); CHECK(set.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.erase(set.nth(0)) == set.nth(0));
        CHECK(set.size() == 2);
        CHECK(set.nth(0)->first == 40); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 50); CHECK(set.nth(1)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.erase(set.nth(0)) == set.nth(0));
        CHECK(set.size() == 1);
        CHECK(set.nth(0)->first == 50); CHECK(set.nth(0)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.erase(set.nth(0)) == set.nth(0));
        CHECK(set.size() == 0);
    }

    // Erase near the end
    {
        sfl::small_flat_multiset< xint_xint,
                                  100,
                                  std::less<xint_xint>,
                                  TPARAM_ALLOCATOR<xint_xint> > set;

        set.emplace(10, 1);
        set.emplace(20, 1);
        set.emplace(30, 1);
        set.emplace(40, 1);
        set.emplace(50, 1);

        CHECK(set.size() == 5);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
        CHECK(set.nth(3)->first == 40); CHECK(set.nth(3)->second == 1);
        CHECK(set.nth(4)->first == 50); CHECK(set.nth(4)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.erase(set.nth(3)) == set.nth(3));
        CHECK(set.size() == 4);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
        CHECK(set.nth(3)->first == 50); CHECK(set.nth(3)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.erase(set.nth(2)) == set.nth(2));
        CHECK(set.size() == 3);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 50); CHECK(set.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.erase(set.nth(1)) == set.nth(1));
        CHECK(set.size() == 2);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 50); CHECK(set.nth(1)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.erase(set.nth(0)) == set.nth(0));
        CHECK(set.size() == 1);
        CHECK(set.nth(0)->first == 50); CHECK(set.nth(0)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.erase(set.nth(0)) == set.nth(0));
        CHECK(set.size() == 0);
    }

    // Erase near the begin
    {
        sfl::small_flat_multiset< xint_xint,
                                  100,
                                  std::less<xint_xint>,
                                  TPARAM_ALLOCATOR<xint_xint> > set;

        set.emplace(10, 1);
        set.emplace(20, 1);
        set.emplace(30, 1);
        set.emplace(40, 1);
        set.emplace(50, 1);

        CHECK(set.size() == 5);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
        CHECK(set.nth(3)->first == 40); CHECK(set.nth(3)->second == 1);
        CHECK(set.nth(4)->first == 50); CHECK(set.nth(4)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.erase(set.nth(1)) == set.nth(1));
        CHECK(set.size() == 4);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 30); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 40); CHECK(set.nth(2)->second == 1);
        CHECK(set.nth(3)->first == 50); CHECK(set.nth(3)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.erase(set.nth(1)) == set.nth(1));
        CHECK(set.size() == 3);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 40); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 50); CHECK(set.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.erase(set.nth(1)) == set.nth(1));
        CHECK(set.size() == 2);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 50); CHECK(set.nth(1)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.erase(set.nth(1)) == set.nth(1));
        CHECK(set.size() == 1);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.erase(set.nth(0)) == set.nth(0));
        CHECK(set.size() == 0);
    }
}

PRINT("Test erase(const_iterator, const_iterator)");
{
    // Erase at the end
    {
        sfl::small_flat_multiset< xint_xint,
                                  100,
                                  std::less<xint_xint>,
                                  TPARAM_ALLOCATOR<xint_xint> > set;

        set.emplace(10, 1);
        set.emplace(20, 1);
        set.emplace(30, 1);
        set.emplace(40, 1);
        set.emplace(50, 1);
        set.emplace(60, 1);
        set.emplace(70, 1);
        set.emplace(80, 1);
        set.emplace(90, 1);

        CHECK(set.size() == 9);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
        CHECK(set.nth(3)->first == 40); CHECK(set.nth(3)->second == 1);
        CHECK(set.nth(4)->first == 50); CHECK(set.nth(4)->second == 1);
        CHECK(set.nth(5)->first == 60); CHECK(set.nth(5)->second == 1);
        CHECK(set.nth(6)->first == 70); CHECK(set.nth(6)->second == 1);
        CHECK(set.nth(7)->first == 80); CHECK(set.nth(7)->second == 1);
        CHECK(set.nth(8)->first == 90); CHECK(set.nth(8)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.erase(set.nth(6), set.nth(9)) == set.nth(6));
        CHECK(set.size() == 6);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
        CHECK(set.nth(3)->first == 40); CHECK(set.nth(3)->second == 1);
        CHECK(set.nth(4)->first == 50); CHECK(set.nth(4)->second == 1);
        CHECK(set.nth(5)->first == 60); CHECK(set.nth(5)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.erase(set.nth(3), set.nth(6)) == set.nth(3));
        CHECK(set.size() == 3);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.erase(set.nth(0), set.nth(3)) == set.nth(0));
        CHECK(set.size() == 0);
    }

    // Erase at the begin
    {
        sfl::small_flat_multiset< xint_xint,
                                  100,
                                  std::less<xint_xint>,
                                  TPARAM_ALLOCATOR<xint_xint> > set;

        set.emplace(10, 1);
        set.emplace(20, 1);
        set.emplace(30, 1);
        set.emplace(40, 1);
        set.emplace(50, 1);
        set.emplace(60, 1);
        set.emplace(70, 1);
        set.emplace(80, 1);
        set.emplace(90, 1);

        CHECK(set.size() == 9);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
        CHECK(set.nth(3)->first == 40); CHECK(set.nth(3)->second == 1);
        CHECK(set.nth(4)->first == 50); CHECK(set.nth(4)->second == 1);
        CHECK(set.nth(5)->first == 60); CHECK(set.nth(5)->second == 1);
        CHECK(set.nth(6)->first == 70); CHECK(set.nth(6)->second == 1);
        CHECK(set.nth(7)->first == 80); CHECK(set.nth(7)->second == 1);
        CHECK(set.nth(8)->first == 90); CHECK(set.nth(8)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.erase(set.nth(0), set.nth(3)) == set.nth(0));
        CHECK(set.size() == 6);
        CHECK(set.nth(0)->first == 40); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 50); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 60); CHECK(set.nth(2)->second == 1);
        CHECK(set.nth(3)->first == 70); CHECK(set.nth(3)->second == 1);
        CHECK(set.nth(4)->first == 80); CHECK(set.nth(4)->second == 1);
        CHECK(set.nth(5)->first == 90); CHECK(set.nth(5)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.erase(set.nth(0), set.nth(3)) == set.nth(0));
        CHECK(set.size() == 3);
        CHECK(set.nth(0)->first == 70); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 80); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 90); CHECK(set.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.erase(set.nth(0), set.nth(3)) == set.nth(0));
        CHECK(set.size() == 0);
    }

    // Erase near the end
    {
        sfl::small_flat_multiset< xint_xint,
                                  100,
                                  std::less<xint_xint>,
                                  TPARAM_ALLOCATOR<xint_xint> > set;

        set.emplace(10, 1);
        set.emplace(20, 1);
        set.emplace(30, 1);
        set.emplace(40, 1);
        set.emplace(50, 1);
        set.emplace(60, 1);
        set.emplace(70, 1);
        set.emplace(80, 1);
        set.emplace(90, 1);

        CHECK(set.size() == 9);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
        CHECK(set.nth(3)->first == 40); CHECK(set.nth(3)->second == 1);
        CHECK(set.nth(4)->first == 50); CHECK(set.nth(4)->second == 1);
        CHECK(set.nth(5)->first == 60); CHECK(set.nth(5)->second == 1);
        CHECK(set.nth(6)->first == 70); CHECK(set.nth(6)->second == 1);
        CHECK(set.nth(7)->first == 80); CHECK(set.nth(7)->second == 1);
        CHECK(set.nth(8)->first == 90); CHECK(set.nth(8)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.erase(set.nth(5), set.nth(8)) == set.nth(5));
        CHECK(set.size() == 6);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
        CHECK(set.nth(3)->first == 40); CHECK(set.nth(3)->second == 1);
        CHECK(set.nth(4)->first == 50); CHECK(set.nth(4)->second == 1);
        CHECK(set.nth(5)->first == 90); CHECK(set.nth(5)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.erase(set.nth(2), set.nth(5)) == set.nth(2));
        CHECK(set.size() == 3);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 90); CHECK(set.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.erase(set.nth(1), set.nth(2)) == set.nth(1));
        CHECK(set.size() == 2);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 90); CHECK(set.nth(1)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.erase(set.nth(0), set.nth(2)) == set.nth(0));
        CHECK(set.size() == 0);
    }

    // Erase near the begin
    {
        sfl::small_flat_multiset< xint_xint,
                                  100,
                                  std::less<xint_xint>,
                                  TPARAM_ALLOCATOR<xint_xint> > set;

        set.emplace(10, 1);
        set.emplace(20, 1);
        set.emplace(30, 1);
        set.emplace(40, 1);
        set.emplace(50, 1);
        set.emplace(60, 1);
        set.emplace(70, 1);
        set.emplace(80, 1);
        set.emplace(90, 1);

        CHECK(set.size() == 9);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
        CHECK(set.nth(3)->first == 40); CHECK(set.nth(3)->second == 1);
        CHECK(set.nth(4)->first == 50); CHECK(set.nth(4)->second == 1);
        CHECK(set.nth(5)->first == 60); CHECK(set.nth(5)->second == 1);
        CHECK(set.nth(6)->first == 70); CHECK(set.nth(6)->second == 1);
        CHECK(set.nth(7)->first == 80); CHECK(set.nth(7)->second == 1);
        CHECK(set.nth(8)->first == 90); CHECK(set.nth(8)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.erase(set.nth(1), set.nth(4)) == set.nth(1));
        CHECK(set.size() == 6);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 50); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 60); CHECK(set.nth(2)->second == 1);
        CHECK(set.nth(3)->first == 70); CHECK(set.nth(3)->second == 1);
        CHECK(set.nth(4)->first == 80); CHECK(set.nth(4)->second == 1);
        CHECK(set.nth(5)->first == 90); CHECK(set.nth(5)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.erase(set.nth(1), set.nth(4)) == set.nth(1));
        CHECK(set.size() == 3);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 80); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 90); CHECK(set.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.erase(set.nth(1), set.nth(2)) == set.nth(1));
        CHECK(set.size() == 2);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 90); CHECK(set.nth(1)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.erase(set.nth(0), set.nth(2)) == set.nth(0));
        CHECK(set.size() == 0);
    }
}

PRINT("Test erase(const Key&)");
{
    sfl::small_flat_multiset< xint,
                              100,
                              std::less<xint>,
                              TPARAM_ALLOCATOR<xint> > set;

    set.emplace(10);
    set.emplace(20);
    set.emplace(20);
    set.emplace(20);
    set.emplace(30);

    CHECK(set.size() == 5);
    CHECK(*set.nth(0) == 10);
    CHECK(*set.nth(1) == 20);
    CHECK(*set.nth(2) == 20);
    CHECK(*set.nth(3) == 20);
    CHECK(*set.nth(4) == 30);

    CHECK(set.erase(30) == 1);
    CHECK(set.erase(30) == 0);
    CHECK(set.size() == 4);
    CHECK(*set.nth(0) == 10);
    CHECK(*set.nth(1) == 20);
    CHECK(*set.nth(2) == 20);
    CHECK(*set.nth(3) == 20);

    CHECK(set.erase(20) == 3);
    CHECK(set.erase(20) == 0);
    CHECK(set.size() == 1);
    CHECK(*set.nth(0) == 10);

    CHECK(set.erase(10) == 1);
    CHECK(set.erase(10) == 0);
    CHECK(set.size() == 0);
}

PRINT("Test erase(K&&)");
{
    sfl::small_flat_multiset< xobj,
                              100,
                              xobj::less,
                              TPARAM_ALLOCATOR<xobj> > set;

    set.emplace(10);
    set.emplace(20);
    set.emplace(20);
    set.emplace(20);
    set.emplace(30);

    CHECK(set.size() == 5);
    CHECK(set.nth(0)->value() == 10);
    CHECK(set.nth(1)->value() == 20);
    CHECK(set.nth(2)->value() == 20);
    CHECK(set.nth(3)->value() == 20);
    CHECK(set.nth(4)->value() == 30);

    CHECK(set.erase(30) == 1);
    CHECK(set.erase(30) == 0);
    CHECK(set.size() == 4);
    CHECK(set.nth(0)->value() == 10);
    CHECK(set.nth(1)->value() == 20);
    CHECK(set.nth(2)->value() == 20);
    CHECK(set.nth(3)->value() == 20);

    CHECK(set.erase(20) == 3);
    CHECK(set.erase(20) == 0);
    CHECK(set.size() == 1);
    CHECK(set.nth(0)->value() == 10);

    CHECK(set.erase(10) == 1);
    CHECK(set.erase(10) == 0);
    CHECK(set.size() == 0);
}

PRINT("Test swap(container&)");
{
    // Both containers are using internal storage
    {
        sfl::small_flat_multiset< xint_xint,
                                  5,
                                  std::less<xint_xint>,
                                  TPARAM_ALLOCATOR<xint_xint> > set1, set2;

        set1.emplace(10, 1);
        set1.emplace(20, 1);
        set1.emplace(30, 1);

        set2.emplace(40, 2);
        set2.emplace(50, 2);
        set2.emplace(60, 2);
        set2.emplace(70, 2);

        CHECK(set1.size() == 3);
        CHECK(set1.capacity() == 5);
        CHECK(set1.nth(0)->first == 10); CHECK(set1.nth(0)->second == 1);
        CHECK(set1.nth(1)->first == 20); CHECK(set1.nth(1)->second == 1);
        CHECK(set1.nth(2)->first == 30); CHECK(set1.nth(2)->second == 1);

        CHECK(set2.size() == 4);
        CHECK(set2.capacity() == 5);
        CHECK(set2.nth(0)->first == 40); CHECK(set2.nth(0)->second == 2);
        CHECK(set2.nth(1)->first == 50); CHECK(set2.nth(1)->second == 2);
        CHECK(set2.nth(2)->first == 60); CHECK(set2.nth(2)->second == 2);
        CHECK(set2.nth(3)->first == 70); CHECK(set2.nth(3)->second == 2);

        ///////////////////////////////////////////////////////////////////////

        set1.swap(set2);

        CHECK(set1.size() == 4);
        CHECK(set1.capacity() == 5);
        CHECK(set1.nth(0)->first == 40); CHECK(set1.nth(0)->second == 2);
        CHECK(set1.nth(1)->first == 50); CHECK(set1.nth(1)->second == 2);
        CHECK(set1.nth(2)->first == 60); CHECK(set1.nth(2)->second == 2);
        CHECK(set1.nth(3)->first == 70); CHECK(set1.nth(3)->second == 2);

        CHECK(set2.size() == 3);
        CHECK(set2.capacity() == 5);
        CHECK(set2.nth(0)->first == 10); CHECK(set2.nth(0)->second == 1);
        CHECK(set2.nth(1)->first == 20); CHECK(set2.nth(1)->second == 1);
        CHECK(set2.nth(2)->first == 30); CHECK(set2.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        set1.swap(set2);

        CHECK(set1.size() == 3);
        CHECK(set1.capacity() == 5);
        CHECK(set1.nth(0)->first == 10); CHECK(set1.nth(0)->second == 1);
        CHECK(set1.nth(1)->first == 20); CHECK(set1.nth(1)->second == 1);
        CHECK(set1.nth(2)->first == 30); CHECK(set1.nth(2)->second == 1);

        CHECK(set2.size() == 4);
        CHECK(set2.capacity() == 5);
        CHECK(set2.nth(0)->first == 40); CHECK(set2.nth(0)->second == 2);
        CHECK(set2.nth(1)->first == 50); CHECK(set2.nth(1)->second == 2);
        CHECK(set2.nth(2)->first == 60); CHECK(set2.nth(2)->second == 2);
        CHECK(set2.nth(3)->first == 70); CHECK(set2.nth(3)->second == 2);
    }

    // One container is using internal storage and another is using external
    {
        sfl::small_flat_multiset< xint_xint,
                                  5,
                                  std::less<xint_xint>,
                                  TPARAM_ALLOCATOR<xint_xint> > set1, set2;

        set1.emplace(10, 1);
        set1.emplace(20, 1);
        set1.emplace(30, 1);

        set2.emplace(40, 2);
        set2.emplace(50, 2);
        set2.emplace(60, 2);
        set2.emplace(70, 2);
        set2.emplace(80, 2);
        set2.emplace(90, 2);

        CHECK(set1.size() == 3);
        CHECK(set1.capacity() == 5);
        CHECK(set1.nth(0)->first == 10); CHECK(set1.nth(0)->second == 1);
        CHECK(set1.nth(1)->first == 20); CHECK(set1.nth(1)->second == 1);
        CHECK(set1.nth(2)->first == 30); CHECK(set1.nth(2)->second == 1);

        CHECK(set2.size() == 6);
        CHECK(set2.capacity() == 7);
        CHECK(set2.nth(0)->first == 40); CHECK(set2.nth(0)->second == 2);
        CHECK(set2.nth(1)->first == 50); CHECK(set2.nth(1)->second == 2);
        CHECK(set2.nth(2)->first == 60); CHECK(set2.nth(2)->second == 2);
        CHECK(set2.nth(3)->first == 70); CHECK(set2.nth(3)->second == 2);
        CHECK(set2.nth(4)->first == 80); CHECK(set2.nth(4)->second == 2);
        CHECK(set2.nth(5)->first == 90); CHECK(set2.nth(5)->second == 2);

        ///////////////////////////////////////////////////////////////////////

        set1.swap(set2);

        CHECK(set1.size() == 6);
        CHECK(set1.capacity() == 7);
        CHECK(set1.nth(0)->first == 40); CHECK(set1.nth(0)->second == 2);
        CHECK(set1.nth(1)->first == 50); CHECK(set1.nth(1)->second == 2);
        CHECK(set1.nth(2)->first == 60); CHECK(set1.nth(2)->second == 2);
        CHECK(set1.nth(3)->first == 70); CHECK(set1.nth(3)->second == 2);
        CHECK(set1.nth(4)->first == 80); CHECK(set1.nth(4)->second == 2);
        CHECK(set1.nth(5)->first == 90); CHECK(set1.nth(5)->second == 2);

        CHECK(set2.size() == 3);
        CHECK(set2.capacity() == 5);
        CHECK(set2.nth(0)->first == 10); CHECK(set2.nth(0)->second == 1);
        CHECK(set2.nth(1)->first == 20); CHECK(set2.nth(1)->second == 1);
        CHECK(set2.nth(2)->first == 30); CHECK(set2.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        set1.swap(set2);

        CHECK(set1.size() == 3);
        CHECK(set1.capacity() == 5);
        CHECK(set1.nth(0)->first == 10); CHECK(set1.nth(0)->second == 1);
        CHECK(set1.nth(1)->first == 20); CHECK(set1.nth(1)->second == 1);
        CHECK(set1.nth(2)->first == 30); CHECK(set1.nth(2)->second == 1);

        CHECK(set2.size() == 6);
        CHECK(set2.capacity() == 7);
        CHECK(set2.nth(0)->first == 40); CHECK(set2.nth(0)->second == 2);
        CHECK(set2.nth(1)->first == 50); CHECK(set2.nth(1)->second == 2);
        CHECK(set2.nth(2)->first == 60); CHECK(set2.nth(2)->second == 2);
        CHECK(set2.nth(3)->first == 70); CHECK(set2.nth(3)->second == 2);
        CHECK(set2.nth(4)->first == 80); CHECK(set2.nth(4)->second == 2);
        CHECK(set2.nth(5)->first == 90); CHECK(set2.nth(5)->second == 2);
    }

    // Both containers are using external storage
    {
        sfl::small_flat_multiset< xint_xint,
                                  0,
                                  std::less<xint_xint>,
                                  TPARAM_ALLOCATOR<xint_xint> > set1, set2;

        set1.emplace(10, 1);
        set1.emplace(20, 1);
        set1.emplace(30, 1);

        set2.emplace(40, 2);
        set2.emplace(50, 2);
        set2.emplace(60, 2);
        set2.emplace(70, 2);

        CHECK(set1.size() == 3);
        CHECK(set1.capacity() == 3);
        CHECK(set1.nth(0)->first == 10); CHECK(set1.nth(0)->second == 1);
        CHECK(set1.nth(1)->first == 20); CHECK(set1.nth(1)->second == 1);
        CHECK(set1.nth(2)->first == 30); CHECK(set1.nth(2)->second == 1);

        CHECK(set2.size() == 4);
        CHECK(set2.capacity() == 4);
        CHECK(set2.nth(0)->first == 40); CHECK(set2.nth(0)->second == 2);
        CHECK(set2.nth(1)->first == 50); CHECK(set2.nth(1)->second == 2);
        CHECK(set2.nth(2)->first == 60); CHECK(set2.nth(2)->second == 2);
        CHECK(set2.nth(3)->first == 70); CHECK(set2.nth(3)->second == 2);

        ///////////////////////////////////////////////////////////////////////

        set1.swap(set2);

        CHECK(set1.size() == 4);
        CHECK(set1.capacity() == 4);
        CHECK(set1.nth(0)->first == 40); CHECK(set1.nth(0)->second == 2);
        CHECK(set1.nth(1)->first == 50); CHECK(set1.nth(1)->second == 2);
        CHECK(set1.nth(2)->first == 60); CHECK(set1.nth(2)->second == 2);
        CHECK(set1.nth(3)->first == 70); CHECK(set1.nth(3)->second == 2);

        CHECK(set2.size() == 3);
        CHECK(set2.capacity() == 3);
        CHECK(set2.nth(0)->first == 10); CHECK(set2.nth(0)->second == 1);
        CHECK(set2.nth(1)->first == 20); CHECK(set2.nth(1)->second == 1);
        CHECK(set2.nth(2)->first == 30); CHECK(set2.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        set1.swap(set2);

        CHECK(set1.size() == 3);
        CHECK(set1.capacity() == 3);
        CHECK(set1.nth(0)->first == 10); CHECK(set1.nth(0)->second == 1);
        CHECK(set1.nth(1)->first == 20); CHECK(set1.nth(1)->second == 1);
        CHECK(set1.nth(2)->first == 30); CHECK(set1.nth(2)->second == 1);

        CHECK(set2.size() == 4);
        CHECK(set2.capacity() == 4);
        CHECK(set2.nth(0)->first == 40); CHECK(set2.nth(0)->second == 2);
        CHECK(set2.nth(1)->first == 50); CHECK(set2.nth(1)->second == 2);
        CHECK(set2.nth(2)->first == 60); CHECK(set2.nth(2)->second == 2);
        CHECK(set2.nth(3)->first == 70); CHECK(set2.nth(3)->second == 2);
    }
}

///////////////////////////////////////////////////////////////////////////////

PRINT("Test data()");
{
    sfl::small_flat_multiset< xint_xint,
                              100,
                              std::less<xint_xint>,
                              TPARAM_ALLOCATOR<xint_xint> > set;

    set.emplace(10, 1);
    set.emplace(20, 1);
    set.emplace(30, 1);

    CHECK(set.size() == 3);
    CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
    CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
    CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);

    ///////////////////////////////////////////////////////////////////////////

    auto data = set.data();
    CHECK(data->first == 10); CHECK(data->second == 1); ++data;
    CHECK(data->first == 20); CHECK(data->second == 1); ++data;
    CHECK(data->first == 30); CHECK(data->second == 1); ++data;
}

///////////////////////////////////////////////////////////////////////////////

PRINT("Test reserve(size_type) and shrink_to_fit()");
{
    // N = 0
    {
        sfl::small_flat_multiset< xint_xint,
                                  0,
                                  std::less<xint_xint>,
                                  TPARAM_ALLOCATOR<xint_xint> > set;

        set.emplace(10, 1);
        set.emplace(20, 1);
        set.emplace(30, 1);
        set.emplace(40, 1);
        set.erase(set.end() - 1);

        CHECK(set.size() == 3);
        CHECK(set.capacity() == 4);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        set.shrink_to_fit();

        CHECK(set.size() == 3);
        CHECK(set.capacity() == 3);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        set.reserve(999);

        CHECK(set.size() == 3);
        CHECK(set.capacity() == 999);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        set.shrink_to_fit();

        CHECK(set.size() == 3);
        CHECK(set.capacity() == 3);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
    }

    // N = 100
    {
        sfl::small_flat_multiset< xint_xint,
                                  100,
                                  std::less<xint_xint>,
                                  TPARAM_ALLOCATOR<xint_xint> > set;

        set.emplace(10, 1);
        set.emplace(20, 1);
        set.emplace(30, 1);

        CHECK(set.size() == 3);
        CHECK(set.capacity() == 100);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        set.shrink_to_fit();

        CHECK(set.size() == 3);
        CHECK(set.capacity() == 100);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        set.reserve(999);

        CHECK(set.size() == 3);
        CHECK(set.capacity() == 999);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        set.shrink_to_fit();

        CHECK(set.size() == 3);
        CHECK(set.capacity() == 100);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
    }
}

///////////////////////////////////////////////////////////////////////////////

PRINT("Test container()");
{
    sfl::small_flat_multiset< xint_xint,
                              100,
                              std::less<xint_xint>,
                              TPARAM_ALLOCATOR<xint_xint> > set;

    CHECK(set.size() == 0);
    CHECK(set.capacity() == 100);
    CHECK(set.available() == 100);
}

PRINT("Test container(const Compare&)");
{
    std::less<xint_xint> comp;

    sfl::small_flat_multiset< xint_xint,
                              100,
                              std::less<xint_xint>,
                              TPARAM_ALLOCATOR<xint_xint> > set(comp);

    CHECK(set.size() == 0);
    CHECK(set.capacity() == 100);
    CHECK(set.available() == 100);
}

PRINT("Test container(const Allocator&)");
{
    TPARAM_ALLOCATOR<xint_xint> alloc;

    sfl::small_flat_multiset< xint_xint,
                              100,
                              std::less<xint_xint>,
                              TPARAM_ALLOCATOR<xint_xint> > set(alloc);

    CHECK(set.size() == 0);
    CHECK(set.capacity() == 100);
    CHECK(set.available() == 100);
}

PRINT("Test container(const Compare&, const Allocator&)");
{
    std::less<xint_xint> comp;

    TPARAM_ALLOCATOR<xint_xint> alloc;

    sfl::small_flat_multiset< xint_xint,
                              100,
                              std::less<xint_xint>,
                              TPARAM_ALLOCATOR<xint_xint> > set(comp, alloc);

    CHECK(set.size() == 0);
    CHECK(set.capacity() == 100);
    CHECK(set.available() == 100);
}

PRINT("Test container(InputIt, InputIt)");
{
    std::vector<xint_xint> data
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

    sfl::small_flat_multiset< xint_xint,
                              100,
                              std::less<xint_xint>,
                              TPARAM_ALLOCATOR<xint_xint> > set(data.begin(), data.end());

    CHECK(set.size() == 12);
    CHECK(set.nth( 0)->first == 10); CHECK(set.nth( 0)->second == 2);
    CHECK(set.nth( 1)->first == 10); CHECK(set.nth( 1)->second == 1);
    CHECK(set.nth( 2)->first == 20); CHECK(set.nth( 2)->second == 2);
    CHECK(set.nth( 3)->first == 20); CHECK(set.nth( 3)->second == 1);
    CHECK(set.nth( 4)->first == 30); CHECK(set.nth( 4)->second == 2);
    CHECK(set.nth( 5)->first == 30); CHECK(set.nth( 5)->second == 1);
    CHECK(set.nth( 6)->first == 40); CHECK(set.nth( 6)->second == 2);
    CHECK(set.nth( 7)->first == 40); CHECK(set.nth( 7)->second == 1);
    CHECK(set.nth( 8)->first == 50); CHECK(set.nth( 8)->second == 2);
    CHECK(set.nth( 9)->first == 50); CHECK(set.nth( 9)->second == 1);
    CHECK(set.nth(10)->first == 60); CHECK(set.nth(10)->second == 2);
    CHECK(set.nth(11)->first == 60); CHECK(set.nth(11)->second == 1);
}

PRINT("Test container(InputIt, InputIt, const Compare&)");
{
    std::vector<xint_xint> data
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

    std::less<xint_xint> comp;

    sfl::small_flat_multiset< xint_xint,
                              100,
                              std::less<xint_xint>,
                              TPARAM_ALLOCATOR<xint_xint> > set(data.begin(), data.end(), comp);

    CHECK(set.size() == 12);
    CHECK(set.nth( 0)->first == 10); CHECK(set.nth( 0)->second == 2);
    CHECK(set.nth( 1)->first == 10); CHECK(set.nth( 1)->second == 1);
    CHECK(set.nth( 2)->first == 20); CHECK(set.nth( 2)->second == 2);
    CHECK(set.nth( 3)->first == 20); CHECK(set.nth( 3)->second == 1);
    CHECK(set.nth( 4)->first == 30); CHECK(set.nth( 4)->second == 2);
    CHECK(set.nth( 5)->first == 30); CHECK(set.nth( 5)->second == 1);
    CHECK(set.nth( 6)->first == 40); CHECK(set.nth( 6)->second == 2);
    CHECK(set.nth( 7)->first == 40); CHECK(set.nth( 7)->second == 1);
    CHECK(set.nth( 8)->first == 50); CHECK(set.nth( 8)->second == 2);
    CHECK(set.nth( 9)->first == 50); CHECK(set.nth( 9)->second == 1);
    CHECK(set.nth(10)->first == 60); CHECK(set.nth(10)->second == 2);
    CHECK(set.nth(11)->first == 60); CHECK(set.nth(11)->second == 1);
}

PRINT("Test container(InputIt, InputIt, const Allocator&)");
{
    std::vector<xint_xint> data
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

    TPARAM_ALLOCATOR<xint_xint> alloc;

    sfl::small_flat_multiset< xint_xint,
                              100,
                              std::less<xint_xint>,
                              TPARAM_ALLOCATOR<xint_xint> > set(data.begin(), data.end(), alloc);

    CHECK(set.size() == 12);
    CHECK(set.nth( 0)->first == 10); CHECK(set.nth( 0)->second == 2);
    CHECK(set.nth( 1)->first == 10); CHECK(set.nth( 1)->second == 1);
    CHECK(set.nth( 2)->first == 20); CHECK(set.nth( 2)->second == 2);
    CHECK(set.nth( 3)->first == 20); CHECK(set.nth( 3)->second == 1);
    CHECK(set.nth( 4)->first == 30); CHECK(set.nth( 4)->second == 2);
    CHECK(set.nth( 5)->first == 30); CHECK(set.nth( 5)->second == 1);
    CHECK(set.nth( 6)->first == 40); CHECK(set.nth( 6)->second == 2);
    CHECK(set.nth( 7)->first == 40); CHECK(set.nth( 7)->second == 1);
    CHECK(set.nth( 8)->first == 50); CHECK(set.nth( 8)->second == 2);
    CHECK(set.nth( 9)->first == 50); CHECK(set.nth( 9)->second == 1);
    CHECK(set.nth(10)->first == 60); CHECK(set.nth(10)->second == 2);
    CHECK(set.nth(11)->first == 60); CHECK(set.nth(11)->second == 1);
}

PRINT("Test container(InputIt, InputIt, const Compare&, const Allocator&)");
{
    std::vector<xint_xint> data
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

    std::less<xint_xint> comp;

    TPARAM_ALLOCATOR<xint_xint> alloc;

    sfl::small_flat_multiset< xint_xint,
                              100,
                              std::less<xint_xint>,
                              TPARAM_ALLOCATOR<xint_xint> > set(data.begin(), data.end(), comp, alloc);

    CHECK(set.size() == 12);
    CHECK(set.nth( 0)->first == 10); CHECK(set.nth( 0)->second == 2);
    CHECK(set.nth( 1)->first == 10); CHECK(set.nth( 1)->second == 1);
    CHECK(set.nth( 2)->first == 20); CHECK(set.nth( 2)->second == 2);
    CHECK(set.nth( 3)->first == 20); CHECK(set.nth( 3)->second == 1);
    CHECK(set.nth( 4)->first == 30); CHECK(set.nth( 4)->second == 2);
    CHECK(set.nth( 5)->first == 30); CHECK(set.nth( 5)->second == 1);
    CHECK(set.nth( 6)->first == 40); CHECK(set.nth( 6)->second == 2);
    CHECK(set.nth( 7)->first == 40); CHECK(set.nth( 7)->second == 1);
    CHECK(set.nth( 8)->first == 50); CHECK(set.nth( 8)->second == 2);
    CHECK(set.nth( 9)->first == 50); CHECK(set.nth( 9)->second == 1);
    CHECK(set.nth(10)->first == 60); CHECK(set.nth(10)->second == 2);
    CHECK(set.nth(11)->first == 60); CHECK(set.nth(11)->second == 1);
}

PRINT("Test container(std::initializer_list)");
{
    std::initializer_list<xint_xint> ilist
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

    sfl::small_flat_multiset< xint_xint,
                              100,
                              std::less<xint_xint>,
                              TPARAM_ALLOCATOR<xint_xint> > set(ilist);

    CHECK(set.size() == 12);
    CHECK(set.nth( 0)->first == 10); CHECK(set.nth( 0)->second == 2);
    CHECK(set.nth( 1)->first == 10); CHECK(set.nth( 1)->second == 1);
    CHECK(set.nth( 2)->first == 20); CHECK(set.nth( 2)->second == 2);
    CHECK(set.nth( 3)->first == 20); CHECK(set.nth( 3)->second == 1);
    CHECK(set.nth( 4)->first == 30); CHECK(set.nth( 4)->second == 2);
    CHECK(set.nth( 5)->first == 30); CHECK(set.nth( 5)->second == 1);
    CHECK(set.nth( 6)->first == 40); CHECK(set.nth( 6)->second == 2);
    CHECK(set.nth( 7)->first == 40); CHECK(set.nth( 7)->second == 1);
    CHECK(set.nth( 8)->first == 50); CHECK(set.nth( 8)->second == 2);
    CHECK(set.nth( 9)->first == 50); CHECK(set.nth( 9)->second == 1);
    CHECK(set.nth(10)->first == 60); CHECK(set.nth(10)->second == 2);
    CHECK(set.nth(11)->first == 60); CHECK(set.nth(11)->second == 1);
}

PRINT("Test container(std::initializer_list, const Compare&)");
{
    std::initializer_list<xint_xint> ilist
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

    std::less<xint_xint> comp;

    sfl::small_flat_multiset< xint_xint,
                              100,
                              std::less<xint_xint>,
                              TPARAM_ALLOCATOR<xint_xint> > set(ilist, comp);

    CHECK(set.size() == 12);
    CHECK(set.nth( 0)->first == 10); CHECK(set.nth( 0)->second == 2);
    CHECK(set.nth( 1)->first == 10); CHECK(set.nth( 1)->second == 1);
    CHECK(set.nth( 2)->first == 20); CHECK(set.nth( 2)->second == 2);
    CHECK(set.nth( 3)->first == 20); CHECK(set.nth( 3)->second == 1);
    CHECK(set.nth( 4)->first == 30); CHECK(set.nth( 4)->second == 2);
    CHECK(set.nth( 5)->first == 30); CHECK(set.nth( 5)->second == 1);
    CHECK(set.nth( 6)->first == 40); CHECK(set.nth( 6)->second == 2);
    CHECK(set.nth( 7)->first == 40); CHECK(set.nth( 7)->second == 1);
    CHECK(set.nth( 8)->first == 50); CHECK(set.nth( 8)->second == 2);
    CHECK(set.nth( 9)->first == 50); CHECK(set.nth( 9)->second == 1);
    CHECK(set.nth(10)->first == 60); CHECK(set.nth(10)->second == 2);
    CHECK(set.nth(11)->first == 60); CHECK(set.nth(11)->second == 1);
}

PRINT("Test container(std::initializer_list, const Allocator&)");
{
    std::initializer_list<xint_xint> ilist
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

    TPARAM_ALLOCATOR<xint_xint> alloc;

    sfl::small_flat_multiset< xint_xint,
                              100,
                              std::less<xint_xint>,
                              TPARAM_ALLOCATOR<xint_xint> > set(ilist, alloc);

    CHECK(set.size() == 12);
    CHECK(set.nth( 0)->first == 10); CHECK(set.nth( 0)->second == 2);
    CHECK(set.nth( 1)->first == 10); CHECK(set.nth( 1)->second == 1);
    CHECK(set.nth( 2)->first == 20); CHECK(set.nth( 2)->second == 2);
    CHECK(set.nth( 3)->first == 20); CHECK(set.nth( 3)->second == 1);
    CHECK(set.nth( 4)->first == 30); CHECK(set.nth( 4)->second == 2);
    CHECK(set.nth( 5)->first == 30); CHECK(set.nth( 5)->second == 1);
    CHECK(set.nth( 6)->first == 40); CHECK(set.nth( 6)->second == 2);
    CHECK(set.nth( 7)->first == 40); CHECK(set.nth( 7)->second == 1);
    CHECK(set.nth( 8)->first == 50); CHECK(set.nth( 8)->second == 2);
    CHECK(set.nth( 9)->first == 50); CHECK(set.nth( 9)->second == 1);
    CHECK(set.nth(10)->first == 60); CHECK(set.nth(10)->second == 2);
    CHECK(set.nth(11)->first == 60); CHECK(set.nth(11)->second == 1);
}

PRINT("Test container(std::initializer_list, const Compare&, const Allocator&)");
{
    std::initializer_list<xint_xint> ilist
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

    std::less<xint_xint> comp;

    TPARAM_ALLOCATOR<xint_xint> alloc;

    sfl::small_flat_multiset< xint_xint,
                              100,
                              std::less<xint_xint>,
                              TPARAM_ALLOCATOR<xint_xint> > set(ilist, comp, alloc);

    CHECK(set.size() == 12);
    CHECK(set.nth( 0)->first == 10); CHECK(set.nth( 0)->second == 2);
    CHECK(set.nth( 1)->first == 10); CHECK(set.nth( 1)->second == 1);
    CHECK(set.nth( 2)->first == 20); CHECK(set.nth( 2)->second == 2);
    CHECK(set.nth( 3)->first == 20); CHECK(set.nth( 3)->second == 1);
    CHECK(set.nth( 4)->first == 30); CHECK(set.nth( 4)->second == 2);
    CHECK(set.nth( 5)->first == 30); CHECK(set.nth( 5)->second == 1);
    CHECK(set.nth( 6)->first == 40); CHECK(set.nth( 6)->second == 2);
    CHECK(set.nth( 7)->first == 40); CHECK(set.nth( 7)->second == 1);
    CHECK(set.nth( 8)->first == 50); CHECK(set.nth( 8)->second == 2);
    CHECK(set.nth( 9)->first == 50); CHECK(set.nth( 9)->second == 1);
    CHECK(set.nth(10)->first == 60); CHECK(set.nth(10)->second == 2);
    CHECK(set.nth(11)->first == 60); CHECK(set.nth(11)->second == 1);
}

PRINT("Test container(const container&)");
{
    // N == 0
    {
        sfl::small_flat_multiset< xint_xint,
                                  0,
                                  std::less<xint_xint>,
                                  TPARAM_ALLOCATOR<xint_xint> > set1;

        set1.emplace(10, 1);
        set1.emplace(20, 1);
        set1.emplace(30, 1);

        CHECK(set1.size() == 3);
        CHECK(set1.nth(0)->first == 10); CHECK(set1.nth(0)->second == 1);
        CHECK(set1.nth(1)->first == 20); CHECK(set1.nth(1)->second == 1);
        CHECK(set1.nth(2)->first == 30); CHECK(set1.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        sfl::small_flat_multiset< xint_xint,
                                  0,
                                  std::less<xint_xint>,
                                  TPARAM_ALLOCATOR<xint_xint> > set2(set1);

        CHECK(set2.size() == 3);
        CHECK(set2.nth(0)->first == 10); CHECK(set2.nth(0)->second == 1);
        CHECK(set2.nth(1)->first == 20); CHECK(set2.nth(1)->second == 1);
        CHECK(set2.nth(2)->first == 30); CHECK(set2.nth(2)->second == 1);
    }

    // N == 100
    {
        sfl::small_flat_multiset< xint_xint,
                                  100,
                                  std::less<xint_xint>,
                                  TPARAM_ALLOCATOR<xint_xint> > set1;

        set1.emplace(10, 1);
        set1.emplace(20, 1);
        set1.emplace(30, 1);

        CHECK(set1.size() == 3);
        CHECK(set1.nth(0)->first == 10); CHECK(set1.nth(0)->second == 1);
        CHECK(set1.nth(1)->first == 20); CHECK(set1.nth(1)->second == 1);
        CHECK(set1.nth(2)->first == 30); CHECK(set1.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        sfl::small_flat_multiset< xint_xint,
                                  100,
                                  std::less<xint_xint>,
                                  TPARAM_ALLOCATOR<xint_xint> > set2(set1);

        CHECK(set2.size() == 3);
        CHECK(set2.nth(0)->first == 10); CHECK(set2.nth(0)->second == 1);
        CHECK(set2.nth(1)->first == 20); CHECK(set2.nth(1)->second == 1);
        CHECK(set2.nth(2)->first == 30); CHECK(set2.nth(2)->second == 1);
    }
}

PRINT("Test container(const container&, const Allocator&)");
{
    // N == 0
    {
        sfl::small_flat_multiset< xint_xint,
                                  0,
                                  std::less<xint_xint>,
                                  TPARAM_ALLOCATOR<xint_xint> > set1;

        set1.emplace(10, 1);
        set1.emplace(20, 1);
        set1.emplace(30, 1);

        CHECK(set1.size() == 3);
        CHECK(set1.nth(0)->first == 10); CHECK(set1.nth(0)->second == 1);
        CHECK(set1.nth(1)->first == 20); CHECK(set1.nth(1)->second == 1);
        CHECK(set1.nth(2)->first == 30); CHECK(set1.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        TPARAM_ALLOCATOR<xint_xint> alloc;

        sfl::small_flat_multiset< xint_xint,
                                  0,
                                  std::less<xint_xint>,
                                  TPARAM_ALLOCATOR<xint_xint> > set2(set1, alloc);

        CHECK(set2.size() == 3);
        CHECK(set2.nth(0)->first == 10); CHECK(set2.nth(0)->second == 1);
        CHECK(set2.nth(1)->first == 20); CHECK(set2.nth(1)->second == 1);
        CHECK(set2.nth(2)->first == 30); CHECK(set2.nth(2)->second == 1);
    }

    // N == 100
    {
        sfl::small_flat_multiset< xint_xint,
                                  100,
                                  std::less<xint_xint>,
                                  TPARAM_ALLOCATOR<xint_xint> > set1;

        set1.emplace(10, 1);
        set1.emplace(20, 1);
        set1.emplace(30, 1);

        CHECK(set1.size() == 3);
        CHECK(set1.nth(0)->first == 10); CHECK(set1.nth(0)->second == 1);
        CHECK(set1.nth(1)->first == 20); CHECK(set1.nth(1)->second == 1);
        CHECK(set1.nth(2)->first == 30); CHECK(set1.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        TPARAM_ALLOCATOR<xint_xint> alloc;

        sfl::small_flat_multiset< xint_xint,
                                  100,
                                  std::less<xint_xint>,
                                  TPARAM_ALLOCATOR<xint_xint> > set2(set1, alloc);

        CHECK(set2.size() == 3);
        CHECK(set2.nth(0)->first == 10); CHECK(set2.nth(0)->second == 1);
        CHECK(set2.nth(1)->first == 20); CHECK(set2.nth(1)->second == 1);
        CHECK(set2.nth(2)->first == 30); CHECK(set2.nth(2)->second == 1);
    }
}

PRINT("Test container(container&&)");
{
    // N == 0
    {
        sfl::small_flat_multiset< xint_xint,
                                  0,
                                  std::less<xint_xint>,
                                  TPARAM_ALLOCATOR<xint_xint> > set1;

        set1.emplace(10, 1);
        set1.emplace(20, 1);
        set1.emplace(30, 1);

        CHECK(set1.size() == 3);
        CHECK(set1.nth(0)->first == 10); CHECK(set1.nth(0)->second == 1);
        CHECK(set1.nth(1)->first == 20); CHECK(set1.nth(1)->second == 1);
        CHECK(set1.nth(2)->first == 30); CHECK(set1.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        sfl::small_flat_multiset< xint_xint,
                                  0,
                                  std::less<xint_xint>,
                                  TPARAM_ALLOCATOR<xint_xint> > set2(std::move(set1));

        CHECK(set2.size() == 3);
        CHECK(set2.nth(0)->first == 10); CHECK(set2.nth(0)->second == 1);
        CHECK(set2.nth(1)->first == 20); CHECK(set2.nth(1)->second == 1);
        CHECK(set2.nth(2)->first == 30); CHECK(set2.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        if (set1.size() == 3)
        {
            // Elements were moved one-by-one
            CHECK(set1.nth(0)->first == -10); CHECK(set1.nth(0)->second == -1);
            CHECK(set1.nth(1)->first == -20); CHECK(set1.nth(1)->second == -1);
            CHECK(set1.nth(2)->first == -30); CHECK(set1.nth(2)->second == -1);
        }
        else
        {
            // Storage was moved
            CHECK(set1.size() == 0);
        }
    }

    // N == 100
    {
        sfl::small_flat_multiset< xint_xint,
                                  100,
                                  std::less<xint_xint>,
                                  TPARAM_ALLOCATOR<xint_xint> > set1;

        set1.emplace(10, 1);
        set1.emplace(20, 1);
        set1.emplace(30, 1);

        CHECK(set1.size() == 3);
        CHECK(set1.nth(0)->first == 10); CHECK(set1.nth(0)->second == 1);
        CHECK(set1.nth(1)->first == 20); CHECK(set1.nth(1)->second == 1);
        CHECK(set1.nth(2)->first == 30); CHECK(set1.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        sfl::small_flat_multiset< xint_xint,
                                  100,
                                  std::less<xint_xint>,
                                  TPARAM_ALLOCATOR<xint_xint> > set2(std::move(set1));

        CHECK(set2.size() == 3);
        CHECK(set2.nth(0)->first == 10); CHECK(set2.nth(0)->second == 1);
        CHECK(set2.nth(1)->first == 20); CHECK(set2.nth(1)->second == 1);
        CHECK(set2.nth(2)->first == 30); CHECK(set2.nth(2)->second == 1);

        if (set1.size() == 3)
        {
            // Elements were moved one-by-one
            CHECK(set1.nth(0)->first == -10); CHECK(set1.nth(0)->second == -1);
            CHECK(set1.nth(1)->first == -20); CHECK(set1.nth(1)->second == -1);
            CHECK(set1.nth(2)->first == -30); CHECK(set1.nth(2)->second == -1);
        }
        else
        {
            // Storage was moved
            CHECK(set1.size() == 0);
        }
    }
}

PRINT("Test container(container&&, const Allocator&)");
{
    // N == 0
    {
        sfl::small_flat_multiset< xint_xint,
                                  0,
                                  std::less<xint_xint>,
                                  TPARAM_ALLOCATOR<xint_xint> > set1;

        set1.emplace(10, 1);
        set1.emplace(20, 1);
        set1.emplace(30, 1);

        CHECK(set1.size() == 3);
        CHECK(set1.nth(0)->first == 10); CHECK(set1.nth(0)->second == 1);
        CHECK(set1.nth(1)->first == 20); CHECK(set1.nth(1)->second == 1);
        CHECK(set1.nth(2)->first == 30); CHECK(set1.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        TPARAM_ALLOCATOR<xint_xint> alloc;

        sfl::small_flat_multiset< xint_xint,
                                  0,
                                  std::less<xint_xint>,
                                  TPARAM_ALLOCATOR<xint_xint> > set2(std::move(set1), alloc);

        CHECK(set2.size() == 3);
        CHECK(set2.nth(0)->first == 10); CHECK(set2.nth(0)->second == 1);
        CHECK(set2.nth(1)->first == 20); CHECK(set2.nth(1)->second == 1);
        CHECK(set2.nth(2)->first == 30); CHECK(set2.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        if (set1.size() == 3)
        {
            // Elements were moved one-by-one
            CHECK(set1.nth(0)->first == -10); CHECK(set1.nth(0)->second == -1);
            CHECK(set1.nth(1)->first == -20); CHECK(set1.nth(1)->second == -1);
            CHECK(set1.nth(2)->first == -30); CHECK(set1.nth(2)->second == -1);
        }
        else
        {
            // Storage was moved
            CHECK(set1.size() == 0);
        }
    }

    // N == 100
    {
        sfl::small_flat_multiset< xint_xint,
                                  100,
                                  std::less<xint_xint>,
                                  TPARAM_ALLOCATOR<xint_xint> > set1;

        set1.emplace(10, 1);
        set1.emplace(20, 1);
        set1.emplace(30, 1);

        CHECK(set1.size() == 3);
        CHECK(set1.nth(0)->first == 10); CHECK(set1.nth(0)->second == 1);
        CHECK(set1.nth(1)->first == 20); CHECK(set1.nth(1)->second == 1);
        CHECK(set1.nth(2)->first == 30); CHECK(set1.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        TPARAM_ALLOCATOR<xint_xint> alloc;

        sfl::small_flat_multiset< xint_xint,
                                  100,
                                  std::less<xint_xint>,
                                  TPARAM_ALLOCATOR<xint_xint> > set2(std::move(set1), alloc);

        CHECK(set2.size() == 3);
        CHECK(set2.nth(0)->first == 10); CHECK(set2.nth(0)->second == 1);
        CHECK(set2.nth(1)->first == 20); CHECK(set2.nth(1)->second == 1);
        CHECK(set2.nth(2)->first == 30); CHECK(set2.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        if (set1.size() == 3)
        {
            // Elements were moved one-by-one
            CHECK(set1.nth(0)->first == -10); CHECK(set1.nth(0)->second == -1);
            CHECK(set1.nth(1)->first == -20); CHECK(set1.nth(1)->second == -1);
            CHECK(set1.nth(2)->first == -30); CHECK(set1.nth(2)->second == -1);
        }
        else
        {
            // Storage was moved
            CHECK(set1.size() == 0);
        }
    }
}

PRINT("Test container(sfl::from_range_t, Range&&)");
{
    // Input iterator (exactly)
    {
        std::istringstream iss("10 20 30 20 20");

        #if SFL_CPP_VERSION >= SFL_CPP_20
        sfl::small_flat_multiset<xint, 32, std::less<xint>, TPARAM_ALLOCATOR<xint>> set
        (
            (sfl::from_range_t()),
            (std::views::istream<int>(iss))
        );
        #else
        sfl::small_flat_multiset<xint, 32, std::less<xint>, TPARAM_ALLOCATOR<xint>> set
        (
            (sfl::from_range_t()),
            (sfl::test::istream_view<int>(iss))
        );
        #endif

        CHECK(set.empty() == false);
        CHECK(set.size() == 5);
        CHECK(set.max_size() > 0);
        CHECK(*NTH(set, 0) == 10);
        CHECK(*NTH(set, 1) == 20);
        CHECK(*NTH(set, 2) == 20);
        CHECK(*NTH(set, 3) == 20);
        CHECK(*NTH(set, 4) == 30);
    }

    // Forward iterator
    {
        std::vector<xint> data({10, 20, 30, 20, 20});

        #if SFL_CPP_VERSION >= SFL_CPP_20
        sfl::small_flat_multiset<xint, 32, std::less<xint>, TPARAM_ALLOCATOR<xint>> set
        (
            sfl::from_range_t(),
            std::views::all(data)
        );
        #else
        sfl::small_flat_multiset<xint, 32, std::less<xint>, TPARAM_ALLOCATOR<xint>> set
        (
            sfl::from_range_t(),
            data
        );
        #endif

        CHECK(set.empty() == false);
        CHECK(set.size() == 5);
        CHECK(set.max_size() > 0);
        CHECK(*NTH(set, 0) == 10);
        CHECK(*NTH(set, 1) == 20);
        CHECK(*NTH(set, 2) == 20);
        CHECK(*NTH(set, 3) == 20);
        CHECK(*NTH(set, 4) == 30);
    }
}

PRINT("Test container(sfl::from_range_t, Range&&, const Compare&)");
{
    // Input iterator (exactly)
    {
        std::istringstream iss("10 20 30 20 20");

        std::less<xint> comp;

        #if SFL_CPP_VERSION >= SFL_CPP_20
        sfl::small_flat_multiset<xint, 32, std::less<xint>, TPARAM_ALLOCATOR<xint>> set
        (
            (sfl::from_range_t()),
            (std::views::istream<int>(iss)),
            comp
        );
        #else
        sfl::small_flat_multiset<xint, 32, std::less<xint>, TPARAM_ALLOCATOR<xint>> set
        (
            (sfl::from_range_t()),
            (sfl::test::istream_view<int>(iss)),
            comp
        );
        #endif

        CHECK(set.empty() == false);
        CHECK(set.size() == 5);
        CHECK(set.max_size() > 0);
        CHECK(*NTH(set, 0) == 10);
        CHECK(*NTH(set, 1) == 20);
        CHECK(*NTH(set, 2) == 20);
        CHECK(*NTH(set, 3) == 20);
        CHECK(*NTH(set, 4) == 30);
    }

    // Forward iterator
    {
        std::vector<xint> data({10, 20, 30, 20, 20});

        std::less<xint> comp;

        #if SFL_CPP_VERSION >= SFL_CPP_20
        sfl::small_flat_multiset<xint, 32, std::less<xint>, TPARAM_ALLOCATOR<xint>> set
        (
            sfl::from_range_t(),
            std::views::all(data),
            comp
        );
        #else
        sfl::small_flat_multiset<xint, 32, std::less<xint>, TPARAM_ALLOCATOR<xint>> set
        (
            sfl::from_range_t(),
            data,
            comp
        );
        #endif

        CHECK(set.empty() == false);
        CHECK(set.size() == 5);
        CHECK(set.max_size() > 0);
        CHECK(*NTH(set, 0) == 10);
        CHECK(*NTH(set, 1) == 20);
        CHECK(*NTH(set, 2) == 20);
        CHECK(*NTH(set, 3) == 20);
        CHECK(*NTH(set, 4) == 30);
    }
}

PRINT("Test container(sfl::from_range_t, Range&&, const Allocator&)");
{
    // Input iterator (exactly)
    {
        std::istringstream iss("10 20 30 20 20");

        TPARAM_ALLOCATOR<xint> alloc;

        #if SFL_CPP_VERSION >= SFL_CPP_20
        sfl::small_flat_multiset<xint, 32, std::less<xint>, TPARAM_ALLOCATOR<xint>> set
        (
            (sfl::from_range_t()),
            (std::views::istream<int>(iss)),
            alloc
        );
        #else
        sfl::small_flat_multiset<xint, 32, std::less<xint>, TPARAM_ALLOCATOR<xint>> set
        (
            (sfl::from_range_t()),
            (sfl::test::istream_view<int>(iss)),
            alloc
        );
        #endif

        CHECK(set.empty() == false);
        CHECK(set.size() == 5);
        CHECK(set.max_size() > 0);
        CHECK(*NTH(set, 0) == 10);
        CHECK(*NTH(set, 1) == 20);
        CHECK(*NTH(set, 2) == 20);
        CHECK(*NTH(set, 3) == 20);
        CHECK(*NTH(set, 4) == 30);
    }

    // Forward iterator
    {
        std::vector<xint> data({10, 20, 30, 20, 20});

        TPARAM_ALLOCATOR<xint> alloc;

        #if SFL_CPP_VERSION >= SFL_CPP_20
        sfl::small_flat_multiset<xint, 32, std::less<xint>, TPARAM_ALLOCATOR<xint>> set
        (
            sfl::from_range_t(),
            std::views::all(data),
            alloc
        );
        #else
        sfl::small_flat_multiset<xint, 32, std::less<xint>, TPARAM_ALLOCATOR<xint>> set
        (
            sfl::from_range_t(),
            data,
            alloc
        );
        #endif

        CHECK(set.empty() == false);
        CHECK(set.size() == 5);
        CHECK(set.max_size() > 0);
        CHECK(*NTH(set, 0) == 10);
        CHECK(*NTH(set, 1) == 20);
        CHECK(*NTH(set, 2) == 20);
        CHECK(*NTH(set, 3) == 20);
        CHECK(*NTH(set, 4) == 30);
    }
}

PRINT("Test container(sfl::from_range_t, Range&&, const Compare&, const Allocator&)");
{
    // Input iterator (exactly)
    {
        std::istringstream iss("10 20 30 20 20");

        std::less<xint> comp;
        TPARAM_ALLOCATOR<xint> alloc;

        #if SFL_CPP_VERSION >= SFL_CPP_20
        sfl::small_flat_multiset<xint, 32, std::less<xint>, TPARAM_ALLOCATOR<xint>> set
        (
            (sfl::from_range_t()),
            (std::views::istream<int>(iss)),
            comp,
            alloc
        );
        #else
        sfl::small_flat_multiset<xint, 32, std::less<xint>, TPARAM_ALLOCATOR<xint>> set
        (
            (sfl::from_range_t()),
            (sfl::test::istream_view<int>(iss)),
            comp,
            alloc
        );
        #endif

        CHECK(set.empty() == false);
        CHECK(set.size() == 5);
        CHECK(set.max_size() > 0);
        CHECK(*NTH(set, 0) == 10);
        CHECK(*NTH(set, 1) == 20);
        CHECK(*NTH(set, 2) == 20);
        CHECK(*NTH(set, 3) == 20);
        CHECK(*NTH(set, 4) == 30);
    }

    // Forward iterator
    {
        std::vector<xint> data({10, 20, 30, 20, 20});

        std::less<xint> comp;
        TPARAM_ALLOCATOR<xint> alloc;

        #if SFL_CPP_VERSION >= SFL_CPP_20
        sfl::small_flat_multiset<xint, 32, std::less<xint>, TPARAM_ALLOCATOR<xint>> set
        (
            sfl::from_range_t(),
            std::views::all(data),
            comp,
            alloc
        );
        #else
        sfl::small_flat_multiset<xint, 32, std::less<xint>, TPARAM_ALLOCATOR<xint>> set
        (
            sfl::from_range_t(),
            data,
            comp,
            alloc
        );
        #endif

        CHECK(set.empty() == false);
        CHECK(set.size() == 5);
        CHECK(set.max_size() > 0);
        CHECK(*NTH(set, 0) == 10);
        CHECK(*NTH(set, 1) == 20);
        CHECK(*NTH(set, 2) == 20);
        CHECK(*NTH(set, 3) == 20);
        CHECK(*NTH(set, 4) == 30);
    }
}

///////////////////////////////////////////////////////////////////////////////

PRINT("Test operator=(const container&)");
{
    // Permutation 1/8
    #define CONDITION set1.size() < set2.size()  &&  set1.size() < set2.capacity()  &&  set1.capacity() < set2.capacity()
    {
        sfl::small_flat_multiset< xint_xint,
                                  0,
                                  std::less<xint_xint>,
                                  TPARAM_ALLOCATOR<xint_xint> > set1, set2;

        set1.reserve(3);
        set1.emplace(10, 1);
        set1.emplace(20, 1);
        set1.emplace(30, 1);

        CHECK(set1.size() == 3);
        CHECK(set1.nth(0)->first == 10); CHECK(set1.nth(0)->second == 1);
        CHECK(set1.nth(1)->first == 20); CHECK(set1.nth(1)->second == 1);
        CHECK(set1.nth(2)->first == 30); CHECK(set1.nth(2)->second == 1);

        set2.reserve(5);
        set2.emplace(40, 2);
        set2.emplace(50, 2);
        set2.emplace(60, 2);
        set2.emplace(70, 2);
        set2.emplace(80, 2);

        CHECK(set2.size() == 5);
        CHECK(set2.nth(0)->first == 40); CHECK(set2.nth(0)->second == 2);
        CHECK(set2.nth(1)->first == 50); CHECK(set2.nth(1)->second == 2);
        CHECK(set2.nth(2)->first == 60); CHECK(set2.nth(2)->second == 2);
        CHECK(set2.nth(3)->first == 70); CHECK(set2.nth(3)->second == 2);
        CHECK(set2.nth(4)->first == 80); CHECK(set2.nth(4)->second == 2);

        ///////////////////////////////////////////////////////////////////////

        CHECK(CONDITION);

        set1 = set2;

        CHECK(set1.size() == 5);
        CHECK(set1.nth(0)->first == 40); CHECK(set1.nth(0)->second == 2);
        CHECK(set1.nth(1)->first == 50); CHECK(set1.nth(1)->second == 2);
        CHECK(set1.nth(2)->first == 60); CHECK(set1.nth(2)->second == 2);
        CHECK(set1.nth(3)->first == 70); CHECK(set1.nth(3)->second == 2);
        CHECK(set1.nth(4)->first == 80); CHECK(set1.nth(4)->second == 2);

        CHECK(set2.size() == 5);
        CHECK(set2.nth(0)->first == 40); CHECK(set2.nth(0)->second == 2);
        CHECK(set2.nth(1)->first == 50); CHECK(set2.nth(1)->second == 2);
        CHECK(set2.nth(2)->first == 60); CHECK(set2.nth(2)->second == 2);
        CHECK(set2.nth(3)->first == 70); CHECK(set2.nth(3)->second == 2);
        CHECK(set2.nth(4)->first == 80); CHECK(set2.nth(4)->second == 2);
    }
    #undef CONDITION

    // Permutation 2/8
    #define CONDITION set1.size() < set2.size()  &&  set1.size() < set2.capacity()  &&  set1.capacity() > set2.capacity()
    {
        sfl::small_flat_multiset< xint_xint,
                                  0,
                                  std::less<xint_xint>,
                                  TPARAM_ALLOCATOR<xint_xint> > set1, set2;

        set1.reserve(100);
        set1.emplace(10, 1);
        set1.emplace(20, 1);
        set1.emplace(30, 1);

        CHECK(set1.size() == 3);
        CHECK(set1.nth(0)->first == 10); CHECK(set1.nth(0)->second == 1);
        CHECK(set1.nth(1)->first == 20); CHECK(set1.nth(1)->second == 1);
        CHECK(set1.nth(2)->first == 30); CHECK(set1.nth(2)->second == 1);

        set2.reserve(5);
        set2.emplace(40, 2);
        set2.emplace(50, 2);
        set2.emplace(60, 2);
        set2.emplace(70, 2);
        set2.emplace(80, 2);

        CHECK(set2.size() == 5);
        CHECK(set2.nth(0)->first == 40); CHECK(set2.nth(0)->second == 2);
        CHECK(set2.nth(1)->first == 50); CHECK(set2.nth(1)->second == 2);
        CHECK(set2.nth(2)->first == 60); CHECK(set2.nth(2)->second == 2);
        CHECK(set2.nth(3)->first == 70); CHECK(set2.nth(3)->second == 2);
        CHECK(set2.nth(4)->first == 80); CHECK(set2.nth(4)->second == 2);

        ///////////////////////////////////////////////////////////////////////

        CHECK(CONDITION);

        set1 = set2;

        CHECK(set1.size() == 5);
        CHECK(set1.nth(0)->first == 40); CHECK(set1.nth(0)->second == 2);
        CHECK(set1.nth(1)->first == 50); CHECK(set1.nth(1)->second == 2);
        CHECK(set1.nth(2)->first == 60); CHECK(set1.nth(2)->second == 2);
        CHECK(set1.nth(3)->first == 70); CHECK(set1.nth(3)->second == 2);
        CHECK(set1.nth(4)->first == 80); CHECK(set1.nth(4)->second == 2);

        CHECK(set2.size() == 5);
        CHECK(set2.nth(0)->first == 40); CHECK(set2.nth(0)->second == 2);
        CHECK(set2.nth(1)->first == 50); CHECK(set2.nth(1)->second == 2);
        CHECK(set2.nth(2)->first == 60); CHECK(set2.nth(2)->second == 2);
        CHECK(set2.nth(3)->first == 70); CHECK(set2.nth(3)->second == 2);
        CHECK(set2.nth(4)->first == 80); CHECK(set2.nth(4)->second == 2);
    }
    #undef CONDITION

    // Permutation 3/8
    #define CONDITION set1.size() < set2.size()  &&  set1.size() > set2.capacity()  &&  set1.capacity() < set2.capacity()
    {
        // Impossible condition
    }
    #undef CONDITION

    // Permutation 4/8
    #define CONDITION set1.size() < set2.size()  &&  set1.size() > set2.capacity()  &&  set1.capacity() > set2.capacity()
    {
        // Impossible condition
    }
    #undef CONDITION

    // Permutation 5/8
    #define CONDITION set1.size() > set2.size()  &&  set1.size() < set2.capacity()  &&  set1.capacity() < set2.capacity()
    {
        sfl::small_flat_multiset< xint_xint,
                                  0,
                                  std::less<xint_xint>,
                                  TPARAM_ALLOCATOR<xint_xint> > set1, set2;

        set1.reserve(5);
        set1.emplace(10, 1);
        set1.emplace(20, 1);
        set1.emplace(30, 1);
        set1.emplace(40, 1);
        set1.emplace(50, 1);

        CHECK(set1.size() == 5);
        CHECK(set1.nth(0)->first == 10); CHECK(set1.nth(0)->second == 1);
        CHECK(set1.nth(1)->first == 20); CHECK(set1.nth(1)->second == 1);
        CHECK(set1.nth(2)->first == 30); CHECK(set1.nth(2)->second == 1);
        CHECK(set1.nth(3)->first == 40); CHECK(set1.nth(3)->second == 1);
        CHECK(set1.nth(4)->first == 50); CHECK(set1.nth(4)->second == 1);

        set2.reserve(100);
        set2.emplace(60, 2);
        set2.emplace(70, 2);
        set2.emplace(80, 2);

        CHECK(set2.size() == 3);
        CHECK(set2.nth(0)->first == 60); CHECK(set2.nth(0)->second == 2);
        CHECK(set2.nth(1)->first == 70); CHECK(set2.nth(1)->second == 2);
        CHECK(set2.nth(2)->first == 80); CHECK(set2.nth(2)->second == 2);

        ///////////////////////////////////////////////////////////////////////

        CHECK(CONDITION);

        set1 = set2;

        CHECK(set1.size() == 3);
        CHECK(set1.nth(0)->first == 60); CHECK(set1.nth(0)->second == 2);
        CHECK(set1.nth(1)->first == 70); CHECK(set1.nth(1)->second == 2);
        CHECK(set1.nth(2)->first == 80); CHECK(set1.nth(2)->second == 2);

        CHECK(set2.size() == 3);
        CHECK(set2.nth(0)->first == 60); CHECK(set2.nth(0)->second == 2);
        CHECK(set2.nth(1)->first == 70); CHECK(set2.nth(1)->second == 2);
        CHECK(set2.nth(2)->first == 80); CHECK(set2.nth(2)->second == 2);
    }
    #undef CONDITION

    // Permutation 6/8
    #define CONDITION set1.size() > set2.size()  &&  set1.size() < set2.capacity()  &&  set1.capacity() > set2.capacity()
    {
        sfl::small_flat_multiset< xint_xint,
                                  0,
                                  std::less<xint_xint>,
                                  TPARAM_ALLOCATOR<xint_xint> > set1, set2;

        set1.reserve(100);
        set1.emplace(10, 1);
        set1.emplace(20, 1);
        set1.emplace(30, 1);
        set1.emplace(40, 1);
        set1.emplace(50, 1);

        CHECK(set1.size() == 5);
        CHECK(set1.nth(0)->first == 10); CHECK(set1.nth(0)->second == 1);
        CHECK(set1.nth(1)->first == 20); CHECK(set1.nth(1)->second == 1);
        CHECK(set1.nth(2)->first == 30); CHECK(set1.nth(2)->second == 1);
        CHECK(set1.nth(3)->first == 40); CHECK(set1.nth(3)->second == 1);
        CHECK(set1.nth(4)->first == 50); CHECK(set1.nth(4)->second == 1);

        set2.reserve(10);
        set2.emplace(60, 2);
        set2.emplace(70, 2);
        set2.emplace(80, 2);

        CHECK(set2.size() == 3);
        CHECK(set2.nth(0)->first == 60); CHECK(set2.nth(0)->second == 2);
        CHECK(set2.nth(1)->first == 70); CHECK(set2.nth(1)->second == 2);
        CHECK(set2.nth(2)->first == 80); CHECK(set2.nth(2)->second == 2);

        ///////////////////////////////////////////////////////////////////////

        CHECK(CONDITION);

        set1 = set2;

        CHECK(set1.size() == 3);
        CHECK(set1.nth(0)->first == 60); CHECK(set1.nth(0)->second == 2);
        CHECK(set1.nth(1)->first == 70); CHECK(set1.nth(1)->second == 2);
        CHECK(set1.nth(2)->first == 80); CHECK(set1.nth(2)->second == 2);

        CHECK(set2.size() == 3);
        CHECK(set2.nth(0)->first == 60); CHECK(set2.nth(0)->second == 2);
        CHECK(set2.nth(1)->first == 70); CHECK(set2.nth(1)->second == 2);
        CHECK(set2.nth(2)->first == 80); CHECK(set2.nth(2)->second == 2);
    }
    #undef CONDITION

    // Permutation 7/8
    #define CONDITION set1.size() > set2.size()  &&  set1.size() > set2.capacity()  &&  set1.capacity() < set2.capacity()
    {
        // Impossible condition
    }
    #undef CONDITION

    // Permutation 8/8
    #define CONDITION set1.size() > set2.size()  &&  set1.size() > set2.capacity()  &&  set1.capacity() > set2.capacity()
    {
        sfl::small_flat_multiset< xint_xint,
                                  0,
                                  std::less<xint_xint>,
                                  TPARAM_ALLOCATOR<xint_xint> > set1, set2;

        set1.reserve(100);
        set1.emplace(10, 1);
        set1.emplace(20, 1);
        set1.emplace(30, 1);
        set1.emplace(40, 1);
        set1.emplace(50, 1);

        CHECK(set1.size() == 5);
        CHECK(set1.nth(0)->first == 10); CHECK(set1.nth(0)->second == 1);
        CHECK(set1.nth(1)->first == 20); CHECK(set1.nth(1)->second == 1);
        CHECK(set1.nth(2)->first == 30); CHECK(set1.nth(2)->second == 1);
        CHECK(set1.nth(3)->first == 40); CHECK(set1.nth(3)->second == 1);
        CHECK(set1.nth(4)->first == 50); CHECK(set1.nth(4)->second == 1);

        set2.reserve(3);
        set2.emplace(60, 2);
        set2.emplace(70, 2);
        set2.emplace(80, 2);

        CHECK(set2.size() == 3);
        CHECK(set2.nth(0)->first == 60); CHECK(set2.nth(0)->second == 2);
        CHECK(set2.nth(1)->first == 70); CHECK(set2.nth(1)->second == 2);
        CHECK(set2.nth(2)->first == 80); CHECK(set2.nth(2)->second == 2);

        ///////////////////////////////////////////////////////////////////////

        CHECK(CONDITION);

        set1 = set2;

        CHECK(set1.size() == 3);
        CHECK(set1.nth(0)->first == 60); CHECK(set1.nth(0)->second == 2);
        CHECK(set1.nth(1)->first == 70); CHECK(set1.nth(1)->second == 2);
        CHECK(set1.nth(2)->first == 80); CHECK(set1.nth(2)->second == 2);

        CHECK(set2.size() == 3);
        CHECK(set2.nth(0)->first == 60); CHECK(set2.nth(0)->second == 2);
        CHECK(set2.nth(1)->first == 70); CHECK(set2.nth(1)->second == 2);
        CHECK(set2.nth(2)->first == 80); CHECK(set2.nth(2)->second == 2);
    }
    #undef CONDITION
}

PRINT("Test operator=(container&&)");
{
    #define N 0
    {
        // Permutation 1/8
        #define CONDITION set2.capacity() != N  &&  set1.size() < set2.size()  &&  set1.size() < set2.capacity()  &&  set1.capacity() < set2.capacity()
        {
            sfl::small_flat_multiset< xint_xint,
                                      N,
                                      std::less<xint_xint>,
                                      TPARAM_ALLOCATOR<xint_xint> > set1, set2;

            set1.reserve(3);
            set1.emplace(10, 1);
            set1.emplace(20, 1);
            set1.emplace(30, 1);

            CHECK(set1.size() == 3);
            CHECK(set1.nth(0)->first == 10); CHECK(set1.nth(0)->second == 1);
            CHECK(set1.nth(1)->first == 20); CHECK(set1.nth(1)->second == 1);
            CHECK(set1.nth(2)->first == 30); CHECK(set1.nth(2)->second == 1);

            set2.reserve(5);
            set2.emplace(40, 2);
            set2.emplace(50, 2);
            set2.emplace(60, 2);
            set2.emplace(70, 2);
            set2.emplace(80, 2);

            CHECK(set2.size() == 5);
            CHECK(set2.nth(0)->first == 40); CHECK(set2.nth(0)->second == 2);
            CHECK(set2.nth(1)->first == 50); CHECK(set2.nth(1)->second == 2);
            CHECK(set2.nth(2)->first == 60); CHECK(set2.nth(2)->second == 2);
            CHECK(set2.nth(3)->first == 70); CHECK(set2.nth(3)->second == 2);
            CHECK(set2.nth(4)->first == 80); CHECK(set2.nth(4)->second == 2);

            ///////////////////////////////////////////////////////////////////////

            CHECK(CONDITION);

            set1 = std::move(set2);

            CHECK(set1.size() == 5);
            CHECK(set1.nth(0)->first == 40); CHECK(set1.nth(0)->second == 2);
            CHECK(set1.nth(1)->first == 50); CHECK(set1.nth(1)->second == 2);
            CHECK(set1.nth(2)->first == 60); CHECK(set1.nth(2)->second == 2);
            CHECK(set1.nth(3)->first == 70); CHECK(set1.nth(3)->second == 2);
            CHECK(set1.nth(4)->first == 80); CHECK(set1.nth(4)->second == 2);

            if (set2.size() == 5)
            {
                // Elements were moved one-by-one
                CHECK(set2.nth(0)->first == -40); CHECK(set2.nth(0)->second == -2);
                CHECK(set2.nth(1)->first == -50); CHECK(set2.nth(1)->second == -2);
                CHECK(set2.nth(2)->first == -60); CHECK(set2.nth(2)->second == -2);
                CHECK(set2.nth(3)->first == -70); CHECK(set2.nth(3)->second == -2);
                CHECK(set2.nth(4)->first == -80); CHECK(set2.nth(4)->second == -2);
            }
            else
            {
                // Storage was moved
                CHECK(set2.size() == 0);
            }
        }
        #undef CONDITION

        // Permutation 2/8
        #define CONDITION set2.capacity() != N  &&  set1.size() < set2.size()  &&  set1.size() < set2.capacity()  &&  set1.capacity() > set2.capacity()
        {
            sfl::small_flat_multiset< xint_xint,
                                      N,
                                      std::less<xint_xint>,
                                      TPARAM_ALLOCATOR<xint_xint> > set1, set2;

            set1.reserve(100);
            set1.emplace(10, 1);
            set1.emplace(20, 1);
            set1.emplace(30, 1);

            CHECK(set1.size() == 3);
            CHECK(set1.nth(0)->first == 10); CHECK(set1.nth(0)->second == 1);
            CHECK(set1.nth(1)->first == 20); CHECK(set1.nth(1)->second == 1);
            CHECK(set1.nth(2)->first == 30); CHECK(set1.nth(2)->second == 1);

            set2.reserve(5);
            set2.emplace(40, 2);
            set2.emplace(50, 2);
            set2.emplace(60, 2);
            set2.emplace(70, 2);
            set2.emplace(80, 2);

            CHECK(set2.size() == 5);
            CHECK(set2.nth(0)->first == 40); CHECK(set2.nth(0)->second == 2);
            CHECK(set2.nth(1)->first == 50); CHECK(set2.nth(1)->second == 2);
            CHECK(set2.nth(2)->first == 60); CHECK(set2.nth(2)->second == 2);
            CHECK(set2.nth(3)->first == 70); CHECK(set2.nth(3)->second == 2);
            CHECK(set2.nth(4)->first == 80); CHECK(set2.nth(4)->second == 2);

            ///////////////////////////////////////////////////////////////////////

            CHECK(CONDITION);

            set1 = std::move(set2);

            CHECK(set1.size() == 5);
            CHECK(set1.nth(0)->first == 40); CHECK(set1.nth(0)->second == 2);
            CHECK(set1.nth(1)->first == 50); CHECK(set1.nth(1)->second == 2);
            CHECK(set1.nth(2)->first == 60); CHECK(set1.nth(2)->second == 2);
            CHECK(set1.nth(3)->first == 70); CHECK(set1.nth(3)->second == 2);
            CHECK(set1.nth(4)->first == 80); CHECK(set1.nth(4)->second == 2);

            if (set2.size() == 5)
            {
                // Elements were moved one-by-one
                CHECK(set2.nth(0)->first == -40); CHECK(set2.nth(0)->second == -2);
                CHECK(set2.nth(1)->first == -50); CHECK(set2.nth(1)->second == -2);
                CHECK(set2.nth(2)->first == -60); CHECK(set2.nth(2)->second == -2);
                CHECK(set2.nth(3)->first == -70); CHECK(set2.nth(3)->second == -2);
                CHECK(set2.nth(4)->first == -80); CHECK(set2.nth(4)->second == -2);
            }
            else
            {
                // Storage was moved
                CHECK(set2.size() == 0);
            }
        }
        #undef CONDITION

        // Permutation 3/8
        #define CONDITION set2.capacity() != N  &&  set1.size() < set2.size()  &&  set1.size() > set2.capacity()  &&  set1.capacity() < set2.capacity()
        {
            // Impossible condition
        }
        #undef CONDITION

        // Permutation 4/8
        #define CONDITION set2.capacity() != N  &&  set1.size() < set2.size()  &&  set1.size() > set2.capacity()  &&  set1.capacity() > set2.capacity()
        {
            // Impossible condition
        }
        #undef CONDITION

        // Permutation 5/8
        #define CONDITION set2.capacity() != N  &&  set1.size() > set2.size()  &&  set1.size() < set2.capacity()  &&  set1.capacity() < set2.capacity()
        {
            sfl::small_flat_multiset< xint_xint,
                                      N,
                                      std::less<xint_xint>,
                                      TPARAM_ALLOCATOR<xint_xint> > set1, set2;

            set1.reserve(5);
            set1.emplace(10, 1);
            set1.emplace(20, 1);
            set1.emplace(30, 1);
            set1.emplace(40, 1);
            set1.emplace(50, 1);

            CHECK(set1.size() == 5);
            CHECK(set1.nth(0)->first == 10); CHECK(set1.nth(0)->second == 1);
            CHECK(set1.nth(1)->first == 20); CHECK(set1.nth(1)->second == 1);
            CHECK(set1.nth(2)->first == 30); CHECK(set1.nth(2)->second == 1);
            CHECK(set1.nth(3)->first == 40); CHECK(set1.nth(3)->second == 1);
            CHECK(set1.nth(4)->first == 50); CHECK(set1.nth(4)->second == 1);

            set2.reserve(100);
            set2.emplace(60, 2);
            set2.emplace(70, 2);
            set2.emplace(80, 2);

            CHECK(set2.size() == 3);
            CHECK(set2.nth(0)->first == 60); CHECK(set2.nth(0)->second == 2);
            CHECK(set2.nth(1)->first == 70); CHECK(set2.nth(1)->second == 2);
            CHECK(set2.nth(2)->first == 80); CHECK(set2.nth(2)->second == 2);

            ///////////////////////////////////////////////////////////////////////

            CHECK(CONDITION);

            set1 = std::move(set2);

            CHECK(set1.size() == 3);
            CHECK(set1.nth(0)->first == 60); CHECK(set1.nth(0)->second == 2);
            CHECK(set1.nth(1)->first == 70); CHECK(set1.nth(1)->second == 2);
            CHECK(set1.nth(2)->first == 80); CHECK(set1.nth(2)->second == 2);

            if (set2.size() == 3)
            {
                // Elements were moved one-by-one
                CHECK(set2.nth(0)->first == -60); CHECK(set2.nth(0)->second == -2);
                CHECK(set2.nth(1)->first == -70); CHECK(set2.nth(1)->second == -2);
                CHECK(set2.nth(2)->first == -80); CHECK(set2.nth(2)->second == -2);
            }
            else
            {
                // Storage was moved
                CHECK(set2.size() == 0);
            }
        }
        #undef CONDITION

        // Permutation 6/8
        #define CONDITION set2.capacity() != N  &&  set1.size() > set2.size()  &&  set1.size() < set2.capacity()  &&  set1.capacity() > set2.capacity()
        {
            sfl::small_flat_multiset< xint_xint,
                                      N,
                                      std::less<xint_xint>,
                                      TPARAM_ALLOCATOR<xint_xint> > set1, set2;

            set1.reserve(100);
            set1.emplace(10, 1);
            set1.emplace(20, 1);
            set1.emplace(30, 1);
            set1.emplace(40, 1);
            set1.emplace(50, 1);

            CHECK(set1.size() == 5);
            CHECK(set1.nth(0)->first == 10); CHECK(set1.nth(0)->second == 1);
            CHECK(set1.nth(1)->first == 20); CHECK(set1.nth(1)->second == 1);
            CHECK(set1.nth(2)->first == 30); CHECK(set1.nth(2)->second == 1);
            CHECK(set1.nth(3)->first == 40); CHECK(set1.nth(3)->second == 1);
            CHECK(set1.nth(4)->first == 50); CHECK(set1.nth(4)->second == 1);

            set2.reserve(10);
            set2.emplace(60, 2);
            set2.emplace(70, 2);
            set2.emplace(80, 2);

            CHECK(set2.size() == 3);
            CHECK(set2.nth(0)->first == 60); CHECK(set2.nth(0)->second == 2);
            CHECK(set2.nth(1)->first == 70); CHECK(set2.nth(1)->second == 2);
            CHECK(set2.nth(2)->first == 80); CHECK(set2.nth(2)->second == 2);

            ///////////////////////////////////////////////////////////////////////

            CHECK(CONDITION);

            set1 = std::move(set2);

            CHECK(set1.size() == 3);
            CHECK(set1.nth(0)->first == 60); CHECK(set1.nth(0)->second == 2);
            CHECK(set1.nth(1)->first == 70); CHECK(set1.nth(1)->second == 2);
            CHECK(set1.nth(2)->first == 80); CHECK(set1.nth(2)->second == 2);

            if (set2.size() == 3)
            {
                // Elements were moved one-by-one
                CHECK(set2.nth(0)->first == -60); CHECK(set2.nth(0)->second == -2);
                CHECK(set2.nth(1)->first == -70); CHECK(set2.nth(1)->second == -2);
                CHECK(set2.nth(2)->first == -80); CHECK(set2.nth(2)->second == -2);
            }
            else
            {
                // Storage was moved
                CHECK(set2.size() == 0);
            }
        }
        #undef CONDITION

        // Permutation 7/8
        #define CONDITION set2.capacity() != N  &&  set1.size() > set2.size()  &&  set1.size() > set2.capacity()  &&  set1.capacity() < set2.capacity()
        {
            // Impossible condition
        }
        #undef CONDITION

        // Permutation 8/8
        #define CONDITION set2.capacity() != N  &&  set1.size() > set2.size()  &&  set1.size() > set2.capacity()  &&  set1.capacity() > set2.capacity()
        {
            sfl::small_flat_multiset< xint_xint,
                                      N,
                                      std::less<xint_xint>,
                                      TPARAM_ALLOCATOR<xint_xint> > set1, set2;

            set1.reserve(5);
            set1.emplace(10, 1);
            set1.emplace(20, 1);
            set1.emplace(30, 1);
            set1.emplace(40, 1);
            set1.emplace(50, 1);

            CHECK(set1.size() == 5);
            CHECK(set1.nth(0)->first == 10); CHECK(set1.nth(0)->second == 1);
            CHECK(set1.nth(1)->first == 20); CHECK(set1.nth(1)->second == 1);
            CHECK(set1.nth(2)->first == 30); CHECK(set1.nth(2)->second == 1);
            CHECK(set1.nth(3)->first == 40); CHECK(set1.nth(3)->second == 1);
            CHECK(set1.nth(4)->first == 50); CHECK(set1.nth(4)->second == 1);

            set2.reserve(3);
            set2.emplace(60, 2);
            set2.emplace(70, 2);
            set2.emplace(80, 2);

            CHECK(set2.size() == 3);
            CHECK(set2.nth(0)->first == 60); CHECK(set2.nth(0)->second == 2);
            CHECK(set2.nth(1)->first == 70); CHECK(set2.nth(1)->second == 2);
            CHECK(set2.nth(2)->first == 80); CHECK(set2.nth(2)->second == 2);

            ///////////////////////////////////////////////////////////////////////

            CHECK(CONDITION);

            set1 = std::move(set2);

            CHECK(set1.size() == 3);
            CHECK(set1.nth(0)->first == 60); CHECK(set1.nth(0)->second == 2);
            CHECK(set1.nth(1)->first == 70); CHECK(set1.nth(1)->second == 2);
            CHECK(set1.nth(2)->first == 80); CHECK(set1.nth(2)->second == 2);

            if (set2.size() == 3)
            {
                // Elements were moved one-by-one
                CHECK(set2.nth(0)->first == -60); CHECK(set2.nth(0)->second == -2);
                CHECK(set2.nth(1)->first == -70); CHECK(set2.nth(1)->second == -2);
                CHECK(set2.nth(2)->first == -80); CHECK(set2.nth(2)->second == -2);
            }
            else
            {
                // Storage was moved
                CHECK(set2.size() == 0);
            }
        }
        #undef CONDITION
    }
    #undef N

    #define N 5
    {
        // Permutation 1/8
        #define CONDITION set2.capacity() == N  &&  set1.size() < set2.size()  &&  set1.size() < set2.capacity()  &&  set1.capacity() < set2.capacity()
        {
            // Impossible condition
        }
        #undef CONDITION

        // Permutation 2/8
        #define CONDITION set2.capacity() == N  &&  set1.size() < set2.size()  &&  set1.size() < set2.capacity()  &&  set1.capacity() > set2.capacity()
        {
            sfl::small_flat_multiset< xint_xint,
                                      N,
                                      std::less<xint_xint>,
                                      TPARAM_ALLOCATOR<xint_xint> > set1, set2;

            set1.reserve(100);
            set1.emplace(10, 1);
            set1.emplace(20, 1);
            set1.emplace(30, 1);

            CHECK(set1.size() == 3);
            CHECK(set1.nth(0)->first == 10); CHECK(set1.nth(0)->second == 1);
            CHECK(set1.nth(1)->first == 20); CHECK(set1.nth(1)->second == 1);
            CHECK(set1.nth(2)->first == 30); CHECK(set1.nth(2)->second == 1);

            set2.emplace(40, 2);
            set2.emplace(50, 2);
            set2.emplace(60, 2);
            set2.emplace(70, 2);
            set2.emplace(80, 2);

            CHECK(set2.size() == 5);
            CHECK(set2.nth(0)->first == 40); CHECK(set2.nth(0)->second == 2);
            CHECK(set2.nth(1)->first == 50); CHECK(set2.nth(1)->second == 2);
            CHECK(set2.nth(2)->first == 60); CHECK(set2.nth(2)->second == 2);
            CHECK(set2.nth(3)->first == 70); CHECK(set2.nth(3)->second == 2);
            CHECK(set2.nth(4)->first == 80); CHECK(set2.nth(4)->second == 2);

            ///////////////////////////////////////////////////////////////////////

            CHECK(CONDITION);

            set1 = std::move(set2);

            CHECK(set1.size() == 5);
            CHECK(set1.nth(0)->first == 40); CHECK(set1.nth(0)->second == 2);
            CHECK(set1.nth(1)->first == 50); CHECK(set1.nth(1)->second == 2);
            CHECK(set1.nth(2)->first == 60); CHECK(set1.nth(2)->second == 2);
            CHECK(set1.nth(3)->first == 70); CHECK(set1.nth(3)->second == 2);
            CHECK(set1.nth(4)->first == 80); CHECK(set1.nth(4)->second == 2);

            if (set2.size() == 5)
            {
                // Elements were moved one-by-one
                CHECK(set2.nth(0)->first == -40); CHECK(set2.nth(0)->second == -2);
                CHECK(set2.nth(1)->first == -50); CHECK(set2.nth(1)->second == -2);
                CHECK(set2.nth(2)->first == -60); CHECK(set2.nth(2)->second == -2);
                CHECK(set2.nth(3)->first == -70); CHECK(set2.nth(3)->second == -2);
                CHECK(set2.nth(4)->first == -80); CHECK(set2.nth(4)->second == -2);
            }
            else
            {
                // Storage was moved
                CHECK(set2.size() == 0);
            }
        }
        #undef CONDITION

        // Permutation 3/8
        #define CONDITION set2.capacity() == N  &&  set1.size() < set2.size()  &&  set1.size() > set2.capacity()  &&  set1.capacity() < set2.capacity()
        {
            // Impossible condition
        }
        #undef CONDITION

        // Permutation 4/8
        #define CONDITION set2.capacity() == N  &&  set1.size() < set2.size()  &&  set1.size() > set2.capacity()  &&  set1.capacity() > set2.capacity()
        {
            // Impossible condition
        }
        #undef CONDITION

        // Permutation 5/8
        #define CONDITION set2.capacity() == N  &&  set1.size() > set2.size()  &&  set1.size() < set2.capacity()  &&  set1.capacity() < set2.capacity()
        {
            // Impossible condition
        }
        #undef CONDITION

        // Permutation 6/8
        #define CONDITION set2.capacity() == N  &&  set1.size() > set2.size()  &&  set1.size() < set2.capacity()  &&  set1.capacity() > set2.capacity()
        {
            sfl::small_flat_multiset< xint_xint,
                                      N,
                                      std::less<xint_xint>,
                                      TPARAM_ALLOCATOR<xint_xint> > set1, set2;

            set1.reserve(100);
            set1.emplace(10, 1);
            set1.emplace(20, 1);
            set1.emplace(30, 1);
            set1.emplace(40, 1);

            CHECK(set1.size() == 4);
            CHECK(set1.nth(0)->first == 10); CHECK(set1.nth(0)->second == 1);
            CHECK(set1.nth(1)->first == 20); CHECK(set1.nth(1)->second == 1);
            CHECK(set1.nth(2)->first == 30); CHECK(set1.nth(2)->second == 1);
            CHECK(set1.nth(3)->first == 40); CHECK(set1.nth(3)->second == 1);

            set2.emplace(60, 2);
            set2.emplace(70, 2);
            set2.emplace(80, 2);

            CHECK(set2.size() == 3);
            CHECK(set2.nth(0)->first == 60); CHECK(set2.nth(0)->second == 2);
            CHECK(set2.nth(1)->first == 70); CHECK(set2.nth(1)->second == 2);
            CHECK(set2.nth(2)->first == 80); CHECK(set2.nth(2)->second == 2);

            ///////////////////////////////////////////////////////////////////////

            CHECK(CONDITION);

            set1 = std::move(set2);

            CHECK(set1.size() == 3);
            CHECK(set1.nth(0)->first == 60); CHECK(set1.nth(0)->second == 2);
            CHECK(set1.nth(1)->first == 70); CHECK(set1.nth(1)->second == 2);
            CHECK(set1.nth(2)->first == 80); CHECK(set1.nth(2)->second == 2);

            if (set2.size() == 3)
            {
                // Elements were moved one-by-one
                CHECK(set2.nth(0)->first == -60); CHECK(set2.nth(0)->second == -2);
                CHECK(set2.nth(1)->first == -70); CHECK(set2.nth(1)->second == -2);
                CHECK(set2.nth(2)->first == -80); CHECK(set2.nth(2)->second == -2);
            }
            else
            {
                // Storage was moved
                CHECK(set2.size() == 0);
            }
        }
        #undef CONDITION

        // Permutation 7/8
        #define CONDITION set2.capacity() == N  &&  set1.size() > set2.size()  &&  set1.size() > set2.capacity()  &&  set1.capacity() < set2.capacity()
        {
            // Impossible condition
        }
        #undef CONDITION

        // Permutation 8/8
        #define CONDITION set2.capacity() == N  &&  set1.size() > set2.size()  &&  set1.size() > set2.capacity()  &&  set1.capacity() > set2.capacity()
        {
            sfl::small_flat_multiset< xint_xint,
                                      N,
                                      std::less<xint_xint>,
                                      TPARAM_ALLOCATOR<xint_xint> > set1, set2;

            set1.reserve(100);
            set1.emplace(10, 1);
            set1.emplace(20, 1);
            set1.emplace(30, 1);
            set1.emplace(40, 1);
            set1.emplace(50, 1);
            set1.emplace(60, 1);

            CHECK(set1.size() == 6);
            CHECK(set1.nth(0)->first == 10); CHECK(set1.nth(0)->second == 1);
            CHECK(set1.nth(1)->first == 20); CHECK(set1.nth(1)->second == 1);
            CHECK(set1.nth(2)->first == 30); CHECK(set1.nth(2)->second == 1);
            CHECK(set1.nth(3)->first == 40); CHECK(set1.nth(3)->second == 1);
            CHECK(set1.nth(4)->first == 50); CHECK(set1.nth(4)->second == 1);
            CHECK(set1.nth(5)->first == 60); CHECK(set1.nth(5)->second == 1);

            set2.emplace(60, 2);
            set2.emplace(70, 2);
            set2.emplace(80, 2);

            CHECK(set2.size() == 3);
            CHECK(set2.nth(0)->first == 60); CHECK(set2.nth(0)->second == 2);
            CHECK(set2.nth(1)->first == 70); CHECK(set2.nth(1)->second == 2);
            CHECK(set2.nth(2)->first == 80); CHECK(set2.nth(2)->second == 2);

            ///////////////////////////////////////////////////////////////////////

            CHECK(CONDITION);

            set1 = std::move(set2);

            CHECK(set1.size() == 3);
            CHECK(set1.nth(0)->first == 60); CHECK(set1.nth(0)->second == 2);
            CHECK(set1.nth(1)->first == 70); CHECK(set1.nth(1)->second == 2);
            CHECK(set1.nth(2)->first == 80); CHECK(set1.nth(2)->second == 2);

            if (set2.size() == 3)
            {
                // Elements were moved one-by-one
                CHECK(set2.nth(0)->first == -60); CHECK(set2.nth(0)->second == -2);
                CHECK(set2.nth(1)->first == -70); CHECK(set2.nth(1)->second == -2);
                CHECK(set2.nth(2)->first == -80); CHECK(set2.nth(2)->second == -2);
            }
            else
            {
                // Storage was moved
                CHECK(set2.size() == 0);
            }
        }
        #undef CONDITION
    }
    #undef N
}

PRINT("Test operator=(std::initializer_list)");
{
    // Permutation 1/4
    #define CONDITION ilist.size() < set.size() && ilist.size() < set.capacity()
    {
        sfl::small_flat_multiset< xint_xint,
                                  5,
                                  std::less<xint_xint>,
                                  TPARAM_ALLOCATOR<xint_xint> > set;

        set.emplace(10, 1);
        set.emplace(20, 1);
        set.emplace(30, 1);
        set.emplace(40, 1);
        set.emplace(50, 1);

        CHECK(set.size() == 5);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
        CHECK(set.nth(3)->first == 40); CHECK(set.nth(3)->second == 1);
        CHECK(set.nth(4)->first == 50); CHECK(set.nth(4)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        std::initializer_list<xint_xint> ilist
        {
            {60, 2},
            {70, 2},
            {80, 2}
        };

        ///////////////////////////////////////////////////////////////////////

        CHECK(CONDITION);

        set = ilist;

        CHECK(set.size() == 3);
        CHECK(set.nth(0)->first == 60); CHECK(set.nth(0)->second == 2);
        CHECK(set.nth(1)->first == 70); CHECK(set.nth(1)->second == 2);
        CHECK(set.nth(2)->first == 80); CHECK(set.nth(2)->second == 2);
    }
    #undef CONDITION

    // Permutation 2/4
    #define CONDITION ilist.size() < set.size() && ilist.size() > set.capacity()
    {
        // Impossible condition
    }
    #undef CONDITION

    // Permutation 3/4
    #define CONDITION ilist.size() > set.size() && ilist.size() < set.capacity()
    {
        sfl::small_flat_multiset< xint_xint,
                                  100,
                                  std::less<xint_xint>,
                                  TPARAM_ALLOCATOR<xint_xint> > set;

        set.emplace(10, 1);
        set.emplace(20, 1);
        set.emplace(30, 1);

        CHECK(set.size() == 3);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        std::initializer_list<xint_xint> ilist
        {
            {40, 2},
            {50, 2},
            {60, 2},
            {70, 2},
            {80, 2}
        };

        ///////////////////////////////////////////////////////////////////////

        CHECK(CONDITION);

        set = ilist;

        CHECK(set.size() == 5);
        CHECK(set.nth(0)->first == 40); CHECK(set.nth(0)->second == 2);
        CHECK(set.nth(1)->first == 50); CHECK(set.nth(1)->second == 2);
        CHECK(set.nth(2)->first == 60); CHECK(set.nth(2)->second == 2);
        CHECK(set.nth(3)->first == 70); CHECK(set.nth(3)->second == 2);
        CHECK(set.nth(4)->first == 80); CHECK(set.nth(4)->second == 2);
    }
    #undef CONDITION

    // Permutation 4/4
    #define CONDITION ilist.size() > set.size() && ilist.size() > set.capacity()
    {
        sfl::small_flat_multiset< xint_xint,
                                  3,
                                  std::less<xint_xint>,
                                  TPARAM_ALLOCATOR<xint_xint> > set;

        set.emplace(10, 1);
        set.emplace(20, 1);
        set.emplace(30, 1);

        CHECK(set.size() == 3);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        std::initializer_list<xint_xint> ilist
        {
            {40, 2},
            {50, 2},
            {60, 2},
            {70, 2},
            {80, 2}
        };

        ///////////////////////////////////////////////////////////////////////

        CHECK(CONDITION);

        set = ilist;

        CHECK(set.size() == 5);
        CHECK(set.nth(0)->first == 40); CHECK(set.nth(0)->second == 2);
        CHECK(set.nth(1)->first == 50); CHECK(set.nth(1)->second == 2);
        CHECK(set.nth(2)->first == 60); CHECK(set.nth(2)->second == 2);
        CHECK(set.nth(3)->first == 70); CHECK(set.nth(3)->second == 2);
        CHECK(set.nth(4)->first == 80); CHECK(set.nth(4)->second == 2);
    }
    #undef CONDITION
}

///////////////////////////////////////////////////////////////////////////////

PRINT("Test NON-MEMBER comparison operators");
{
    sfl::small_flat_multiset< xint_xint,
                              100,
                              std::less<xint_xint>,
                              TPARAM_ALLOCATOR<xint_xint> > set1, set2;

    set1.emplace(10, 1);
    set1.emplace(20, 1);
    set1.emplace(30, 1);

    set2.emplace(10, 1);
    set2.emplace(20, 1);
    set2.emplace(30, 1);
    set2.emplace(40, 1);
    set2.emplace(50, 1);

    CHECK((set1 == set1) == true);
    CHECK((set1 == set2) == false);
    CHECK((set2 == set1) == false);
    CHECK((set2 == set2) == true);

    CHECK((set1 != set1) == false);
    CHECK((set1 != set2) == true);
    CHECK((set2 != set1) == true);
    CHECK((set2 != set2) == false);

    CHECK((set1 < set1) == false);
    CHECK((set1 < set2) == true);
    CHECK((set2 < set1) == false);
    CHECK((set2 < set2) == false);

    CHECK((set1 > set1) == false);
    CHECK((set1 > set2) == false);
    CHECK((set2 > set1) == true);
    CHECK((set2 > set2) == false);

    CHECK((set1 <= set1) == true);
    CHECK((set1 <= set2) == true);
    CHECK((set2 <= set1) == false);
    CHECK((set2 <= set2) == true);

    CHECK((set1 >= set1) == true);
    CHECK((set1 >= set2) == false);
    CHECK((set2 >= set1) == true);
    CHECK((set2 >= set2) == true);
}

PRINT("Test NON-MEMBER swap(container&)");
{
    sfl::small_flat_multiset< xint_xint,
                              100,
                              std::less<xint_xint>,
                              TPARAM_ALLOCATOR<xint_xint> > set1, set2;

    set1.emplace(10, 1);
    set1.emplace(20, 1);
    set1.emplace(30, 1);

    set2.emplace(40, 2);
    set2.emplace(50, 2);
    set2.emplace(60, 2);
    set2.emplace(70, 2);
    set2.emplace(80, 2);

    CHECK(set1.size() == 3);
    CHECK(set1.nth(0)->first == 10); CHECK(set1.nth(0)->second == 1);
    CHECK(set1.nth(1)->first == 20); CHECK(set1.nth(1)->second == 1);
    CHECK(set1.nth(2)->first == 30); CHECK(set1.nth(2)->second == 1);

    CHECK(set2.size() == 5);
    CHECK(set2.nth(0)->first == 40); CHECK(set2.nth(0)->second == 2);
    CHECK(set2.nth(1)->first == 50); CHECK(set2.nth(1)->second == 2);
    CHECK(set2.nth(2)->first == 60); CHECK(set2.nth(2)->second == 2);
    CHECK(set2.nth(3)->first == 70); CHECK(set2.nth(3)->second == 2);
    CHECK(set2.nth(4)->first == 80); CHECK(set2.nth(4)->second == 2);

    ///////////////////////////////////////////////////////////////////////////

    swap(set1, set2);

    CHECK(set1.size() == 5);
    CHECK(set1.nth(0)->first == 40); CHECK(set1.nth(0)->second == 2);
    CHECK(set1.nth(1)->first == 50); CHECK(set1.nth(1)->second == 2);
    CHECK(set1.nth(2)->first == 60); CHECK(set1.nth(2)->second == 2);
    CHECK(set1.nth(3)->first == 70); CHECK(set1.nth(3)->second == 2);
    CHECK(set1.nth(4)->first == 80); CHECK(set1.nth(4)->second == 2);

    CHECK(set2.size() == 3);
    CHECK(set2.nth(0)->first == 10); CHECK(set2.nth(0)->second == 1);
    CHECK(set2.nth(1)->first == 20); CHECK(set2.nth(1)->second == 1);
    CHECK(set2.nth(2)->first == 30); CHECK(set2.nth(2)->second == 1);
}

PRINT("Test NON-MEMBER erase_if(container&, Predicate)");
{
    using container_type =
        sfl::small_flat_multiset< xint_xint,
                                  100,
                                  std::less<xint_xint>,
                                  TPARAM_ALLOCATOR<xint_xint> >;

    using const_reference = typename container_type::const_reference;

    ///////////////////////////////////////////////////////////////////////////

    container_type set;

    set.emplace(10, 1);
    set.emplace(20, 1);
    set.emplace(20, 2);
    set.emplace(20, 3);
    set.emplace(30, 1);

    CHECK(set.size() == 5);
    CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
    CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 3);
    CHECK(set.nth(2)->first == 20); CHECK(set.nth(2)->second == 2);
    CHECK(set.nth(3)->first == 20); CHECK(set.nth(3)->second == 1);
    CHECK(set.nth(4)->first == 30); CHECK(set.nth(4)->second == 1);

    ///////////////////////////////////////////////////////////////////////////

    CHECK(erase_if(set, [](const_reference& value){ return value.first == 20; }) == 3);
    CHECK(set.size() == 2);
    CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
    CHECK(set.nth(1)->first == 30); CHECK(set.nth(1)->second == 1);

    ///////////////////////////////////////////////////////////////////////////

    CHECK(erase_if(set, [](const_reference& value){ return value.first == 20; }) == 0);
    CHECK(set.size() == 2);
    CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
    CHECK(set.nth(1)->first == 30); CHECK(set.nth(1)->second == 1);
}
