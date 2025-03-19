#ifndef TPARAM_ALLOCATOR
#error Please define macro TPARAM_ALLOCATOR
#endif

using sfl::test::xint;
using sfl::test::xobj;

PRINT("Test PRIVATE member function emplace_back(Args&&...)");
{
    // N == 0
    {
        sfl::small_unordered_flat_multimap< xint,
                                            xint,
                                            0,
                                            std::equal_to<xint>,
                                            TPARAM_ALLOCATOR<std::pair<xint, xint>> > map;

        {
            PRINT(">");
            auto res = map.emplace_back(10, 1);
            PRINT("<");

            CHECK(res == map.nth(0));
            CHECK(map.size() == 1);
            CHECK(map.capacity() == 1);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
        }

        {
            PRINT(">");
            auto res = map.emplace_back(20, 1);
            PRINT("<");

            CHECK(res == map.nth(1));
            CHECK(map.size() == 2);
            CHECK(map.capacity() == 2);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
        }

        {
            PRINT(">");
            auto res = map.emplace_back(30, 1);
            PRINT("<");

            CHECK(res == map.nth(2));
            CHECK(map.size() == 3);
            CHECK(map.capacity() == 3);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
        }

        {
            PRINT(">");
            auto res = map.emplace_back(40, 1);
            PRINT("<");

            CHECK(res == map.nth(3));
            CHECK(map.size() == 4);
            CHECK(map.capacity() == 4);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
            CHECK(map.nth(3)->first == 40); CHECK(map.nth(3)->second == 1);
        }

        {
            PRINT(">");
            auto res = map.emplace_back(50, 1);
            PRINT("<");

            CHECK(res == map.nth(4));
            CHECK(map.size() == 5);
            CHECK(map.capacity() == 6);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
            CHECK(map.nth(3)->first == 40); CHECK(map.nth(3)->second == 1);
            CHECK(map.nth(4)->first == 50); CHECK(map.nth(4)->second == 1);
        }

        {
            PRINT(">");
            auto res = map.emplace_back(60, 1);
            PRINT("<");

            CHECK(res == map.nth(5));
            CHECK(map.size() == 6);
            CHECK(map.capacity() == 6);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
            CHECK(map.nth(3)->first == 40); CHECK(map.nth(3)->second == 1);
            CHECK(map.nth(4)->first == 50); CHECK(map.nth(4)->second == 1);
            CHECK(map.nth(5)->first == 60); CHECK(map.nth(5)->second == 1);
        }
    }

    // N == 3
    {
        sfl::small_unordered_flat_multimap< xint,
                                            xint,
                                            3,
                                            std::equal_to<xint>,
                                            TPARAM_ALLOCATOR<std::pair<xint, xint>> > map;

        {
            PRINT(">");
            auto res = map.emplace_back(10, 1);
            PRINT("<");

            CHECK(res == map.nth(0));
            CHECK(map.size() == 1);
            CHECK(map.capacity() == 3);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
        }

        {
            PRINT(">");
            auto res = map.emplace_back(20, 1);
            PRINT("<");

            CHECK(res == map.nth(1));
            CHECK(map.size() == 2);
            CHECK(map.capacity() == 3);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
        }

        {
            PRINT(">");
            auto res = map.emplace_back(30, 1);
            PRINT("<");

            CHECK(res == map.nth(2));
            CHECK(map.size() == 3);
            CHECK(map.capacity() == 3);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
        }

        {
            PRINT(">");
            auto res = map.emplace_back(40, 1);
            PRINT("<");

            CHECK(res == map.nth(3));
            CHECK(map.size() == 4);
            CHECK(map.capacity() == 4);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
            CHECK(map.nth(3)->first == 40); CHECK(map.nth(3)->second == 1);
        }

        {
            PRINT(">");
            auto res = map.emplace_back(50, 1);
            PRINT("<");

            CHECK(res == map.nth(4));
            CHECK(map.size() == 5);
            CHECK(map.capacity() == 6);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
            CHECK(map.nth(3)->first == 40); CHECK(map.nth(3)->second == 1);
            CHECK(map.nth(4)->first == 50); CHECK(map.nth(4)->second == 1);
        }

        {
            PRINT(">");
            auto res = map.emplace_back(60, 1);
            PRINT("<");

            CHECK(res == map.nth(5));
            CHECK(map.size() == 6);
            CHECK(map.capacity() == 6);
            CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
            CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
            CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
            CHECK(map.nth(3)->first == 40); CHECK(map.nth(3)->second == 1);
            CHECK(map.nth(4)->first == 50); CHECK(map.nth(4)->second == 1);
            CHECK(map.nth(5)->first == 60); CHECK(map.nth(5)->second == 1);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////

PRINT("Test begin, end, cbegin, cend, nth, index_of");
{
    sfl::small_unordered_flat_multimap< xint,
                                        xint,
                                        100,
                                        std::equal_to<xint>,
                                        TPARAM_ALLOCATOR<std::pair<xint, xint>> > map;

    map.emplace_back(20, 1);
    map.emplace_back(40, 1);
    map.emplace_back(60, 1);

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

///////////////////////////////////////////////////////////////////////////////

PRINT("Test static_capacity");
{
    CHECK((sfl::small_unordered_flat_multimap< xint,
                                               xint,
                                               100,
                                               std::equal_to<xint>,
                                               TPARAM_ALLOCATOR<std::pair<xint, xint>> >::static_capacity == 100));
}

///////////////////////////////////////////////////////////////////////////////

PRINT("Test get_allocator()");
{
    sfl::small_unordered_flat_multimap< xint,
                                        xint,
                                        100,
                                        std::equal_to<xint>,
                                        TPARAM_ALLOCATOR<std::pair<xint, xint>> > map;

    auto alloc = map.get_allocator();

    (void)alloc;
}

///////////////////////////////////////////////////////////////////////////////

PRINT("Test key_eq()");
{
    {
        sfl::small_unordered_flat_multimap< xint,
                                            xint,
                                            100,
                                            std::equal_to<xint>,
                                            TPARAM_ALLOCATOR<std::pair<xint, xint>> > map;

        auto key_eq = map.key_eq();

        CHECK(key_eq(10, 10) == true);
        CHECK(key_eq(10, 20) == false);
        CHECK(key_eq(20, 10) == false);
        CHECK(key_eq(20, 20) == true);
    }

    {
        sfl::small_unordered_flat_multimap< xobj,
                                            xint,
                                            100,
                                            xobj::equal,
                                            TPARAM_ALLOCATOR<std::pair<xobj, xint>> > map;

        auto key_eq = map.key_eq();

        CHECK(key_eq(xobj(10), 10) == true);
        CHECK(key_eq(xobj(10), 20) == false);
        CHECK(key_eq(xobj(20), 10) == false);
        CHECK(key_eq(xobj(20), 20) == true);
    }
}

///////////////////////////////////////////////////////////////////////////////

PRINT("Test value_eq()");
{
    {
        sfl::small_unordered_flat_multimap< xint,
                                            xint,
                                            100,
                                            std::equal_to<xint>,
                                            TPARAM_ALLOCATOR<std::pair<xint, xint>> > map;

        auto value_eq = map.value_eq();

        CHECK(value_eq({10, 1}, {10, 2}) == true);
        CHECK(value_eq({10, 1}, {20, 2}) == false);
        CHECK(value_eq({20, 1}, {10, 2}) == false);
        CHECK(value_eq({20, 1}, {20, 2}) == true);
    }

    {
        sfl::small_unordered_flat_multimap< xobj,
                                            xint,
                                            100,
                                            xobj::equal,
                                            TPARAM_ALLOCATOR<std::pair<xobj, xint>> > map;

        auto value_eq = map.value_eq();

        CHECK(value_eq({xobj(10), 1}, {xobj(10), 2}) == true);
        CHECK(value_eq({xobj(10), 1}, {xobj(20), 2}) == false);
        CHECK(value_eq({xobj(20), 1}, {xobj(10), 2}) == false);
        CHECK(value_eq({xobj(20), 1}, {xobj(20), 2}) == true);
    }
}

///////////////////////////////////////////////////////////////////////////////

PRINT("Test find, count, contains");
{
    // xint, xint
    {
        sfl::small_unordered_flat_multimap< xint,
                                            xint,
                                            100,
                                            std::equal_to<xint>,
                                            TPARAM_ALLOCATOR<std::pair<xint, xint>> > map;

        map.emplace_back(20, 1);
        map.emplace_back(40, 1);
        map.emplace_back(60, 1);

        CHECK(map.size() == 3);
        CHECK(map.nth(0)->first == 20); CHECK(map.nth(0)->second == 1);
        CHECK(map.nth(1)->first == 40); CHECK(map.nth(1)->second == 1);
        CHECK(map.nth(2)->first == 60); CHECK(map.nth(2)->second == 1);

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
        sfl::small_unordered_flat_multimap< xobj,
                                            xint,
                                            100,
                                            xobj::equal,
                                            TPARAM_ALLOCATOR<std::pair<xobj, xint>> > map;

        map.emplace_back(std::piecewise_construct, std::forward_as_tuple(20), std::forward_as_tuple(1));
        map.emplace_back(std::piecewise_construct, std::forward_as_tuple(40), std::forward_as_tuple(1));
        map.emplace_back(std::piecewise_construct, std::forward_as_tuple(60), std::forward_as_tuple(1));

        CHECK(map.size() == 3);
        CHECK(map.nth(0)->first.value() == 20); CHECK(map.nth(0)->second == 1);
        CHECK(map.nth(1)->first.value() == 40); CHECK(map.nth(1)->second == 1);
        CHECK(map.nth(2)->first.value() == 60); CHECK(map.nth(2)->second == 1);

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
    sfl::small_unordered_flat_multimap< xint,
                                        xint,
                                        100,
                                        std::equal_to<xint>,
                                        TPARAM_ALLOCATOR<std::pair<xint, xint>> > map;

    CHECK(map.size() == 0);

    map.emplace_back(10, 1);
    map.emplace_back(20, 1);
    map.emplace_back(30, 1);

    CHECK(map.size() == 3);
    CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
    CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
    CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);

    map.clear();

    CHECK(map.size() == 0);

    map.emplace_back(40, 2);
    map.emplace_back(50, 2);
    map.emplace_back(60, 2);

    CHECK(map.size() == 3);
    CHECK(map.nth(0)->first == 40); CHECK(map.nth(0)->second == 2);
    CHECK(map.nth(1)->first == 50); CHECK(map.nth(1)->second == 2);
    CHECK(map.nth(2)->first == 60); CHECK(map.nth(2)->second == 2);

    map.clear();

    CHECK(map.size() == 0);
}

PRINT("Test emplace(Args&&...)");
{
    sfl::small_unordered_flat_multimap< xint,
                                        xint,
                                        100,
                                        std::equal_to<xint>,
                                        TPARAM_ALLOCATOR<std::pair<xint, xint>> > map;

    {
        CHECK(map.emplace(10, 1) == map.nth(0));
        CHECK(map.emplace(20, 1) == map.nth(1));
        CHECK(map.emplace(30, 1) == map.nth(2));

        CHECK(map.size() == 3);
        CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
        CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
        CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
    }

    {
        CHECK(map.emplace(10, 2) == map.nth(3));
        CHECK(map.emplace(20, 2) == map.nth(4));
        CHECK(map.emplace(30, 2) == map.nth(5));

        CHECK(map.size() == 6);
        CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
        CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
        CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
        CHECK(map.nth(3)->first == 10); CHECK(map.nth(3)->second == 2);
        CHECK(map.nth(4)->first == 20); CHECK(map.nth(4)->second == 2);
        CHECK(map.nth(5)->first == 30); CHECK(map.nth(5)->second == 2);
    }
}

PRINT("Test emplace_hint(const_iterator, Args&&...)");
{
    sfl::small_unordered_flat_multimap< xint,
                                        xint,
                                        100,
                                        std::equal_to<xint>,
                                        TPARAM_ALLOCATOR<std::pair<xint, xint>> > map;

    {
        CHECK(map.emplace_hint(map.begin(), 10, 1) == map.nth(0));
        CHECK(map.emplace_hint(map.begin(), 20, 1) == map.nth(1));
        CHECK(map.emplace_hint(map.begin(), 30, 1) == map.nth(2));

        CHECK(map.size() == 3);
        CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
        CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
        CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
    }

    {
        CHECK(map.emplace_hint(map.begin(), 10, 2) == map.nth(3));
        CHECK(map.emplace_hint(map.begin(), 20, 2) == map.nth(4));
        CHECK(map.emplace_hint(map.begin(), 30, 2) == map.nth(5));

        CHECK(map.size() == 6);
        CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
        CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
        CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
        CHECK(map.nth(3)->first == 10); CHECK(map.nth(3)->second == 2);
        CHECK(map.nth(4)->first == 20); CHECK(map.nth(4)->second == 2);
        CHECK(map.nth(5)->first == 30); CHECK(map.nth(5)->second == 2);
    }
}

PRINT("Test insert(const value_type&)");
{
    sfl::small_unordered_flat_multimap< xint,
                                        xint,
                                        100,
                                        std::equal_to<xint>,
                                        TPARAM_ALLOCATOR<std::pair<xint, xint>> > map;

    using value_type = std::pair<xint, xint>;

    {
        value_type value_10_1(10, 1);
        value_type value_20_1(20, 1);
        value_type value_30_1(30, 1);

        CHECK(map.insert(value_10_1) == map.nth(0));
        CHECK(map.insert(value_20_1) == map.nth(1));
        CHECK(map.insert(value_30_1) == map.nth(2));

        CHECK(map.size() == 3);
        CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
        CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
        CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);

        CHECK(value_10_1.first == 10); CHECK(value_10_1.second == 1);
        CHECK(value_20_1.first == 20); CHECK(value_20_1.second == 1);
        CHECK(value_30_1.first == 30); CHECK(value_30_1.second == 1);
    }

    {
        value_type value_10_2(10, 2);
        value_type value_20_2(20, 2);
        value_type value_30_2(30, 2);

        CHECK(map.insert(value_10_2) == map.nth(3));
        CHECK(map.insert(value_20_2) == map.nth(4));
        CHECK(map.insert(value_30_2) == map.nth(5));

        CHECK(map.size() == 6);
        CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
        CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
        CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
        CHECK(map.nth(3)->first == 10); CHECK(map.nth(3)->second == 2);
        CHECK(map.nth(4)->first == 20); CHECK(map.nth(4)->second == 2);
        CHECK(map.nth(5)->first == 30); CHECK(map.nth(5)->second == 2);

        CHECK(value_10_2.first == 10); CHECK(value_10_2.second == 2);
        CHECK(value_20_2.first == 20); CHECK(value_20_2.second == 2);
        CHECK(value_30_2.first == 30); CHECK(value_30_2.second == 2);
    }
}

PRINT("Test insert(value_type&&)");
{
    sfl::small_unordered_flat_multimap< xint,
                                        xint,
                                        100,
                                        std::equal_to<xint>,
                                        TPARAM_ALLOCATOR<std::pair<xint, xint>> > map;

    using value_type = std::pair<xint, xint>;

    {
        value_type value_10_1(10, 1);
        value_type value_20_1(20, 1);
        value_type value_30_1(30, 1);

        CHECK(map.insert(std::move(value_10_1)) == map.nth(0));
        CHECK(map.insert(std::move(value_20_1)) == map.nth(1));
        CHECK(map.insert(std::move(value_30_1)) == map.nth(2));

        CHECK(map.size() == 3);
        CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
        CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
        CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);

        CHECK(value_10_1.first == -10); CHECK(value_10_1.second == -1);
        CHECK(value_20_1.first == -20); CHECK(value_20_1.second == -1);
        CHECK(value_30_1.first == -30); CHECK(value_30_1.second == -1);
    }

    {
        value_type value_10_2(10, 2);
        value_type value_20_2(20, 2);
        value_type value_30_2(30, 2);

        CHECK(map.insert(std::move(value_10_2)) == map.nth(3));
        CHECK(map.insert(std::move(value_20_2)) == map.nth(4));
        CHECK(map.insert(std::move(value_30_2)) == map.nth(5));

        CHECK(map.size() == 6);
        CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
        CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
        CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
        CHECK(map.nth(3)->first == 10); CHECK(map.nth(3)->second == 2);
        CHECK(map.nth(4)->first == 20); CHECK(map.nth(4)->second == 2);
        CHECK(map.nth(5)->first == 30); CHECK(map.nth(5)->second == 2);

        CHECK(value_10_2.first == -10); CHECK(value_10_2.second == -2);
        CHECK(value_20_2.first == -20); CHECK(value_20_2.second == -2);
        CHECK(value_30_2.first == -30); CHECK(value_30_2.second == -2);
    }
}

PRINT("Test insert(P&&)");
{
    sfl::small_unordered_flat_multimap< xint,
                                        xint,
                                        100,
                                        std::equal_to<xint>,
                                        TPARAM_ALLOCATOR<std::pair<xint, xint>> > map;

    using value_type = std::pair<long, long>;

    {
        value_type value_10_1(10, 1);
        value_type value_20_1(20, 1);
        value_type value_30_1(30, 1);

        CHECK(map.insert(std::move(value_10_1)) == map.nth(0));
        CHECK(map.insert(std::move(value_20_1)) == map.nth(1));
        CHECK(map.insert(std::move(value_30_1)) == map.nth(2));

        CHECK(map.size() == 3);
        CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
        CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
        CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);

        CHECK(value_10_1.first == 10); CHECK(value_10_1.second == 1);
        CHECK(value_20_1.first == 20); CHECK(value_20_1.second == 1);
        CHECK(value_30_1.first == 30); CHECK(value_30_1.second == 1);
    }

    {
        value_type value_10_2(10, 2);
        value_type value_20_2(20, 2);
        value_type value_30_2(30, 2);

        CHECK(map.insert(std::move(value_10_2)) == map.nth(3));
        CHECK(map.insert(std::move(value_20_2)) == map.nth(4));
        CHECK(map.insert(std::move(value_30_2)) == map.nth(5));

        CHECK(map.size() == 6);
        CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
        CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
        CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
        CHECK(map.nth(3)->first == 10); CHECK(map.nth(3)->second == 2);
        CHECK(map.nth(4)->first == 20); CHECK(map.nth(4)->second == 2);
        CHECK(map.nth(5)->first == 30); CHECK(map.nth(5)->second == 2);

        CHECK(value_10_2.first == 10); CHECK(value_10_2.second == 2);
        CHECK(value_20_2.first == 20); CHECK(value_20_2.second == 2);
        CHECK(value_30_2.first == 30); CHECK(value_30_2.second == 2);
    }
}

PRINT("Test insert(const_iterator, const value_type&)");
{
    sfl::small_unordered_flat_multimap< xint,
                                        xint,
                                        100,
                                        std::equal_to<xint>,
                                        TPARAM_ALLOCATOR<std::pair<xint, xint>> > map;

    using value_type = std::pair<xint, xint>;

    {
        value_type value_10_1(10, 1);
        value_type value_20_1(20, 1);
        value_type value_30_1(30, 1);

        CHECK(map.insert(map.begin(), value_10_1) == map.nth(0));
        CHECK(map.insert(map.begin(), value_20_1) == map.nth(1));
        CHECK(map.insert(map.begin(), value_30_1) == map.nth(2));

        CHECK(map.size() == 3);
        CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
        CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
        CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);

        CHECK(value_10_1.first == 10); CHECK(value_10_1.second == 1);
        CHECK(value_20_1.first == 20); CHECK(value_20_1.second == 1);
        CHECK(value_30_1.first == 30); CHECK(value_30_1.second == 1);
    }

    {
        value_type value_10_2(10, 2);
        value_type value_20_2(20, 2);
        value_type value_30_2(30, 2);

        CHECK(map.insert(map.begin(), value_10_2) == map.nth(3));
        CHECK(map.insert(map.begin(), value_20_2) == map.nth(4));
        CHECK(map.insert(map.begin(), value_30_2) == map.nth(5));

        CHECK(map.size() == 6);
        CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
        CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
        CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
        CHECK(map.nth(3)->first == 10); CHECK(map.nth(3)->second == 2);
        CHECK(map.nth(4)->first == 20); CHECK(map.nth(4)->second == 2);
        CHECK(map.nth(5)->first == 30); CHECK(map.nth(5)->second == 2);

        CHECK(value_10_2.first == 10); CHECK(value_10_2.second == 2);
        CHECK(value_20_2.first == 20); CHECK(value_20_2.second == 2);
        CHECK(value_30_2.first == 30); CHECK(value_30_2.second == 2);
    }
}

PRINT("Test insert(const_iterator, value_type&&)");
{
    sfl::small_unordered_flat_multimap< xint,
                                        xint,
                                        100,
                                        std::equal_to<xint>,
                                        TPARAM_ALLOCATOR<std::pair<xint, xint>> > map;

    using value_type = std::pair<xint, xint>;

    {
        value_type value_10_1(10, 1);
        value_type value_20_1(20, 1);
        value_type value_30_1(30, 1);

        CHECK(map.insert(map.begin(), std::move(value_10_1)) == map.nth(0));
        CHECK(map.insert(map.begin(), std::move(value_20_1)) == map.nth(1));
        CHECK(map.insert(map.begin(), std::move(value_30_1)) == map.nth(2));

        CHECK(map.size() == 3);
        CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
        CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
        CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);

        CHECK(value_10_1.first == -10); CHECK(value_10_1.second == -1);
        CHECK(value_20_1.first == -20); CHECK(value_20_1.second == -1);
        CHECK(value_30_1.first == -30); CHECK(value_30_1.second == -1);
    }

    {
        value_type value_10_2(10, 2);
        value_type value_20_2(20, 2);
        value_type value_30_2(30, 2);

        CHECK(map.insert(map.begin(), std::move(value_10_2)) == map.nth(3));
        CHECK(map.insert(map.begin(), std::move(value_20_2)) == map.nth(4));
        CHECK(map.insert(map.begin(), std::move(value_30_2)) == map.nth(5));

        CHECK(map.size() == 6);
        CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
        CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
        CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
        CHECK(map.nth(3)->first == 10); CHECK(map.nth(3)->second == 2);
        CHECK(map.nth(4)->first == 20); CHECK(map.nth(4)->second == 2);
        CHECK(map.nth(5)->first == 30); CHECK(map.nth(5)->second == 2);

        CHECK(value_10_2.first == -10); CHECK(value_10_2.second == -2);
        CHECK(value_20_2.first == -20); CHECK(value_20_2.second == -2);
        CHECK(value_30_2.first == -30); CHECK(value_30_2.second == -2);
    }
}

PRINT("Test insert(const_iterator, P&&)");
{
    sfl::small_unordered_flat_multimap< xint,
                                        xint,
                                        100,
                                        std::equal_to<xint>,
                                        TPARAM_ALLOCATOR<std::pair<xint, xint>> > map;

    using value_type = std::pair<long, long>;

    {
        value_type value_10_1(10, 1);
        value_type value_20_1(20, 1);
        value_type value_30_1(30, 1);

        CHECK(map.insert(map.begin(), std::move(value_10_1)) == map.nth(0));
        CHECK(map.insert(map.begin(), std::move(value_20_1)) == map.nth(1));
        CHECK(map.insert(map.begin(), std::move(value_30_1)) == map.nth(2));

        CHECK(map.size() == 3);
        CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
        CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
        CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);

        CHECK(value_10_1.first == 10); CHECK(value_10_1.second == 1);
        CHECK(value_20_1.first == 20); CHECK(value_20_1.second == 1);
        CHECK(value_30_1.first == 30); CHECK(value_30_1.second == 1);
    }

    {
        value_type value_10_2(10, 2);
        value_type value_20_2(20, 2);
        value_type value_30_2(30, 2);

        CHECK(map.insert(map.begin(), std::move(value_10_2)) == map.nth(3));
        CHECK(map.insert(map.begin(), std::move(value_20_2)) == map.nth(4));
        CHECK(map.insert(map.begin(), std::move(value_30_2)) == map.nth(5));

        CHECK(map.size() == 6);
        CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
        CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
        CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
        CHECK(map.nth(3)->first == 10); CHECK(map.nth(3)->second == 2);
        CHECK(map.nth(4)->first == 20); CHECK(map.nth(4)->second == 2);
        CHECK(map.nth(5)->first == 30); CHECK(map.nth(5)->second == 2);

        CHECK(value_10_2.first == 10); CHECK(value_10_2.second == 2);
        CHECK(value_20_2.first == 20); CHECK(value_20_2.second == 2);
        CHECK(value_30_2.first == 30); CHECK(value_30_2.second == 2);
    }
}

PRINT("Test insert(InputIt, InputIt)");
{
    sfl::small_unordered_flat_multimap< xint,
                                        xint,
                                        100,
                                        std::equal_to<xint>,
                                        TPARAM_ALLOCATOR<std::pair<xint, xint>> > map;

    {
        std::vector<std::pair<xint, xint>> data
        (
            {
                {10, 1},
                {20, 1},
                {30, 1}
            }
        );

        map.insert(data.begin(), data.end());

        CHECK(map.size() == 3);
        CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
        CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
        CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
    }

    {
        std::vector<std::pair<xint, xint>> data
        (
            {
                {10, 2},
                {20, 2},
                {30, 2}
            }
        );

        map.insert(data.begin(), data.end());

        CHECK(map.size() == 6);
        CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
        CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
        CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
        CHECK(map.nth(3)->first == 10); CHECK(map.nth(3)->second == 2);
        CHECK(map.nth(4)->first == 20); CHECK(map.nth(4)->second == 2);
        CHECK(map.nth(5)->first == 30); CHECK(map.nth(5)->second == 2);
    }
}

PRINT("Test insert(std::initializer_list)");
{
    sfl::small_unordered_flat_multimap< xint,
                                        xint,
                                        100,
                                        std::equal_to<xint>,
                                        TPARAM_ALLOCATOR<std::pair<xint, xint>> > map;

    {
        std::initializer_list<std::pair<xint, xint>> ilist
        {
            {10, 1},
            {20, 1},
            {30, 1}
        };

        map.insert(ilist);

        CHECK(map.size() == 3);
        CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
        CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
        CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
    }

    {
        std::initializer_list<std::pair<xint, xint>> ilist
        {
            {10, 2},
            {20, 2},
            {30, 2}
        };

        map.insert(ilist);

        CHECK(map.size() == 6);
        CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
        CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
        CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
        CHECK(map.nth(3)->first == 10); CHECK(map.nth(3)->second == 2);
        CHECK(map.nth(4)->first == 20); CHECK(map.nth(4)->second == 2);
        CHECK(map.nth(5)->first == 30); CHECK(map.nth(5)->second == 2);
    }
}

PRINT("Test insert_range(Range&&");
{
    // Input iterator (exactly)
    {
        std::istringstream iss("10 1 20 1 30 1 20 2 20 3");

        sfl::small_unordered_flat_multimap<xint, xint, 32, std::equal_to<xint>, TPARAM_ALLOCATOR<std::pair<xint, xint>>> map;

        #if SFL_CPP_VERSION >= SFL_CPP_20
        map.insert_range(std::views::istream<std::pair<int, int>>(iss));
        #else
        map.insert_range(sfl::test::istream_view<std::pair<int, int>>(iss));
        #endif

        CHECK(map.size() == 5);
        CHECK(NTH(map, 0)->first == 10); CHECK(NTH(map, 0)->second == 1);
        CHECK(NTH(map, 1)->first == 20); CHECK(NTH(map, 1)->second == 1);
        CHECK(NTH(map, 2)->first == 30); CHECK(NTH(map, 2)->second == 1);
        CHECK(NTH(map, 3)->first == 20); CHECK(NTH(map, 3)->second == 2);
        CHECK(NTH(map, 4)->first == 20); CHECK(NTH(map, 4)->second == 3);
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

        sfl::small_unordered_flat_multimap<xint, xint, 32, std::equal_to<xint>, TPARAM_ALLOCATOR<std::pair<xint, xint>>> map;

        #if SFL_CPP_VERSION >= SFL_CPP_20
        map.insert_range(std::views::all(data));
        #else
        map.insert_range(data);
        #endif

        CHECK(map.size() == 5);
        CHECK(NTH(map, 0)->first == 10); CHECK(NTH(map, 0)->second == 1);
        CHECK(NTH(map, 1)->first == 20); CHECK(NTH(map, 1)->second == 1);
        CHECK(NTH(map, 2)->first == 30); CHECK(NTH(map, 2)->second == 1);
        CHECK(NTH(map, 3)->first == 20); CHECK(NTH(map, 3)->second == 2);
        CHECK(NTH(map, 4)->first == 20); CHECK(NTH(map, 4)->second == 3);
    }
}

PRINT("Test erase(const_iterator)");
{
    // Erase at the end
    {
        sfl::small_unordered_flat_multimap< xint,
                                            xint,
                                            100,
                                            std::equal_to<xint>,
                                            TPARAM_ALLOCATOR<std::pair<xint, xint>> > map;

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
        sfl::small_unordered_flat_multimap< xint,
                                            xint,
                                            100,
                                            std::equal_to<xint>,
                                            TPARAM_ALLOCATOR<std::pair<xint, xint>> > map;

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
        CHECK(map.nth(0)->first == 50); CHECK(map.nth(0)->second == 1);
        CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
        CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
        CHECK(map.nth(3)->first == 40); CHECK(map.nth(3)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(map.erase(map.nth(0)) == map.nth(0));
        CHECK(map.size() == 3);
        CHECK(map.nth(0)->first == 40); CHECK(map.nth(0)->second == 1);
        CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
        CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(map.erase(map.nth(0)) == map.nth(0));
        CHECK(map.size() == 2);
        CHECK(map.nth(0)->first == 30); CHECK(map.nth(0)->second == 1);
        CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(map.erase(map.nth(0)) == map.nth(0));
        CHECK(map.size() == 1);
        CHECK(map.nth(0)->first == 20); CHECK(map.nth(0)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(map.erase(map.nth(0)) == map.nth(0));
        CHECK(map.size() == 0);
    }

    // Erase near the end
    {
        sfl::small_unordered_flat_multimap< xint,
                                            xint,
                                            100,
                                            std::equal_to<xint>,
                                            TPARAM_ALLOCATOR<std::pair<xint, xint>> > map;

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
        sfl::small_unordered_flat_multimap< xint,
                                            xint,
                                            100,
                                            std::equal_to<xint>,
                                            TPARAM_ALLOCATOR<std::pair<xint, xint>> > map;

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
        CHECK(map.nth(1)->first == 50); CHECK(map.nth(1)->second == 1);
        CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
        CHECK(map.nth(3)->first == 40); CHECK(map.nth(3)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(map.erase(map.nth(1)) == map.nth(1));
        CHECK(map.size() == 3);
        CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
        CHECK(map.nth(1)->first == 40); CHECK(map.nth(1)->second == 1);
        CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(map.erase(map.nth(1)) == map.nth(1));
        CHECK(map.size() == 2);
        CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
        CHECK(map.nth(1)->first == 30); CHECK(map.nth(1)->second == 1);

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
        sfl::small_unordered_flat_multimap< xint,
                                            xint,
                                            100,
                                            std::equal_to<xint>,
                                            TPARAM_ALLOCATOR<std::pair<xint, xint>> > map;

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
        sfl::small_unordered_flat_multimap< xint,
                                            xint,
                                            100,
                                            std::equal_to<xint>,
                                            TPARAM_ALLOCATOR<std::pair<xint, xint>> > map;

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
        CHECK(map.nth(0)->first == 70); CHECK(map.nth(0)->second == 1);
        CHECK(map.nth(1)->first == 80); CHECK(map.nth(1)->second == 1);
        CHECK(map.nth(2)->first == 90); CHECK(map.nth(2)->second == 1);
        CHECK(map.nth(3)->first == 40); CHECK(map.nth(3)->second == 1);
        CHECK(map.nth(4)->first == 50); CHECK(map.nth(4)->second == 1);
        CHECK(map.nth(5)->first == 60); CHECK(map.nth(5)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(map.erase(map.nth(0), map.nth(3)) == map.nth(0));
        CHECK(map.size() == 3);
        CHECK(map.nth(0)->first == 40); CHECK(map.nth(0)->second == 1);
        CHECK(map.nth(1)->first == 50); CHECK(map.nth(1)->second == 1);
        CHECK(map.nth(2)->first == 60); CHECK(map.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(map.erase(map.nth(0), map.nth(3)) == map.nth(0));
        CHECK(map.size() == 0);
    }

    // Erase near the end
    {
        sfl::small_unordered_flat_multimap< xint,
                                            xint,
                                            100,
                                            std::equal_to<xint>,
                                            TPARAM_ALLOCATOR<std::pair<xint, xint>> > map;

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
        sfl::small_unordered_flat_multimap< xint,
                                            xint,
                                            100,
                                            std::equal_to<xint>,
                                            TPARAM_ALLOCATOR<std::pair<xint, xint>> > map;

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
        CHECK(map.nth(1)->first == 70); CHECK(map.nth(1)->second == 1);
        CHECK(map.nth(2)->first == 80); CHECK(map.nth(2)->second == 1);
        CHECK(map.nth(3)->first == 90); CHECK(map.nth(3)->second == 1);
        CHECK(map.nth(4)->first == 50); CHECK(map.nth(4)->second == 1);
        CHECK(map.nth(5)->first == 60); CHECK(map.nth(5)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(map.erase(map.nth(1), map.nth(4)) == map.nth(1));
        CHECK(map.size() == 3);
        CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
        CHECK(map.nth(1)->first == 50); CHECK(map.nth(1)->second == 1);
        CHECK(map.nth(2)->first == 60); CHECK(map.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(map.erase(map.nth(1), map.nth(2)) == map.nth(1));
        CHECK(map.size() == 2);
        CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
        CHECK(map.nth(1)->first == 60); CHECK(map.nth(1)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(map.erase(map.nth(0), map.nth(2)) == map.nth(0));
        CHECK(map.size() == 0);
    }
}

PRINT("Test erase(const Key&)");
{
    sfl::small_unordered_flat_multimap< xint,
                                        xint,
                                        100,
                                        std::equal_to<xint>,
                                        TPARAM_ALLOCATOR<std::pair<xint, xint>> > map;

    map.emplace(10, 1);
    map.emplace(20, 1);
    map.emplace(20, 2);
    map.emplace(20, 3);
    map.emplace(30, 1);

    CHECK(map.size() == 5);
    CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
    CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
    CHECK(map.nth(2)->first == 20); CHECK(map.nth(2)->second == 2);
    CHECK(map.nth(3)->first == 20); CHECK(map.nth(3)->second == 3);
    CHECK(map.nth(4)->first == 30); CHECK(map.nth(4)->second == 1);

    CHECK(map.erase(30) == 1);
    CHECK(map.erase(30) == 0);
    CHECK(map.size() == 4);
    CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
    CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
    CHECK(map.nth(2)->first == 20); CHECK(map.nth(2)->second == 2);
    CHECK(map.nth(3)->first == 20); CHECK(map.nth(3)->second == 3);

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
    sfl::small_unordered_flat_multimap< xobj,
                                        xint,
                                        100,
                                        xobj::equal,
                                        TPARAM_ALLOCATOR<std::pair<xobj, xint>> > map;

    map.emplace(std::piecewise_construct, std::forward_as_tuple(10), std::forward_as_tuple(1));
    map.emplace(std::piecewise_construct, std::forward_as_tuple(20), std::forward_as_tuple(1));
    map.emplace(std::piecewise_construct, std::forward_as_tuple(20), std::forward_as_tuple(2));
    map.emplace(std::piecewise_construct, std::forward_as_tuple(20), std::forward_as_tuple(3));
    map.emplace(std::piecewise_construct, std::forward_as_tuple(30), std::forward_as_tuple(1));

    CHECK(map.size() == 5);
    CHECK(map.nth(0)->first.value() == 10); CHECK(map.nth(0)->second == 1);
    CHECK(map.nth(1)->first.value() == 20); CHECK(map.nth(1)->second == 1);
    CHECK(map.nth(2)->first.value() == 20); CHECK(map.nth(2)->second == 2);
    CHECK(map.nth(3)->first.value() == 20); CHECK(map.nth(3)->second == 3);
    CHECK(map.nth(4)->first.value() == 30); CHECK(map.nth(4)->second == 1);

    CHECK(map.erase(30) == 1);
    CHECK(map.erase(30) == 0);
    CHECK(map.size() == 4);
    CHECK(map.nth(0)->first.value() == 10); CHECK(map.nth(0)->second == 1);
    CHECK(map.nth(1)->first.value() == 20); CHECK(map.nth(1)->second == 1);
    CHECK(map.nth(2)->first.value() == 20); CHECK(map.nth(2)->second == 2);
    CHECK(map.nth(3)->first.value() == 20); CHECK(map.nth(3)->second == 3);

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
    // Both containers are using internal storage
    {
        sfl::small_unordered_flat_multimap< xint,
                                            xint,
                                            5,
                                            std::equal_to<xint>,
                                            TPARAM_ALLOCATOR<std::pair<xint, xint>> > map1, map2;

        map1.emplace(10, 1);
        map1.emplace(20, 1);
        map1.emplace(30, 1);

        map2.emplace(40, 2);
        map2.emplace(50, 2);
        map2.emplace(60, 2);
        map2.emplace(70, 2);

        CHECK(map1.size() == 3);
        CHECK(map1.capacity() == 5);
        CHECK(map1.nth(0)->first == 10); CHECK(map1.nth(0)->second == 1);
        CHECK(map1.nth(1)->first == 20); CHECK(map1.nth(1)->second == 1);
        CHECK(map1.nth(2)->first == 30); CHECK(map1.nth(2)->second == 1);

        CHECK(map2.size() == 4);
        CHECK(map2.capacity() == 5);
        CHECK(map2.nth(0)->first == 40); CHECK(map2.nth(0)->second == 2);
        CHECK(map2.nth(1)->first == 50); CHECK(map2.nth(1)->second == 2);
        CHECK(map2.nth(2)->first == 60); CHECK(map2.nth(2)->second == 2);
        CHECK(map2.nth(3)->first == 70); CHECK(map2.nth(3)->second == 2);

        ///////////////////////////////////////////////////////////////////////

        map1.swap(map2);

        CHECK(map1.size() == 4);
        CHECK(map1.capacity() == 5);
        CHECK(map1.nth(0)->first == 40); CHECK(map1.nth(0)->second == 2);
        CHECK(map1.nth(1)->first == 50); CHECK(map1.nth(1)->second == 2);
        CHECK(map1.nth(2)->first == 60); CHECK(map1.nth(2)->second == 2);
        CHECK(map1.nth(3)->first == 70); CHECK(map1.nth(3)->second == 2);

        CHECK(map2.size() == 3);
        CHECK(map2.capacity() == 5);
        CHECK(map2.nth(0)->first == 10); CHECK(map2.nth(0)->second == 1);
        CHECK(map2.nth(1)->first == 20); CHECK(map2.nth(1)->second == 1);
        CHECK(map2.nth(2)->first == 30); CHECK(map2.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        map1.swap(map2);

        CHECK(map1.size() == 3);
        CHECK(map1.capacity() == 5);
        CHECK(map1.nth(0)->first == 10); CHECK(map1.nth(0)->second == 1);
        CHECK(map1.nth(1)->first == 20); CHECK(map1.nth(1)->second == 1);
        CHECK(map1.nth(2)->first == 30); CHECK(map1.nth(2)->second == 1);

        CHECK(map2.size() == 4);
        CHECK(map2.capacity() == 5);
        CHECK(map2.nth(0)->first == 40); CHECK(map2.nth(0)->second == 2);
        CHECK(map2.nth(1)->first == 50); CHECK(map2.nth(1)->second == 2);
        CHECK(map2.nth(2)->first == 60); CHECK(map2.nth(2)->second == 2);
        CHECK(map2.nth(3)->first == 70); CHECK(map2.nth(3)->second == 2);
    }

    // One container is using internal storage and another is using external
    {
        sfl::small_unordered_flat_multimap< xint,
                                            xint,
                                            5,
                                            std::equal_to<xint>,
                                            TPARAM_ALLOCATOR<std::pair<xint, xint>> > map1, map2;

        map1.emplace(10, 1);
        map1.emplace(20, 1);
        map1.emplace(30, 1);

        map2.emplace(40, 2);
        map2.emplace(50, 2);
        map2.emplace(60, 2);
        map2.emplace(70, 2);
        map2.emplace(80, 2);
        map2.emplace(90, 2);

        CHECK(map1.size() == 3);
        CHECK(map1.capacity() == 5);
        CHECK(map1.nth(0)->first == 10); CHECK(map1.nth(0)->second == 1);
        CHECK(map1.nth(1)->first == 20); CHECK(map1.nth(1)->second == 1);
        CHECK(map1.nth(2)->first == 30); CHECK(map1.nth(2)->second == 1);

        CHECK(map2.size() == 6);
        CHECK(map2.capacity() == 7);
        CHECK(map2.nth(0)->first == 40); CHECK(map2.nth(0)->second == 2);
        CHECK(map2.nth(1)->first == 50); CHECK(map2.nth(1)->second == 2);
        CHECK(map2.nth(2)->first == 60); CHECK(map2.nth(2)->second == 2);
        CHECK(map2.nth(3)->first == 70); CHECK(map2.nth(3)->second == 2);
        CHECK(map2.nth(4)->first == 80); CHECK(map2.nth(4)->second == 2);
        CHECK(map2.nth(5)->first == 90); CHECK(map2.nth(5)->second == 2);

        ///////////////////////////////////////////////////////////////////////

        map1.swap(map2);

        CHECK(map1.size() == 6);
        CHECK(map1.capacity() == 7);
        CHECK(map1.nth(0)->first == 40); CHECK(map1.nth(0)->second == 2);
        CHECK(map1.nth(1)->first == 50); CHECK(map1.nth(1)->second == 2);
        CHECK(map1.nth(2)->first == 60); CHECK(map1.nth(2)->second == 2);
        CHECK(map1.nth(3)->first == 70); CHECK(map1.nth(3)->second == 2);
        CHECK(map1.nth(4)->first == 80); CHECK(map1.nth(4)->second == 2);
        CHECK(map1.nth(5)->first == 90); CHECK(map1.nth(5)->second == 2);

        CHECK(map2.size() == 3);
        CHECK(map2.capacity() == 5);
        CHECK(map2.nth(0)->first == 10); CHECK(map2.nth(0)->second == 1);
        CHECK(map2.nth(1)->first == 20); CHECK(map2.nth(1)->second == 1);
        CHECK(map2.nth(2)->first == 30); CHECK(map2.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        map1.swap(map2);

        CHECK(map1.size() == 3);
        CHECK(map1.capacity() == 5);
        CHECK(map1.nth(0)->first == 10); CHECK(map1.nth(0)->second == 1);
        CHECK(map1.nth(1)->first == 20); CHECK(map1.nth(1)->second == 1);
        CHECK(map1.nth(2)->first == 30); CHECK(map1.nth(2)->second == 1);

        CHECK(map2.size() == 6);
        CHECK(map2.capacity() == 7);
        CHECK(map2.nth(0)->first == 40); CHECK(map2.nth(0)->second == 2);
        CHECK(map2.nth(1)->first == 50); CHECK(map2.nth(1)->second == 2);
        CHECK(map2.nth(2)->first == 60); CHECK(map2.nth(2)->second == 2);
        CHECK(map2.nth(3)->first == 70); CHECK(map2.nth(3)->second == 2);
        CHECK(map2.nth(4)->first == 80); CHECK(map2.nth(4)->second == 2);
        CHECK(map2.nth(5)->first == 90); CHECK(map2.nth(5)->second == 2);
    }

    // Both containers are using external storage
    {
        sfl::small_unordered_flat_multimap< xint,
                                            xint,
                                            0,
                                            std::equal_to<xint>,
                                            TPARAM_ALLOCATOR<std::pair<xint, xint>> > map1, map2;

        map1.emplace(10, 1);
        map1.emplace(20, 1);
        map1.emplace(30, 1);

        map2.emplace(40, 2);
        map2.emplace(50, 2);
        map2.emplace(60, 2);
        map2.emplace(70, 2);

        CHECK(map1.size() == 3);
        CHECK(map1.capacity() == 3);
        CHECK(map1.nth(0)->first == 10); CHECK(map1.nth(0)->second == 1);
        CHECK(map1.nth(1)->first == 20); CHECK(map1.nth(1)->second == 1);
        CHECK(map1.nth(2)->first == 30); CHECK(map1.nth(2)->second == 1);

        CHECK(map2.size() == 4);
        CHECK(map2.capacity() == 4);
        CHECK(map2.nth(0)->first == 40); CHECK(map2.nth(0)->second == 2);
        CHECK(map2.nth(1)->first == 50); CHECK(map2.nth(1)->second == 2);
        CHECK(map2.nth(2)->first == 60); CHECK(map2.nth(2)->second == 2);
        CHECK(map2.nth(3)->first == 70); CHECK(map2.nth(3)->second == 2);

        ///////////////////////////////////////////////////////////////////////

        map1.swap(map2);

        CHECK(map1.size() == 4);
        CHECK(map1.capacity() == 4);
        CHECK(map1.nth(0)->first == 40); CHECK(map1.nth(0)->second == 2);
        CHECK(map1.nth(1)->first == 50); CHECK(map1.nth(1)->second == 2);
        CHECK(map1.nth(2)->first == 60); CHECK(map1.nth(2)->second == 2);
        CHECK(map1.nth(3)->first == 70); CHECK(map1.nth(3)->second == 2);

        CHECK(map2.size() == 3);
        CHECK(map2.capacity() == 3);
        CHECK(map2.nth(0)->first == 10); CHECK(map2.nth(0)->second == 1);
        CHECK(map2.nth(1)->first == 20); CHECK(map2.nth(1)->second == 1);
        CHECK(map2.nth(2)->first == 30); CHECK(map2.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        map1.swap(map2);

        CHECK(map1.size() == 3);
        CHECK(map1.capacity() == 3);
        CHECK(map1.nth(0)->first == 10); CHECK(map1.nth(0)->second == 1);
        CHECK(map1.nth(1)->first == 20); CHECK(map1.nth(1)->second == 1);
        CHECK(map1.nth(2)->first == 30); CHECK(map1.nth(2)->second == 1);

        CHECK(map2.size() == 4);
        CHECK(map2.capacity() == 4);
        CHECK(map2.nth(0)->first == 40); CHECK(map2.nth(0)->second == 2);
        CHECK(map2.nth(1)->first == 50); CHECK(map2.nth(1)->second == 2);
        CHECK(map2.nth(2)->first == 60); CHECK(map2.nth(2)->second == 2);
        CHECK(map2.nth(3)->first == 70); CHECK(map2.nth(3)->second == 2);
    }
}

///////////////////////////////////////////////////////////////////////////////

PRINT("Test data()");
{
    sfl::small_unordered_flat_multimap< xint,
                                        xint,
                                        100,
                                        std::equal_to<xint>,
                                        TPARAM_ALLOCATOR<std::pair<xint, xint>> > map;

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

PRINT("Test reserve(size_type) and shrink_to_fit()");
{
    // N = 0
    {
        sfl::small_unordered_flat_multimap< xint,
                                            xint,
                                            0,
                                            std::equal_to<xint>,
                                            TPARAM_ALLOCATOR<std::pair<xint, xint>> > map;

        map.emplace(10, 1);
        map.emplace(20, 1);
        map.emplace(30, 1);
        map.emplace(40, 1);
        map.erase(map.end() - 1);

        CHECK(map.size() == 3);
        CHECK(map.capacity() == 4);
        CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
        CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
        CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        map.shrink_to_fit();

        CHECK(map.size() == 3);
        CHECK(map.capacity() == 3);
        CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
        CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
        CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        map.reserve(999);

        CHECK(map.size() == 3);
        CHECK(map.capacity() == 999);
        CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
        CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
        CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        map.shrink_to_fit();

        CHECK(map.size() == 3);
        CHECK(map.capacity() == 3);
        CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
        CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
        CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
    }

    // N = 100
    {
        sfl::small_unordered_flat_multimap< xint,
                                            xint,
                                            100,
                                            std::equal_to<xint>,
                                            TPARAM_ALLOCATOR<std::pair<xint, xint>> > map;

        map.emplace(10, 1);
        map.emplace(20, 1);
        map.emplace(30, 1);

        CHECK(map.size() == 3);
        CHECK(map.capacity() == 100);
        CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
        CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
        CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        map.shrink_to_fit();

        CHECK(map.size() == 3);
        CHECK(map.capacity() == 100);
        CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
        CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
        CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        map.reserve(999);

        CHECK(map.size() == 3);
        CHECK(map.capacity() == 999);
        CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
        CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
        CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        map.shrink_to_fit();

        CHECK(map.size() == 3);
        CHECK(map.capacity() == 100);
        CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
        CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
        CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
    }
}

///////////////////////////////////////////////////////////////////////////////

PRINT("Test container()");
{
    sfl::small_unordered_flat_multimap< xint,
                                        xint,
                                        100,
                                        std::equal_to<xint>,
                                        TPARAM_ALLOCATOR<std::pair<xint, xint>> > map;

    CHECK(map.size() == 0);
    CHECK(map.capacity() == 100);
    CHECK(map.available() == 100);
}

PRINT("Test container(const KeyEqual&)");
{
    std::equal_to<xint> equal;

    sfl::small_unordered_flat_multimap< xint,
                                        xint,
                                        100,
                                        std::equal_to<xint>,
                                        TPARAM_ALLOCATOR<std::pair<xint, xint>> > map(equal);

    CHECK(map.size() == 0);
    CHECK(map.capacity() == 100);
    CHECK(map.available() == 100);
}

PRINT("Test container(const Allocator&)");
{
    TPARAM_ALLOCATOR<std::pair<xint, xint>> alloc;

    sfl::small_unordered_flat_multimap< xint,
                                        xint,
                                        100,
                                        std::equal_to<xint>,
                                        TPARAM_ALLOCATOR<std::pair<xint, xint>> > map(alloc);

    CHECK(map.size() == 0);
    CHECK(map.capacity() == 100);
    CHECK(map.available() == 100);
}

PRINT("Test container(const KeyEqual&, const Allocator&)");
{
    std::equal_to<xint> equal;

    TPARAM_ALLOCATOR<std::pair<xint, xint>> alloc;

    sfl::small_unordered_flat_multimap< xint,
                                        xint,
                                        100,
                                        std::equal_to<xint>,
                                        TPARAM_ALLOCATOR<std::pair<xint, xint>> > map(equal, alloc);

    CHECK(map.size() == 0);
    CHECK(map.capacity() == 100);
    CHECK(map.available() == 100);
}

PRINT("Test container(InputIt, InputIt)");
{
    std::vector<std::pair<xint, xint>> data
    (
        {
            {10, 1},
            {20, 1},
            {30, 1},

            {10, 2},
            {20, 2},
            {30, 2}
        }
    );

    sfl::small_unordered_flat_multimap< xint,
                                        xint,
                                        100,
                                        std::equal_to<xint>,
                                        TPARAM_ALLOCATOR<std::pair<xint, xint>> > map(data.begin(), data.end());

    CHECK(map.size() == 6);
    CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
    CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
    CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
    CHECK(map.nth(3)->first == 10); CHECK(map.nth(3)->second == 2);
    CHECK(map.nth(4)->first == 20); CHECK(map.nth(4)->second == 2);
    CHECK(map.nth(5)->first == 30); CHECK(map.nth(5)->second == 2);
}

PRINT("Test container(InputIt, InputIt, const KeyEqual&)");
{
    std::vector<std::pair<xint, xint>> data
    (
        {
            {10, 1},
            {20, 1},
            {30, 1},

            {10, 2},
            {20, 2},
            {30, 2}
        }
    );

    std::equal_to<xint> equal;

    sfl::small_unordered_flat_multimap< xint,
                                        xint,
                                        100,
                                        std::equal_to<xint>,
                                        TPARAM_ALLOCATOR<std::pair<xint, xint>> > map(data.begin(), data.end(), equal);

    CHECK(map.size() == 6);
    CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
    CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
    CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
    CHECK(map.nth(3)->first == 10); CHECK(map.nth(3)->second == 2);
    CHECK(map.nth(4)->first == 20); CHECK(map.nth(4)->second == 2);
    CHECK(map.nth(5)->first == 30); CHECK(map.nth(5)->second == 2);
}

PRINT("Test container(InputIt, InputIt, const Allocator&)");
{
    std::vector<std::pair<xint, xint>> data
    (
        {
            {10, 1},
            {20, 1},
            {30, 1},

            {10, 2},
            {20, 2},
            {30, 2}
        }
    );

    TPARAM_ALLOCATOR<std::pair<xint, xint>> alloc;

    sfl::small_unordered_flat_multimap< xint,
                                        xint,
                                        100,
                                        std::equal_to<xint>,
                                        TPARAM_ALLOCATOR<std::pair<xint, xint>> > map(data.begin(), data.end(), alloc);

    CHECK(map.size() == 6);
    CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
    CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
    CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
    CHECK(map.nth(3)->first == 10); CHECK(map.nth(3)->second == 2);
    CHECK(map.nth(4)->first == 20); CHECK(map.nth(4)->second == 2);
    CHECK(map.nth(5)->first == 30); CHECK(map.nth(5)->second == 2);
}

PRINT("Test container(InputIt, InputIt, const KeyEqual&, const Allocator&)");
{
    std::vector<std::pair<xint, xint>> data
    (
        {
            {10, 1},
            {20, 1},
            {30, 1},

            {10, 2},
            {20, 2},
            {30, 2}
        }
    );

    std::equal_to<xint> equal;

    TPARAM_ALLOCATOR<std::pair<xint, xint>> alloc;

    sfl::small_unordered_flat_multimap< xint,
                                        xint,
                                        100,
                                        std::equal_to<xint>,
                                        TPARAM_ALLOCATOR<std::pair<xint, xint>> > map(data.begin(), data.end(), equal, alloc);

    CHECK(map.size() == 6);
    CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
    CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
    CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
    CHECK(map.nth(3)->first == 10); CHECK(map.nth(3)->second == 2);
    CHECK(map.nth(4)->first == 20); CHECK(map.nth(4)->second == 2);
    CHECK(map.nth(5)->first == 30); CHECK(map.nth(5)->second == 2);
}

PRINT("Test container(std::initializer_list)");
{
    std::initializer_list<std::pair<xint, xint>> ilist
    {
        {10, 1},
        {20, 1},
        {30, 1},

        {10, 2},
        {20, 2},
        {30, 2}
    };

    sfl::small_unordered_flat_multimap< xint,
                                        xint,
                                        100,
                                        std::equal_to<xint>,
                                        TPARAM_ALLOCATOR<std::pair<xint, xint>> > map(ilist);

    CHECK(map.size() == 6);
    CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
    CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
    CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
    CHECK(map.nth(3)->first == 10); CHECK(map.nth(3)->second == 2);
    CHECK(map.nth(4)->first == 20); CHECK(map.nth(4)->second == 2);
    CHECK(map.nth(5)->first == 30); CHECK(map.nth(5)->second == 2);
}

PRINT("Test container(std::initializer_list, const KeyEqual&)");
{
    std::initializer_list<std::pair<xint, xint>> ilist
    {
        {10, 1},
        {20, 1},
        {30, 1},

        {10, 2},
        {20, 2},
        {30, 2}
    };

    std::equal_to<xint> equal;

    sfl::small_unordered_flat_multimap< xint,
                                        xint,
                                        100,
                                        std::equal_to<xint>,
                                        TPARAM_ALLOCATOR<std::pair<xint, xint>> > map(ilist, equal);

    CHECK(map.size() == 6);
    CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
    CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
    CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
    CHECK(map.nth(3)->first == 10); CHECK(map.nth(3)->second == 2);
    CHECK(map.nth(4)->first == 20); CHECK(map.nth(4)->second == 2);
    CHECK(map.nth(5)->first == 30); CHECK(map.nth(5)->second == 2);
}

PRINT("Test container(std::initializer_list, const Allocator&)");
{
    std::initializer_list<std::pair<xint, xint>> ilist
    {
        {10, 1},
        {20, 1},
        {30, 1},

        {10, 2},
        {20, 2},
        {30, 2}
    };

    TPARAM_ALLOCATOR<std::pair<xint, xint>> alloc;

    sfl::small_unordered_flat_multimap< xint,
                                        xint,
                                        100,
                                        std::equal_to<xint>,
                                        TPARAM_ALLOCATOR<std::pair<xint, xint>> > map(ilist, alloc);

    CHECK(map.size() == 6);
    CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
    CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
    CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
    CHECK(map.nth(3)->first == 10); CHECK(map.nth(3)->second == 2);
    CHECK(map.nth(4)->first == 20); CHECK(map.nth(4)->second == 2);
    CHECK(map.nth(5)->first == 30); CHECK(map.nth(5)->second == 2);
}

PRINT("Test container(std::initializer_list, const KeyEqual&, const Allocator&)");
{
    std::initializer_list<std::pair<xint, xint>> ilist
    {
        {10, 1},
        {20, 1},
        {30, 1},

        {10, 2},
        {20, 2},
        {30, 2}
    };

    std::equal_to<xint> equal;

    TPARAM_ALLOCATOR<std::pair<xint, xint>> alloc;

    sfl::small_unordered_flat_multimap< xint,
                                        xint,
                                        100,
                                        std::equal_to<xint>,
                                        TPARAM_ALLOCATOR<std::pair<xint, xint>> > map(ilist, equal, alloc);

    CHECK(map.size() == 6);
    CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
    CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
    CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);
    CHECK(map.nth(3)->first == 10); CHECK(map.nth(3)->second == 2);
    CHECK(map.nth(4)->first == 20); CHECK(map.nth(4)->second == 2);
    CHECK(map.nth(5)->first == 30); CHECK(map.nth(5)->second == 2);
}

PRINT("Test container(const container&)");
{
    // N == 0
    {
        sfl::small_unordered_flat_multimap< xint,
                                            xint,
                                            0,
                                            std::equal_to<xint>,
                                            TPARAM_ALLOCATOR<std::pair<xint, xint>> > map1;

        map1.emplace(10, 1);
        map1.emplace(20, 1);
        map1.emplace(30, 1);

        CHECK(map1.size() == 3);
        CHECK(map1.nth(0)->first == 10); CHECK(map1.nth(0)->second == 1);
        CHECK(map1.nth(1)->first == 20); CHECK(map1.nth(1)->second == 1);
        CHECK(map1.nth(2)->first == 30); CHECK(map1.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        sfl::small_unordered_flat_multimap< xint,
                                            xint,
                                            0,
                                            std::equal_to<xint>,
                                            TPARAM_ALLOCATOR<std::pair<xint, xint>> > map2(map1);

        CHECK(map2.size() == 3);
        CHECK(map2.nth(0)->first == 10); CHECK(map2.nth(0)->second == 1);
        CHECK(map2.nth(1)->first == 20); CHECK(map2.nth(1)->second == 1);
        CHECK(map2.nth(2)->first == 30); CHECK(map2.nth(2)->second == 1);
    }

    // N == 100
    {
        sfl::small_unordered_flat_multimap< xint,
                                            xint,
                                            100,
                                            std::equal_to<xint>,
                                            TPARAM_ALLOCATOR<std::pair<xint, xint>> > map1;

        map1.emplace(10, 1);
        map1.emplace(20, 1);
        map1.emplace(30, 1);

        CHECK(map1.size() == 3);
        CHECK(map1.nth(0)->first == 10); CHECK(map1.nth(0)->second == 1);
        CHECK(map1.nth(1)->first == 20); CHECK(map1.nth(1)->second == 1);
        CHECK(map1.nth(2)->first == 30); CHECK(map1.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        sfl::small_unordered_flat_multimap< xint,
                                            xint,
                                            100,
                                            std::equal_to<xint>,
                                            TPARAM_ALLOCATOR<std::pair<xint, xint>> > map2(map1);

        CHECK(map2.size() == 3);
        CHECK(map2.nth(0)->first == 10); CHECK(map2.nth(0)->second == 1);
        CHECK(map2.nth(1)->first == 20); CHECK(map2.nth(1)->second == 1);
        CHECK(map2.nth(2)->first == 30); CHECK(map2.nth(2)->second == 1);
    }
}

PRINT("Test container(const container&, const Allocator&)");
{
    // N == 0
    {
        sfl::small_unordered_flat_multimap< xint,
                                            xint,
                                            0,
                                            std::equal_to<xint>,
                                            TPARAM_ALLOCATOR<std::pair<xint, xint>> > map1;

        map1.emplace(10, 1);
        map1.emplace(20, 1);
        map1.emplace(30, 1);

        CHECK(map1.size() == 3);
        CHECK(map1.nth(0)->first == 10); CHECK(map1.nth(0)->second == 1);
        CHECK(map1.nth(1)->first == 20); CHECK(map1.nth(1)->second == 1);
        CHECK(map1.nth(2)->first == 30); CHECK(map1.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        TPARAM_ALLOCATOR<std::pair<xint, xint>> alloc;

        sfl::small_unordered_flat_multimap< xint,
                                            xint,
                                            0,
                                            std::equal_to<xint>,
                                            TPARAM_ALLOCATOR<std::pair<xint, xint>> > map2(map1, alloc);

        CHECK(map2.size() == 3);
        CHECK(map2.nth(0)->first == 10); CHECK(map2.nth(0)->second == 1);
        CHECK(map2.nth(1)->first == 20); CHECK(map2.nth(1)->second == 1);
        CHECK(map2.nth(2)->first == 30); CHECK(map2.nth(2)->second == 1);
    }

    // N == 100
    {
        sfl::small_unordered_flat_multimap< xint,
                                            xint,
                                            100,
                                            std::equal_to<xint>,
                                            TPARAM_ALLOCATOR<std::pair<xint, xint>> > map1;

        map1.emplace(10, 1);
        map1.emplace(20, 1);
        map1.emplace(30, 1);

        CHECK(map1.size() == 3);
        CHECK(map1.nth(0)->first == 10); CHECK(map1.nth(0)->second == 1);
        CHECK(map1.nth(1)->first == 20); CHECK(map1.nth(1)->second == 1);
        CHECK(map1.nth(2)->first == 30); CHECK(map1.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        TPARAM_ALLOCATOR<std::pair<xint, xint>> alloc;

        sfl::small_unordered_flat_multimap< xint,
                                            xint,
                                            100,
                                            std::equal_to<xint>,
                                            TPARAM_ALLOCATOR<std::pair<xint, xint>> > map2(map1, alloc);

        CHECK(map2.size() == 3);
        CHECK(map2.nth(0)->first == 10); CHECK(map2.nth(0)->second == 1);
        CHECK(map2.nth(1)->first == 20); CHECK(map2.nth(1)->second == 1);
        CHECK(map2.nth(2)->first == 30); CHECK(map2.nth(2)->second == 1);
    }
}

PRINT("Test container(container&&)");
{
    // N == 0
    {
        sfl::small_unordered_flat_multimap< xint,
                                            xint,
                                            0,
                                            std::equal_to<xint>,
                                            TPARAM_ALLOCATOR<std::pair<xint, xint>> > map1;

        map1.emplace(10, 1);
        map1.emplace(20, 1);
        map1.emplace(30, 1);

        CHECK(map1.size() == 3);
        CHECK(map1.nth(0)->first == 10); CHECK(map1.nth(0)->second == 1);
        CHECK(map1.nth(1)->first == 20); CHECK(map1.nth(1)->second == 1);
        CHECK(map1.nth(2)->first == 30); CHECK(map1.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        sfl::small_unordered_flat_multimap< xint,
                                            xint,
                                            0,
                                            std::equal_to<xint>,
                                            TPARAM_ALLOCATOR<std::pair<xint, xint>> > map2(std::move(map1));

        CHECK(map2.size() == 3);
        CHECK(map2.nth(0)->first == 10); CHECK(map2.nth(0)->second == 1);
        CHECK(map2.nth(1)->first == 20); CHECK(map2.nth(1)->second == 1);
        CHECK(map2.nth(2)->first == 30); CHECK(map2.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        if (map1.size() == 3)
        {
            // Elements were moved one-by-one
            CHECK(map1.nth(0)->first == -10); CHECK(map1.nth(0)->second == -1);
            CHECK(map1.nth(1)->first == -20); CHECK(map1.nth(1)->second == -1);
            CHECK(map1.nth(2)->first == -30); CHECK(map1.nth(2)->second == -1);
        }
        else
        {
            // Storage was moved
            CHECK(map1.size() == 0);
        }
    }

    // N == 100
    {
        sfl::small_unordered_flat_multimap< xint,
                                            xint,
                                            100,
                                            std::equal_to<xint>,
                                            TPARAM_ALLOCATOR<std::pair<xint, xint>> > map1;

        map1.emplace(10, 1);
        map1.emplace(20, 1);
        map1.emplace(30, 1);

        CHECK(map1.size() == 3);
        CHECK(map1.nth(0)->first == 10); CHECK(map1.nth(0)->second == 1);
        CHECK(map1.nth(1)->first == 20); CHECK(map1.nth(1)->second == 1);
        CHECK(map1.nth(2)->first == 30); CHECK(map1.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        sfl::small_unordered_flat_multimap< xint,
                                            xint,
                                            100,
                                            std::equal_to<xint>,
                                            TPARAM_ALLOCATOR<std::pair<xint, xint>> > map2(std::move(map1));

        CHECK(map2.size() == 3);
        CHECK(map2.nth(0)->first == 10); CHECK(map2.nth(0)->second == 1);
        CHECK(map2.nth(1)->first == 20); CHECK(map2.nth(1)->second == 1);
        CHECK(map2.nth(2)->first == 30); CHECK(map2.nth(2)->second == 1);

        if (map1.size() == 3)
        {
            // Elements were moved one-by-one
            CHECK(map1.nth(0)->first == -10); CHECK(map1.nth(0)->second == -1);
            CHECK(map1.nth(1)->first == -20); CHECK(map1.nth(1)->second == -1);
            CHECK(map1.nth(2)->first == -30); CHECK(map1.nth(2)->second == -1);
        }
        else
        {
            // Storage was moved
            CHECK(map1.size() == 0);
        }
    }
}

PRINT("Test container(container&&, const Allocator&)");
{
    // N == 0
    {
        sfl::small_unordered_flat_multimap< xint,
                                            xint,
                                            0,
                                            std::equal_to<xint>,
                                            TPARAM_ALLOCATOR<std::pair<xint, xint>> > map1;

        map1.emplace(10, 1);
        map1.emplace(20, 1);
        map1.emplace(30, 1);

        CHECK(map1.size() == 3);
        CHECK(map1.nth(0)->first == 10); CHECK(map1.nth(0)->second == 1);
        CHECK(map1.nth(1)->first == 20); CHECK(map1.nth(1)->second == 1);
        CHECK(map1.nth(2)->first == 30); CHECK(map1.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        TPARAM_ALLOCATOR<std::pair<xint, xint>> alloc;

        sfl::small_unordered_flat_multimap< xint,
                                            xint,
                                            0,
                                            std::equal_to<xint>,
                                            TPARAM_ALLOCATOR<std::pair<xint, xint>> > map2(std::move(map1), alloc);

        CHECK(map2.size() == 3);
        CHECK(map2.nth(0)->first == 10); CHECK(map2.nth(0)->second == 1);
        CHECK(map2.nth(1)->first == 20); CHECK(map2.nth(1)->second == 1);
        CHECK(map2.nth(2)->first == 30); CHECK(map2.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        if (map1.size() == 3)
        {
            // Elements were moved one-by-one
            CHECK(map1.nth(0)->first == -10); CHECK(map1.nth(0)->second == -1);
            CHECK(map1.nth(1)->first == -20); CHECK(map1.nth(1)->second == -1);
            CHECK(map1.nth(2)->first == -30); CHECK(map1.nth(2)->second == -1);
        }
        else
        {
            // Storage was moved
            CHECK(map1.size() == 0);
        }
    }

    // N == 100
    {
        sfl::small_unordered_flat_multimap< xint,
                                            xint,
                                            100,
                                            std::equal_to<xint>,
                                            TPARAM_ALLOCATOR<std::pair<xint, xint>> > map1;

        map1.emplace(10, 1);
        map1.emplace(20, 1);
        map1.emplace(30, 1);

        CHECK(map1.size() == 3);
        CHECK(map1.nth(0)->first == 10); CHECK(map1.nth(0)->second == 1);
        CHECK(map1.nth(1)->first == 20); CHECK(map1.nth(1)->second == 1);
        CHECK(map1.nth(2)->first == 30); CHECK(map1.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        TPARAM_ALLOCATOR<std::pair<xint, xint>> alloc;

        sfl::small_unordered_flat_multimap< xint,
                                            xint,
                                            100,
                                            std::equal_to<xint>,
                                            TPARAM_ALLOCATOR<std::pair<xint, xint>> > map2(std::move(map1), alloc);

        CHECK(map2.size() == 3);
        CHECK(map2.nth(0)->first == 10); CHECK(map2.nth(0)->second == 1);
        CHECK(map2.nth(1)->first == 20); CHECK(map2.nth(1)->second == 1);
        CHECK(map2.nth(2)->first == 30); CHECK(map2.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        if (map1.size() == 3)
        {
            // Elements were moved one-by-one
            CHECK(map1.nth(0)->first == -10); CHECK(map1.nth(0)->second == -1);
            CHECK(map1.nth(1)->first == -20); CHECK(map1.nth(1)->second == -1);
            CHECK(map1.nth(2)->first == -30); CHECK(map1.nth(2)->second == -1);
        }
        else
        {
            // Storage was moved
            CHECK(map1.size() == 0);
        }
    }
}

PRINT("Test container(sfl::from_range_t, Range&&)");
{
    // Input iterator (exactly)
    {
        std::istringstream iss("10 1 20 1 30 1 20 2 20 3");

        #if SFL_CPP_VERSION >= SFL_CPP_20
        sfl::small_unordered_flat_multimap<xint, xint, 32, std::equal_to<xint>, TPARAM_ALLOCATOR<std::pair<xint, xint>>> map
        (
            (sfl::from_range_t()),
            (std::views::istream<std::pair<int, int>>(iss))
        );
        #else
        sfl::small_unordered_flat_multimap<xint, xint, 32, std::equal_to<xint>, TPARAM_ALLOCATOR<std::pair<xint, xint>>> map
        (
            (sfl::from_range_t()),
            (sfl::test::istream_view<std::pair<int, int>>(iss))
        );
        #endif

        CHECK(map.empty() == false);
        CHECK(map.size() == 5);
        CHECK(map.max_size() > 0);
        CHECK(NTH(map, 0)->first == 10); CHECK(NTH(map, 0)->second == 1);
        CHECK(NTH(map, 1)->first == 20); CHECK(NTH(map, 1)->second == 1);
        CHECK(NTH(map, 2)->first == 30); CHECK(NTH(map, 2)->second == 1);
        CHECK(NTH(map, 3)->first == 20); CHECK(NTH(map, 3)->second == 2);
        CHECK(NTH(map, 4)->first == 20); CHECK(NTH(map, 4)->second == 3);
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
        sfl::small_unordered_flat_multimap<xint, xint, 32, std::equal_to<xint>, TPARAM_ALLOCATOR<std::pair<xint, xint>>> map
        (
            sfl::from_range_t(),
            std::views::all(data)
        );
        #else
        sfl::small_unordered_flat_multimap<xint, xint, 32, std::equal_to<xint>, TPARAM_ALLOCATOR<std::pair<xint, xint>>> map
        (
            sfl::from_range_t(),
            data
        );
        #endif

        CHECK(map.empty() == false);
        CHECK(map.size() == 5);
        CHECK(map.max_size() > 0);
        CHECK(NTH(map, 0)->first == 10); CHECK(NTH(map, 0)->second == 1);
        CHECK(NTH(map, 1)->first == 20); CHECK(NTH(map, 1)->second == 1);
        CHECK(NTH(map, 2)->first == 30); CHECK(NTH(map, 2)->second == 1);
        CHECK(NTH(map, 3)->first == 20); CHECK(NTH(map, 3)->second == 2);
        CHECK(NTH(map, 4)->first == 20); CHECK(NTH(map, 4)->second == 3);
    }
}

PRINT("Test container(sfl::from_range_t, Range&&, const KeyEqual&)");
{
    // Input iterator (exactly)
    {
        std::istringstream iss("10 1 20 1 30 1 20 2 20 3");

        std::equal_to<xint> equal;

        #if SFL_CPP_VERSION >= SFL_CPP_20
        sfl::small_unordered_flat_multimap<xint, xint, 32, std::equal_to<xint>, TPARAM_ALLOCATOR<std::pair<xint, xint>>> map
        (
            (sfl::from_range_t()),
            (std::views::istream<std::pair<int, int>>(iss)),
            equal
        );
        #else
        sfl::small_unordered_flat_multimap<xint, xint, 32, std::equal_to<xint>, TPARAM_ALLOCATOR<std::pair<xint, xint>>> map
        (
            (sfl::from_range_t()),
            (sfl::test::istream_view<std::pair<int, int>>(iss)),
            equal
        );
        #endif

        CHECK(map.empty() == false);
        CHECK(map.size() == 5);
        CHECK(map.max_size() > 0);
        CHECK(NTH(map, 0)->first == 10); CHECK(NTH(map, 0)->second == 1);
        CHECK(NTH(map, 1)->first == 20); CHECK(NTH(map, 1)->second == 1);
        CHECK(NTH(map, 2)->first == 30); CHECK(NTH(map, 2)->second == 1);
        CHECK(NTH(map, 3)->first == 20); CHECK(NTH(map, 3)->second == 2);
        CHECK(NTH(map, 4)->first == 20); CHECK(NTH(map, 4)->second == 3);
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

        std::equal_to<xint> equal;

        #if SFL_CPP_VERSION >= SFL_CPP_20
        sfl::small_unordered_flat_multimap<xint, xint, 32, std::equal_to<xint>, TPARAM_ALLOCATOR<std::pair<xint, xint>>> map
        (
            sfl::from_range_t(),
            std::views::all(data),
            equal
        );
        #else
        sfl::small_unordered_flat_multimap<xint, xint, 32, std::equal_to<xint>, TPARAM_ALLOCATOR<std::pair<xint, xint>>> map
        (
            sfl::from_range_t(),
            data,
            equal
        );
        #endif

        CHECK(map.empty() == false);
        CHECK(map.size() == 5);
        CHECK(map.max_size() > 0);
        CHECK(NTH(map, 0)->first == 10); CHECK(NTH(map, 0)->second == 1);
        CHECK(NTH(map, 1)->first == 20); CHECK(NTH(map, 1)->second == 1);
        CHECK(NTH(map, 2)->first == 30); CHECK(NTH(map, 2)->second == 1);
        CHECK(NTH(map, 3)->first == 20); CHECK(NTH(map, 3)->second == 2);
        CHECK(NTH(map, 4)->first == 20); CHECK(NTH(map, 4)->second == 3);
    }
}

PRINT("Test container(sfl::from_range_t, Range&&, const Allocator&)");
{
    // Input iterator (exactly)
    {
        std::istringstream iss("10 1 20 1 30 1 20 2 20 3");

        TPARAM_ALLOCATOR<std::pair<const xint, xint>> alloc;

        #if SFL_CPP_VERSION >= SFL_CPP_20
        sfl::small_unordered_flat_multimap<xint, xint, 32, std::equal_to<xint>, TPARAM_ALLOCATOR<std::pair<xint, xint>>> map
        (
            (sfl::from_range_t()),
            (std::views::istream<std::pair<int, int>>(iss)),
            alloc
        );
        #else
        sfl::small_unordered_flat_multimap<xint, xint, 32, std::equal_to<xint>, TPARAM_ALLOCATOR<std::pair<xint, xint>>> map
        (
            (sfl::from_range_t()),
            (sfl::test::istream_view<std::pair<int, int>>(iss)),
            alloc
        );
        #endif

        CHECK(map.empty() == false);
        CHECK(map.size() == 5);
        CHECK(map.max_size() > 0);
        CHECK(NTH(map, 0)->first == 10); CHECK(NTH(map, 0)->second == 1);
        CHECK(NTH(map, 1)->first == 20); CHECK(NTH(map, 1)->second == 1);
        CHECK(NTH(map, 2)->first == 30); CHECK(NTH(map, 2)->second == 1);
        CHECK(NTH(map, 3)->first == 20); CHECK(NTH(map, 3)->second == 2);
        CHECK(NTH(map, 4)->first == 20); CHECK(NTH(map, 4)->second == 3);
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

        TPARAM_ALLOCATOR<std::pair<const xint, xint>> alloc;

        #if SFL_CPP_VERSION >= SFL_CPP_20
        sfl::small_unordered_flat_multimap<xint, xint, 32, std::equal_to<xint>, TPARAM_ALLOCATOR<std::pair<xint, xint>>> map
        (
            sfl::from_range_t(),
            std::views::all(data),
            alloc
        );
        #else
        sfl::small_unordered_flat_multimap<xint, xint, 32, std::equal_to<xint>, TPARAM_ALLOCATOR<std::pair<xint, xint>>> map
        (
            sfl::from_range_t(),
            data,
            alloc
        );
        #endif

        CHECK(map.empty() == false);
        CHECK(map.size() == 5);
        CHECK(map.max_size() > 0);
        CHECK(NTH(map, 0)->first == 10); CHECK(NTH(map, 0)->second == 1);
        CHECK(NTH(map, 1)->first == 20); CHECK(NTH(map, 1)->second == 1);
        CHECK(NTH(map, 2)->first == 30); CHECK(NTH(map, 2)->second == 1);
        CHECK(NTH(map, 3)->first == 20); CHECK(NTH(map, 3)->second == 2);
        CHECK(NTH(map, 4)->first == 20); CHECK(NTH(map, 4)->second == 3);
    }
}

PRINT("Test container(sfl::from_range_t, Range&&, const KeyEqual&, const Allocator&)");
{
    // Input iterator (exactly)
    {
        std::istringstream iss("10 1 20 1 30 1 20 2 20 3");

        std::equal_to<xint> equal;
        TPARAM_ALLOCATOR<std::pair<const xint, xint>> alloc;

        #if SFL_CPP_VERSION >= SFL_CPP_20
        sfl::small_unordered_flat_multimap<xint, xint, 32, std::equal_to<xint>, TPARAM_ALLOCATOR<std::pair<xint, xint>>> map
        (
            (sfl::from_range_t()),
            (std::views::istream<std::pair<int, int>>(iss)),
            equal,
            alloc
        );
        #else
        sfl::small_unordered_flat_multimap<xint, xint, 32, std::equal_to<xint>, TPARAM_ALLOCATOR<std::pair<xint, xint>>> map
        (
            (sfl::from_range_t()),
            (sfl::test::istream_view<std::pair<int, int>>(iss)),
            equal,
            alloc
        );
        #endif

        CHECK(map.empty() == false);
        CHECK(map.size() == 5);
        CHECK(map.max_size() > 0);
        CHECK(NTH(map, 0)->first == 10); CHECK(NTH(map, 0)->second == 1);
        CHECK(NTH(map, 1)->first == 20); CHECK(NTH(map, 1)->second == 1);
        CHECK(NTH(map, 2)->first == 30); CHECK(NTH(map, 2)->second == 1);
        CHECK(NTH(map, 3)->first == 20); CHECK(NTH(map, 3)->second == 2);
        CHECK(NTH(map, 4)->first == 20); CHECK(NTH(map, 4)->second == 3);
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

        std::equal_to<xint> equal;
        TPARAM_ALLOCATOR<std::pair<const xint, xint>> alloc;

        #if SFL_CPP_VERSION >= SFL_CPP_20
        sfl::small_unordered_flat_multimap<xint, xint, 32, std::equal_to<xint>, TPARAM_ALLOCATOR<std::pair<xint, xint>>> map
        (
            sfl::from_range_t(),
            std::views::all(data),
            equal,
            alloc
        );
        #else
        sfl::small_unordered_flat_multimap<xint, xint, 32, std::equal_to<xint>, TPARAM_ALLOCATOR<std::pair<xint, xint>>> map
        (
            sfl::from_range_t(),
            data,
            equal,
            alloc
        );
        #endif

        CHECK(map.empty() == false);
        CHECK(map.size() == 5);
        CHECK(map.max_size() > 0);
        CHECK(NTH(map, 0)->first == 10); CHECK(NTH(map, 0)->second == 1);
        CHECK(NTH(map, 1)->first == 20); CHECK(NTH(map, 1)->second == 1);
        CHECK(NTH(map, 2)->first == 30); CHECK(NTH(map, 2)->second == 1);
        CHECK(NTH(map, 3)->first == 20); CHECK(NTH(map, 3)->second == 2);
        CHECK(NTH(map, 4)->first == 20); CHECK(NTH(map, 4)->second == 3);
    }
}

///////////////////////////////////////////////////////////////////////////////

PRINT("Test operator=(const container&)");
{
    // Permutation 1/8
    #define CONDITION map1.size() < map2.size()  &&  map1.size() < map2.capacity()  &&  map1.capacity() < map2.capacity()
    {
        sfl::small_unordered_flat_multimap< xint,
                                            xint,
                                            0,
                                            std::equal_to<xint>,
                                            TPARAM_ALLOCATOR<std::pair<xint, xint>> > map1, map2;

        map1.reserve(3);
        map1.emplace(10, 1);
        map1.emplace(20, 1);
        map1.emplace(30, 1);

        CHECK(map1.size() == 3);
        CHECK(map1.nth(0)->first == 10); CHECK(map1.nth(0)->second == 1);
        CHECK(map1.nth(1)->first == 20); CHECK(map1.nth(1)->second == 1);
        CHECK(map1.nth(2)->first == 30); CHECK(map1.nth(2)->second == 1);

        map2.reserve(5);
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

        ///////////////////////////////////////////////////////////////////////

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

    // Permutation 2/8
    #define CONDITION map1.size() < map2.size()  &&  map1.size() < map2.capacity()  &&  map1.capacity() > map2.capacity()
    {
        sfl::small_unordered_flat_multimap< xint,
                                            xint,
                                            0,
                                            std::equal_to<xint>,
                                            TPARAM_ALLOCATOR<std::pair<xint, xint>> > map1, map2;

        map1.reserve(100);
        map1.emplace(10, 1);
        map1.emplace(20, 1);
        map1.emplace(30, 1);

        CHECK(map1.size() == 3);
        CHECK(map1.nth(0)->first == 10); CHECK(map1.nth(0)->second == 1);
        CHECK(map1.nth(1)->first == 20); CHECK(map1.nth(1)->second == 1);
        CHECK(map1.nth(2)->first == 30); CHECK(map1.nth(2)->second == 1);

        map2.reserve(5);
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

        ///////////////////////////////////////////////////////////////////////

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

    // Permutation 3/8
    #define CONDITION map1.size() < map2.size()  &&  map1.size() > map2.capacity()  &&  map1.capacity() < map2.capacity()
    {
        // Impossible condition
    }
    #undef CONDITION

    // Permutation 4/8
    #define CONDITION map1.size() < map2.size()  &&  map1.size() > map2.capacity()  &&  map1.capacity() > map2.capacity()
    {
        // Impossible condition
    }
    #undef CONDITION

    // Permutation 5/8
    #define CONDITION map1.size() > map2.size()  &&  map1.size() < map2.capacity()  &&  map1.capacity() < map2.capacity()
    {
        sfl::small_unordered_flat_multimap< xint,
                                            xint,
                                            0,
                                            std::equal_to<xint>,
                                            TPARAM_ALLOCATOR<std::pair<xint, xint>> > map1, map2;

        map1.reserve(5);
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

        map2.reserve(100);
        map2.emplace(60, 2);
        map2.emplace(70, 2);
        map2.emplace(80, 2);

        CHECK(map2.size() == 3);
        CHECK(map2.nth(0)->first == 60); CHECK(map2.nth(0)->second == 2);
        CHECK(map2.nth(1)->first == 70); CHECK(map2.nth(1)->second == 2);
        CHECK(map2.nth(2)->first == 80); CHECK(map2.nth(2)->second == 2);

        ///////////////////////////////////////////////////////////////////////

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

    // Permutation 6/8
    #define CONDITION map1.size() > map2.size()  &&  map1.size() < map2.capacity()  &&  map1.capacity() > map2.capacity()
    {
        sfl::small_unordered_flat_multimap< xint,
                                            xint,
                                            0,
                                            std::equal_to<xint>,
                                            TPARAM_ALLOCATOR<std::pair<xint, xint>> > map1, map2;

        map1.reserve(100);
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

        map2.reserve(10);
        map2.emplace(60, 2);
        map2.emplace(70, 2);
        map2.emplace(80, 2);

        CHECK(map2.size() == 3);
        CHECK(map2.nth(0)->first == 60); CHECK(map2.nth(0)->second == 2);
        CHECK(map2.nth(1)->first == 70); CHECK(map2.nth(1)->second == 2);
        CHECK(map2.nth(2)->first == 80); CHECK(map2.nth(2)->second == 2);

        ///////////////////////////////////////////////////////////////////////

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

    // Permutation 7/8
    #define CONDITION map1.size() > map2.size()  &&  map1.size() > map2.capacity()  &&  map1.capacity() < map2.capacity()
    {
        // Impossible condition
    }
    #undef CONDITION

    // Permutation 8/8
    #define CONDITION map1.size() > map2.size()  &&  map1.size() > map2.capacity()  &&  map1.capacity() > map2.capacity()
    {
        sfl::small_unordered_flat_multimap< xint,
                                            xint,
                                            0,
                                            std::equal_to<xint>,
                                            TPARAM_ALLOCATOR<std::pair<xint, xint>> > map1, map2;

        map1.reserve(100);
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

        map2.reserve(3);
        map2.emplace(60, 2);
        map2.emplace(70, 2);
        map2.emplace(80, 2);

        CHECK(map2.size() == 3);
        CHECK(map2.nth(0)->first == 60); CHECK(map2.nth(0)->second == 2);
        CHECK(map2.nth(1)->first == 70); CHECK(map2.nth(1)->second == 2);
        CHECK(map2.nth(2)->first == 80); CHECK(map2.nth(2)->second == 2);

        ///////////////////////////////////////////////////////////////////////

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
    #define N 0
    {
        // Permutation 1/8
        #define CONDITION map2.capacity() != N  &&  map1.size() < map2.size()  &&  map1.size() < map2.capacity()  &&  map1.capacity() < map2.capacity()
        {
            sfl::small_unordered_flat_multimap< xint,
                                                xint,
                                                N,
                                                std::equal_to<xint>,
                                                TPARAM_ALLOCATOR<std::pair<xint, xint>> > map1, map2;

            map1.reserve(3);
            map1.emplace(10, 1);
            map1.emplace(20, 1);
            map1.emplace(30, 1);

            CHECK(map1.size() == 3);
            CHECK(map1.nth(0)->first == 10); CHECK(map1.nth(0)->second == 1);
            CHECK(map1.nth(1)->first == 20); CHECK(map1.nth(1)->second == 1);
            CHECK(map1.nth(2)->first == 30); CHECK(map1.nth(2)->second == 1);

            map2.reserve(5);
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

            ///////////////////////////////////////////////////////////////////////

            CHECK(CONDITION);

            map1 = std::move(map2);

            CHECK(map1.size() == 5);
            CHECK(map1.nth(0)->first == 40); CHECK(map1.nth(0)->second == 2);
            CHECK(map1.nth(1)->first == 50); CHECK(map1.nth(1)->second == 2);
            CHECK(map1.nth(2)->first == 60); CHECK(map1.nth(2)->second == 2);
            CHECK(map1.nth(3)->first == 70); CHECK(map1.nth(3)->second == 2);
            CHECK(map1.nth(4)->first == 80); CHECK(map1.nth(4)->second == 2);

            if (map2.size() == 5)
            {
                // Elements were moved one-by-one
                CHECK(map2.nth(0)->first == -40); CHECK(map2.nth(0)->second == -2);
                CHECK(map2.nth(1)->first == -50); CHECK(map2.nth(1)->second == -2);
                CHECK(map2.nth(2)->first == -60); CHECK(map2.nth(2)->second == -2);
                CHECK(map2.nth(3)->first == -70); CHECK(map2.nth(3)->second == -2);
                CHECK(map2.nth(4)->first == -80); CHECK(map2.nth(4)->second == -2);
            }
            else
            {
                // Storage was moved
                CHECK(map2.size() == 0);
            }
        }
        #undef CONDITION

        // Permutation 2/8
        #define CONDITION map2.capacity() != N  &&  map1.size() < map2.size()  &&  map1.size() < map2.capacity()  &&  map1.capacity() > map2.capacity()
        {
            sfl::small_unordered_flat_multimap< xint,
                                                xint,
                                                N,
                                                std::equal_to<xint>,
                                                TPARAM_ALLOCATOR<std::pair<xint, xint>> > map1, map2;

            map1.reserve(100);
            map1.emplace(10, 1);
            map1.emplace(20, 1);
            map1.emplace(30, 1);

            CHECK(map1.size() == 3);
            CHECK(map1.nth(0)->first == 10); CHECK(map1.nth(0)->second == 1);
            CHECK(map1.nth(1)->first == 20); CHECK(map1.nth(1)->second == 1);
            CHECK(map1.nth(2)->first == 30); CHECK(map1.nth(2)->second == 1);

            map2.reserve(5);
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

            ///////////////////////////////////////////////////////////////////////

            CHECK(CONDITION);

            map1 = std::move(map2);

            CHECK(map1.size() == 5);
            CHECK(map1.nth(0)->first == 40); CHECK(map1.nth(0)->second == 2);
            CHECK(map1.nth(1)->first == 50); CHECK(map1.nth(1)->second == 2);
            CHECK(map1.nth(2)->first == 60); CHECK(map1.nth(2)->second == 2);
            CHECK(map1.nth(3)->first == 70); CHECK(map1.nth(3)->second == 2);
            CHECK(map1.nth(4)->first == 80); CHECK(map1.nth(4)->second == 2);

            if (map2.size() == 5)
            {
                // Elements were moved one-by-one
                CHECK(map2.nth(0)->first == -40); CHECK(map2.nth(0)->second == -2);
                CHECK(map2.nth(1)->first == -50); CHECK(map2.nth(1)->second == -2);
                CHECK(map2.nth(2)->first == -60); CHECK(map2.nth(2)->second == -2);
                CHECK(map2.nth(3)->first == -70); CHECK(map2.nth(3)->second == -2);
                CHECK(map2.nth(4)->first == -80); CHECK(map2.nth(4)->second == -2);
            }
            else
            {
                // Storage was moved
                CHECK(map2.size() == 0);
            }
        }
        #undef CONDITION

        // Permutation 3/8
        #define CONDITION map2.capacity() != N  &&  map1.size() < map2.size()  &&  map1.size() > map2.capacity()  &&  map1.capacity() < map2.capacity()
        {
            // Impossible condition
        }
        #undef CONDITION

        // Permutation 4/8
        #define CONDITION map2.capacity() != N  &&  map1.size() < map2.size()  &&  map1.size() > map2.capacity()  &&  map1.capacity() > map2.capacity()
        {
            // Impossible condition
        }
        #undef CONDITION

        // Permutation 5/8
        #define CONDITION map2.capacity() != N  &&  map1.size() > map2.size()  &&  map1.size() < map2.capacity()  &&  map1.capacity() < map2.capacity()
        {
            sfl::small_unordered_flat_multimap< xint,
                                                xint,
                                                N,
                                                std::equal_to<xint>,
                                                TPARAM_ALLOCATOR<std::pair<xint, xint>> > map1, map2;

            map1.reserve(5);
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

            map2.reserve(100);
            map2.emplace(60, 2);
            map2.emplace(70, 2);
            map2.emplace(80, 2);

            CHECK(map2.size() == 3);
            CHECK(map2.nth(0)->first == 60); CHECK(map2.nth(0)->second == 2);
            CHECK(map2.nth(1)->first == 70); CHECK(map2.nth(1)->second == 2);
            CHECK(map2.nth(2)->first == 80); CHECK(map2.nth(2)->second == 2);

            ///////////////////////////////////////////////////////////////////////

            CHECK(CONDITION);

            map1 = std::move(map2);

            CHECK(map1.size() == 3);
            CHECK(map1.nth(0)->first == 60); CHECK(map1.nth(0)->second == 2);
            CHECK(map1.nth(1)->first == 70); CHECK(map1.nth(1)->second == 2);
            CHECK(map1.nth(2)->first == 80); CHECK(map1.nth(2)->second == 2);

            if (map2.size() == 3)
            {
                // Elements were moved one-by-one
                CHECK(map2.nth(0)->first == -60); CHECK(map2.nth(0)->second == -2);
                CHECK(map2.nth(1)->first == -70); CHECK(map2.nth(1)->second == -2);
                CHECK(map2.nth(2)->first == -80); CHECK(map2.nth(2)->second == -2);
            }
            else
            {
                // Storage was moved
                CHECK(map2.size() == 0);
            }
        }
        #undef CONDITION

        // Permutation 6/8
        #define CONDITION map2.capacity() != N  &&  map1.size() > map2.size()  &&  map1.size() < map2.capacity()  &&  map1.capacity() > map2.capacity()
        {
            sfl::small_unordered_flat_multimap< xint,
                                                xint,
                                                N,
                                                std::equal_to<xint>,
                                                TPARAM_ALLOCATOR<std::pair<xint, xint>> > map1, map2;

            map1.reserve(100);
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

            map2.reserve(10);
            map2.emplace(60, 2);
            map2.emplace(70, 2);
            map2.emplace(80, 2);

            CHECK(map2.size() == 3);
            CHECK(map2.nth(0)->first == 60); CHECK(map2.nth(0)->second == 2);
            CHECK(map2.nth(1)->first == 70); CHECK(map2.nth(1)->second == 2);
            CHECK(map2.nth(2)->first == 80); CHECK(map2.nth(2)->second == 2);

            ///////////////////////////////////////////////////////////////////////

            CHECK(CONDITION);

            map1 = std::move(map2);

            CHECK(map1.size() == 3);
            CHECK(map1.nth(0)->first == 60); CHECK(map1.nth(0)->second == 2);
            CHECK(map1.nth(1)->first == 70); CHECK(map1.nth(1)->second == 2);
            CHECK(map1.nth(2)->first == 80); CHECK(map1.nth(2)->second == 2);

            if (map2.size() == 3)
            {
                // Elements were moved one-by-one
                CHECK(map2.nth(0)->first == -60); CHECK(map2.nth(0)->second == -2);
                CHECK(map2.nth(1)->first == -70); CHECK(map2.nth(1)->second == -2);
                CHECK(map2.nth(2)->first == -80); CHECK(map2.nth(2)->second == -2);
            }
            else
            {
                // Storage was moved
                CHECK(map2.size() == 0);
            }
        }
        #undef CONDITION

        // Permutation 7/8
        #define CONDITION map2.capacity() != N  &&  map1.size() > map2.size()  &&  map1.size() > map2.capacity()  &&  map1.capacity() < map2.capacity()
        {
            // Impossible condition
        }
        #undef CONDITION

        // Permutation 8/8
        #define CONDITION map2.capacity() != N  &&  map1.size() > map2.size()  &&  map1.size() > map2.capacity()  &&  map1.capacity() > map2.capacity()
        {
            sfl::small_unordered_flat_multimap< xint,
                                                xint,
                                                N,
                                                std::equal_to<xint>,
                                                TPARAM_ALLOCATOR<std::pair<xint, xint>> > map1, map2;

            map1.reserve(5);
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

            map2.reserve(3);
            map2.emplace(60, 2);
            map2.emplace(70, 2);
            map2.emplace(80, 2);

            CHECK(map2.size() == 3);
            CHECK(map2.nth(0)->first == 60); CHECK(map2.nth(0)->second == 2);
            CHECK(map2.nth(1)->first == 70); CHECK(map2.nth(1)->second == 2);
            CHECK(map2.nth(2)->first == 80); CHECK(map2.nth(2)->second == 2);

            ///////////////////////////////////////////////////////////////////////

            CHECK(CONDITION);

            map1 = std::move(map2);

            CHECK(map1.size() == 3);
            CHECK(map1.nth(0)->first == 60); CHECK(map1.nth(0)->second == 2);
            CHECK(map1.nth(1)->first == 70); CHECK(map1.nth(1)->second == 2);
            CHECK(map1.nth(2)->first == 80); CHECK(map1.nth(2)->second == 2);

            if (map2.size() == 3)
            {
                // Elements were moved one-by-one
                CHECK(map2.nth(0)->first == -60); CHECK(map2.nth(0)->second == -2);
                CHECK(map2.nth(1)->first == -70); CHECK(map2.nth(1)->second == -2);
                CHECK(map2.nth(2)->first == -80); CHECK(map2.nth(2)->second == -2);
            }
            else
            {
                // Storage was moved
                CHECK(map2.size() == 0);
            }
        }
        #undef CONDITION
    }
    #undef N

    #define N 5
    {
        // Permutation 1/8
        #define CONDITION map2.capacity() == N  &&  map1.size() < map2.size()  &&  map1.size() < map2.capacity()  &&  map1.capacity() < map2.capacity()
        {
            // Impossible condition
        }
        #undef CONDITION

        // Permutation 2/8
        #define CONDITION map2.capacity() == N  &&  map1.size() < map2.size()  &&  map1.size() < map2.capacity()  &&  map1.capacity() > map2.capacity()
        {
            sfl::small_unordered_flat_multimap< xint,
                                                xint,
                                                N,
                                                std::equal_to<xint>,
                                                TPARAM_ALLOCATOR<std::pair<xint, xint>> > map1, map2;

            map1.reserve(100);
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

            ///////////////////////////////////////////////////////////////////////

            CHECK(CONDITION);

            map1 = std::move(map2);

            CHECK(map1.size() == 5);
            CHECK(map1.nth(0)->first == 40); CHECK(map1.nth(0)->second == 2);
            CHECK(map1.nth(1)->first == 50); CHECK(map1.nth(1)->second == 2);
            CHECK(map1.nth(2)->first == 60); CHECK(map1.nth(2)->second == 2);
            CHECK(map1.nth(3)->first == 70); CHECK(map1.nth(3)->second == 2);
            CHECK(map1.nth(4)->first == 80); CHECK(map1.nth(4)->second == 2);

            if (map2.size() == 5)
            {
                // Elements were moved one-by-one
                CHECK(map2.nth(0)->first == -40); CHECK(map2.nth(0)->second == -2);
                CHECK(map2.nth(1)->first == -50); CHECK(map2.nth(1)->second == -2);
                CHECK(map2.nth(2)->first == -60); CHECK(map2.nth(2)->second == -2);
                CHECK(map2.nth(3)->first == -70); CHECK(map2.nth(3)->second == -2);
                CHECK(map2.nth(4)->first == -80); CHECK(map2.nth(4)->second == -2);
            }
            else
            {
                // Storage was moved
                CHECK(map2.size() == 0);
            }
        }
        #undef CONDITION

        // Permutation 3/8
        #define CONDITION map2.capacity() == N  &&  map1.size() < map2.size()  &&  map1.size() > map2.capacity()  &&  map1.capacity() < map2.capacity()
        {
            // Impossible condition
        }
        #undef CONDITION

        // Permutation 4/8
        #define CONDITION map2.capacity() == N  &&  map1.size() < map2.size()  &&  map1.size() > map2.capacity()  &&  map1.capacity() > map2.capacity()
        {
            // Impossible condition
        }
        #undef CONDITION

        // Permutation 5/8
        #define CONDITION map2.capacity() == N  &&  map1.size() > map2.size()  &&  map1.size() < map2.capacity()  &&  map1.capacity() < map2.capacity()
        {
            // Impossible condition
        }
        #undef CONDITION

        // Permutation 6/8
        #define CONDITION map2.capacity() == N  &&  map1.size() > map2.size()  &&  map1.size() < map2.capacity()  &&  map1.capacity() > map2.capacity()
        {
            sfl::small_unordered_flat_multimap< xint,
                                                xint,
                                                N,
                                                std::equal_to<xint>,
                                                TPARAM_ALLOCATOR<std::pair<xint, xint>> > map1, map2;

            map1.reserve(100);
            map1.emplace(10, 1);
            map1.emplace(20, 1);
            map1.emplace(30, 1);
            map1.emplace(40, 1);

            CHECK(map1.size() == 4);
            CHECK(map1.nth(0)->first == 10); CHECK(map1.nth(0)->second == 1);
            CHECK(map1.nth(1)->first == 20); CHECK(map1.nth(1)->second == 1);
            CHECK(map1.nth(2)->first == 30); CHECK(map1.nth(2)->second == 1);
            CHECK(map1.nth(3)->first == 40); CHECK(map1.nth(3)->second == 1);

            map2.emplace(60, 2);
            map2.emplace(70, 2);
            map2.emplace(80, 2);

            CHECK(map2.size() == 3);
            CHECK(map2.nth(0)->first == 60); CHECK(map2.nth(0)->second == 2);
            CHECK(map2.nth(1)->first == 70); CHECK(map2.nth(1)->second == 2);
            CHECK(map2.nth(2)->first == 80); CHECK(map2.nth(2)->second == 2);

            ///////////////////////////////////////////////////////////////////////

            CHECK(CONDITION);

            map1 = std::move(map2);

            CHECK(map1.size() == 3);
            CHECK(map1.nth(0)->first == 60); CHECK(map1.nth(0)->second == 2);
            CHECK(map1.nth(1)->first == 70); CHECK(map1.nth(1)->second == 2);
            CHECK(map1.nth(2)->first == 80); CHECK(map1.nth(2)->second == 2);

            if (map2.size() == 3)
            {
                // Elements were moved one-by-one
                CHECK(map2.nth(0)->first == -60); CHECK(map2.nth(0)->second == -2);
                CHECK(map2.nth(1)->first == -70); CHECK(map2.nth(1)->second == -2);
                CHECK(map2.nth(2)->first == -80); CHECK(map2.nth(2)->second == -2);
            }
            else
            {
                // Storage was moved
                CHECK(map2.size() == 0);
            }
        }
        #undef CONDITION

        // Permutation 7/8
        #define CONDITION map2.capacity() == N  &&  map1.size() > map2.size()  &&  map1.size() > map2.capacity()  &&  map1.capacity() < map2.capacity()
        {
            // Impossible condition
        }
        #undef CONDITION

        // Permutation 8/8
        #define CONDITION map2.capacity() == N  &&  map1.size() > map2.size()  &&  map1.size() > map2.capacity()  &&  map1.capacity() > map2.capacity()
        {
            sfl::small_unordered_flat_multimap< xint,
                                                xint,
                                                N,
                                                std::equal_to<xint>,
                                                TPARAM_ALLOCATOR<std::pair<xint, xint>> > map1, map2;

            map1.reserve(100);
            map1.emplace(10, 1);
            map1.emplace(20, 1);
            map1.emplace(30, 1);
            map1.emplace(40, 1);
            map1.emplace(50, 1);
            map1.emplace(60, 1);

            CHECK(map1.size() == 6);
            CHECK(map1.nth(0)->first == 10); CHECK(map1.nth(0)->second == 1);
            CHECK(map1.nth(1)->first == 20); CHECK(map1.nth(1)->second == 1);
            CHECK(map1.nth(2)->first == 30); CHECK(map1.nth(2)->second == 1);
            CHECK(map1.nth(3)->first == 40); CHECK(map1.nth(3)->second == 1);
            CHECK(map1.nth(4)->first == 50); CHECK(map1.nth(4)->second == 1);
            CHECK(map1.nth(5)->first == 60); CHECK(map1.nth(5)->second == 1);

            map2.emplace(60, 2);
            map2.emplace(70, 2);
            map2.emplace(80, 2);

            CHECK(map2.size() == 3);
            CHECK(map2.nth(0)->first == 60); CHECK(map2.nth(0)->second == 2);
            CHECK(map2.nth(1)->first == 70); CHECK(map2.nth(1)->second == 2);
            CHECK(map2.nth(2)->first == 80); CHECK(map2.nth(2)->second == 2);

            ///////////////////////////////////////////////////////////////////////

            CHECK(CONDITION);

            map1 = std::move(map2);

            CHECK(map1.size() == 3);
            CHECK(map1.nth(0)->first == 60); CHECK(map1.nth(0)->second == 2);
            CHECK(map1.nth(1)->first == 70); CHECK(map1.nth(1)->second == 2);
            CHECK(map1.nth(2)->first == 80); CHECK(map1.nth(2)->second == 2);

            if (map2.size() == 3)
            {
                // Elements were moved one-by-one
                CHECK(map2.nth(0)->first == -60); CHECK(map2.nth(0)->second == -2);
                CHECK(map2.nth(1)->first == -70); CHECK(map2.nth(1)->second == -2);
                CHECK(map2.nth(2)->first == -80); CHECK(map2.nth(2)->second == -2);
            }
            else
            {
                // Storage was moved
                CHECK(map2.size() == 0);
            }
        }
        #undef CONDITION
    }
    #undef N
}

PRINT("Test operator=(std::initializer_list)");
{
    // Permutation 1/4
    #define CONDITION ilist.size() < map.size() && ilist.size() < map.capacity()
    {
        sfl::small_unordered_flat_multimap< xint,
                                            xint,
                                            5,
                                            std::equal_to<xint>,
                                            TPARAM_ALLOCATOR<std::pair<xint, xint>> > map;

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

        std::initializer_list<std::pair<xint, xint>> ilist
        {
            {60, 2},
            {70, 2},
            {80, 2}
        };

        ///////////////////////////////////////////////////////////////////////

        CHECK(CONDITION);

        map = ilist;

        CHECK(map.size() == 3);
        CHECK(map.nth(0)->first == 60); CHECK(map.nth(0)->second == 2);
        CHECK(map.nth(1)->first == 70); CHECK(map.nth(1)->second == 2);
        CHECK(map.nth(2)->first == 80); CHECK(map.nth(2)->second == 2);
    }
    #undef CONDITION

    // Permutation 2/4
    #define CONDITION ilist.size() < map.size() && ilist.size() > map.capacity()
    {
        // Impossible condition
    }
    #undef CONDITION

    // Permutation 3/4
    #define CONDITION ilist.size() > map.size() && ilist.size() < map.capacity()
    {
        sfl::small_unordered_flat_multimap< xint,
                                            xint,
                                            100,
                                            std::equal_to<xint>,
                                            TPARAM_ALLOCATOR<std::pair<xint, xint>> > map;

        map.emplace(10, 1);
        map.emplace(20, 1);
        map.emplace(30, 1);

        CHECK(map.size() == 3);
        CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
        CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
        CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        std::initializer_list<std::pair<xint, xint>> ilist
        {
            {40, 2},
            {50, 2},
            {60, 2},
            {70, 2},
            {80, 2}
        };

        ///////////////////////////////////////////////////////////////////////

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

    // Permutation 4/4
    #define CONDITION ilist.size() > map.size() && ilist.size() > map.capacity()
    {
        sfl::small_unordered_flat_multimap< xint,
                                            xint,
                                            3,
                                            std::equal_to<xint>,
                                            TPARAM_ALLOCATOR<std::pair<xint, xint>> > map;

        map.emplace(10, 1);
        map.emplace(20, 1);
        map.emplace(30, 1);

        CHECK(map.size() == 3);
        CHECK(map.nth(0)->first == 10); CHECK(map.nth(0)->second == 1);
        CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
        CHECK(map.nth(2)->first == 30); CHECK(map.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        std::initializer_list<std::pair<xint, xint>> ilist
        {
            {40, 2},
            {50, 2},
            {60, 2},
            {70, 2},
            {80, 2}
        };

        ///////////////////////////////////////////////////////////////////////

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
}

///////////////////////////////////////////////////////////////////////////////

PRINT("Test NON-MEMBER comparison operators");
{
    sfl::small_unordered_flat_multimap< xint,
                                        xint,
                                        100,
                                        std::equal_to<xint>,
                                        TPARAM_ALLOCATOR<std::pair<xint, xint>> > map1, map2, map3;

    map1.emplace(10, 1);
    map1.emplace(20, 1);
    map1.emplace(30, 1);

    map2.emplace(10, 1);
    map2.emplace(20, 1);
    map2.emplace(30, 1);
    map2.emplace(40, 1);
    map2.emplace(50, 1);

    map3.emplace(20, 1);
    map3.emplace(10, 1);
    map3.emplace(30, 1);
    map3.emplace(50, 1);
    map3.emplace(40, 1);

    CHECK((map1 == map1) == true);
    CHECK((map1 == map2) == false);
    CHECK((map2 == map1) == false);
    CHECK((map2 == map2) == true);

    CHECK((map1 != map1) == false);
    CHECK((map1 != map2) == true);
    CHECK((map2 != map1) == true);
    CHECK((map2 != map2) == false);

    CHECK((map2 == map3) == true);
    CHECK((map3 == map2) == true);
    CHECK((map2 != map3) == false);
    CHECK((map3 != map2) == false);
}

PRINT("Test NON-MEMBER swap(container&)");
{
    sfl::small_unordered_flat_multimap< xint,
                                        xint,
                                        100,
                                        std::equal_to<xint>,
                                        TPARAM_ALLOCATOR<std::pair<xint, xint>> > map1, map2;

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
        sfl::small_unordered_flat_multimap< xint,
                                            xint,
                                            100,
                                            std::equal_to<xint>,
                                            TPARAM_ALLOCATOR<std::pair<xint, xint>> >;

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
    CHECK(map.nth(1)->first == 20); CHECK(map.nth(1)->second == 1);
    CHECK(map.nth(2)->first == 20); CHECK(map.nth(2)->second == 2);
    CHECK(map.nth(3)->first == 20); CHECK(map.nth(3)->second == 3);
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
