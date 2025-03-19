#ifndef TPARAM_ALLOCATOR
#error Please define macro TPARAM_ALLOCATOR
#endif

using sfl::test::xint;
using sfl::test::xint_xint;
using sfl::test::xobj;

PRINT("Test PRIVATE member function emplace_back(Args&&...)");
{
    // N == 0
    {
        sfl::small_unordered_flat_set< xint_xint,
                                       0,
                                       std::equal_to<xint_xint>,
                                       TPARAM_ALLOCATOR<xint_xint> > set;

        {
            PRINT(">");
            auto res = set.emplace_back(10, 1);
            PRINT("<");

            CHECK(res == set.nth(0));
            CHECK(set.size() == 1);
            CHECK(set.capacity() == 1);
            CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        }

        {
            PRINT(">");
            auto res = set.emplace_back(20, 1);
            PRINT("<");

            CHECK(res == set.nth(1));
            CHECK(set.size() == 2);
            CHECK(set.capacity() == 2);
            CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
            CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
        }

        {
            PRINT(">");
            auto res = set.emplace_back(30, 1);
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
            auto res = set.emplace_back(40, 1);
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
            auto res = set.emplace_back(50, 1);
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
            auto res = set.emplace_back(60, 1);
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
    }

    // N == 3
    {
        sfl::small_unordered_flat_set< xint_xint,
                                       3,
                                       std::equal_to<xint_xint>,
                                       TPARAM_ALLOCATOR<xint_xint> > set;

        {
            PRINT(">");
            auto res = set.emplace_back(10, 1);
            PRINT("<");

            CHECK(res == set.nth(0));
            CHECK(set.size() == 1);
            CHECK(set.capacity() == 3);
            CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        }

        {
            PRINT(">");
            auto res = set.emplace_back(20, 1);
            PRINT("<");

            CHECK(res == set.nth(1));
            CHECK(set.size() == 2);
            CHECK(set.capacity() == 3);
            CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
            CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
        }

        {
            PRINT(">");
            auto res = set.emplace_back(30, 1);
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
            auto res = set.emplace_back(40, 1);
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
            auto res = set.emplace_back(50, 1);
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
            auto res = set.emplace_back(60, 1);
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
    }
}

///////////////////////////////////////////////////////////////////////////////

PRINT("Test begin, end, cbegin, cend, nth, index_of");
{
    sfl::small_unordered_flat_set< xint_xint,
                                   100,
                                   std::equal_to<xint_xint>,
                                   TPARAM_ALLOCATOR<xint_xint> > set;

    set.emplace_back(20, 1);
    set.emplace_back(40, 1);
    set.emplace_back(60, 1);

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
    CHECK((sfl::small_unordered_flat_set< xint_xint,
                                          100,
                                          std::equal_to<xint_xint>,
                                          TPARAM_ALLOCATOR<xint_xint> >::static_capacity == 100));
}

///////////////////////////////////////////////////////////////////////////////

PRINT("Test get_allocator()");
{
    sfl::small_unordered_flat_set< xint_xint,
                                   100,
                                   std::equal_to<xint_xint>,
                                   TPARAM_ALLOCATOR<xint_xint> > set;

    auto alloc = set.get_allocator();

    (void)alloc;
}

///////////////////////////////////////////////////////////////////////////////

PRINT("Test key_eq()");
{
    {
        sfl::small_unordered_flat_set< xint,
                                       100,
                                       std::equal_to<xint>,
                                       TPARAM_ALLOCATOR<xint> > set;

        auto key_eq = set.key_eq();

        CHECK(key_eq(10, 10) == true);
        CHECK(key_eq(10, 20) == false);
        CHECK(key_eq(20, 10) == false);
        CHECK(key_eq(20, 20) == true);
    }

    {
        sfl::small_unordered_flat_set< xobj,
                                       100,
                                       xobj::equal,
                                       TPARAM_ALLOCATOR<xobj> > set;

        auto key_eq = set.key_eq();

        CHECK(key_eq(xobj(10), 10) == true);
        CHECK(key_eq(xobj(10), 20) == false);
        CHECK(key_eq(xobj(20), 10) == false);
        CHECK(key_eq(xobj(20), 20) == true);
    }
}

///////////////////////////////////////////////////////////////////////////////

PRINT("Test find, count, contains");
{
    // xint
    {
        sfl::small_unordered_flat_set< xint,
                                       100,
                                       std::equal_to<xint>,
                                       TPARAM_ALLOCATOR<xint> > set;

        set.emplace_back(20);
        set.emplace_back(40);
        set.emplace_back(60);

        CHECK(set.size() == 3);
        CHECK(*set.nth(0) == 20);
        CHECK(*set.nth(1) == 40);
        CHECK(*set.nth(2) == 60);

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
        sfl::small_unordered_flat_set< xobj,
                                       100,
                                       xobj::equal,
                                       TPARAM_ALLOCATOR<xobj> > set;

        set.emplace_back(20);
        set.emplace_back(40);
        set.emplace_back(60);

        CHECK(set.size() == 3);
        CHECK(set.nth(0)->value() == 20);
        CHECK(set.nth(1)->value() == 40);
        CHECK(set.nth(2)->value() == 60);

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
    sfl::small_unordered_flat_set< xint_xint,
                                   100,
                                   std::equal_to<xint_xint>,
                                   TPARAM_ALLOCATOR<xint_xint> > set;

    CHECK(set.size() == 0);

    set.emplace_back(10, 1);
    set.emplace_back(20, 1);
    set.emplace_back(30, 1);

    CHECK(set.size() == 3);
    CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
    CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
    CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);

    set.clear();

    CHECK(set.size() == 0);

    set.emplace_back(40, 2);
    set.emplace_back(50, 2);
    set.emplace_back(60, 2);

    CHECK(set.size() == 3);
    CHECK(set.nth(0)->first == 40); CHECK(set.nth(0)->second == 2);
    CHECK(set.nth(1)->first == 50); CHECK(set.nth(1)->second == 2);
    CHECK(set.nth(2)->first == 60); CHECK(set.nth(2)->second == 2);

    set.clear();

    CHECK(set.size() == 0);
}

PRINT("Test emplace(Args&&...)");
{
    sfl::small_unordered_flat_set< xint_xint,
                                   100,
                                   std::equal_to<xint_xint>,
                                   TPARAM_ALLOCATOR<xint_xint> > set;

    {
        CHECK(set.emplace(10, 1) == std::make_pair(set.nth(0), true));
        CHECK(set.emplace(20, 1) == std::make_pair(set.nth(1), true));
        CHECK(set.emplace(30, 1) == std::make_pair(set.nth(2), true));

        CHECK(set.size() == 3);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
    }

    {
        CHECK(set.emplace(10, 2) == std::make_pair(set.nth(0), false));
        CHECK(set.emplace(20, 2) == std::make_pair(set.nth(1), false));
        CHECK(set.emplace(30, 2) == std::make_pair(set.nth(2), false));

        CHECK(set.size() == 3);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
    }
}

PRINT("Test emplace_hint(const_iterator, Args&&...)");
{
    sfl::small_unordered_flat_set< xint_xint,
                                   100,
                                   std::equal_to<xint_xint>,
                                   TPARAM_ALLOCATOR<xint_xint> > set;

    {
        CHECK(set.emplace_hint(set.begin(), 10, 1) == set.nth(0));
        CHECK(set.emplace_hint(set.begin(), 20, 1) == set.nth(1));
        CHECK(set.emplace_hint(set.begin(), 30, 1) == set.nth(2));

        CHECK(set.size() == 3);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
    }

    {
        CHECK(set.emplace_hint(set.begin(), 10, 2) == set.nth(0));
        CHECK(set.emplace_hint(set.begin(), 20, 2) == set.nth(1));
        CHECK(set.emplace_hint(set.begin(), 30, 2) == set.nth(2));

        CHECK(set.size() == 3);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
    }
}

PRINT("Test insert(const value_type&)");
{
    sfl::small_unordered_flat_set< xint_xint,
                                   100,
                                   std::equal_to<xint_xint>,
                                   TPARAM_ALLOCATOR<xint_xint> > set;

    using value_type = xint_xint;

    {
        value_type value_10_1(10, 1);
        value_type value_20_1(20, 1);
        value_type value_30_1(30, 1);

        CHECK(set.insert(value_10_1) == std::make_pair(set.nth(0), true));
        CHECK(set.insert(value_20_1) == std::make_pair(set.nth(1), true));
        CHECK(set.insert(value_30_1) == std::make_pair(set.nth(2), true));

        CHECK(set.size() == 3);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);

        CHECK(value_10_1.first == 10); CHECK(value_10_1.second == 1);
        CHECK(value_20_1.first == 20); CHECK(value_20_1.second == 1);
        CHECK(value_30_1.first == 30); CHECK(value_30_1.second == 1);
    }

    {
        value_type value_10_2(10, 2);
        value_type value_20_2(20, 2);
        value_type value_30_2(30, 2);

        CHECK(set.insert(value_10_2) == std::make_pair(set.nth(0), false));
        CHECK(set.insert(value_20_2) == std::make_pair(set.nth(1), false));
        CHECK(set.insert(value_30_2) == std::make_pair(set.nth(2), false));

        CHECK(set.size() == 3);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);

        CHECK(value_10_2.first == 10); CHECK(value_10_2.second == 2);
        CHECK(value_20_2.first == 20); CHECK(value_20_2.second == 2);
        CHECK(value_30_2.first == 30); CHECK(value_30_2.second == 2);
    }
}

PRINT("Test insert(value_type&&)");
{
    sfl::small_unordered_flat_set< xint_xint,
                                   100,
                                   std::equal_to<xint_xint>,
                                   TPARAM_ALLOCATOR<xint_xint> > set;

    using value_type = xint_xint;

    {
        value_type value_10_1(10, 1);
        value_type value_20_1(20, 1);
        value_type value_30_1(30, 1);

        CHECK(set.insert(std::move(value_10_1)) == std::make_pair(set.nth(0), true));
        CHECK(set.insert(std::move(value_20_1)) == std::make_pair(set.nth(1), true));
        CHECK(set.insert(std::move(value_30_1)) == std::make_pair(set.nth(2), true));

        CHECK(set.size() == 3);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);

        CHECK(value_10_1.first == -10); CHECK(value_10_1.second == -1);
        CHECK(value_20_1.first == -20); CHECK(value_20_1.second == -1);
        CHECK(value_30_1.first == -30); CHECK(value_30_1.second == -1);
    }

    {
        value_type value_10_2(10, 2);
        value_type value_20_2(20, 2);
        value_type value_30_2(30, 2);

        CHECK(set.insert(std::move(value_10_2)) == std::make_pair(set.nth(0), false));
        CHECK(set.insert(std::move(value_20_2)) == std::make_pair(set.nth(1), false));
        CHECK(set.insert(std::move(value_30_2)) == std::make_pair(set.nth(2), false));

        CHECK(set.size() == 3);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);

        CHECK(value_10_2.first == +10); CHECK(value_10_2.second == +2);
        CHECK(value_20_2.first == +20); CHECK(value_20_2.second == +2);
        CHECK(value_30_2.first == +30); CHECK(value_30_2.second == +2);
    }
}

PRINT("Test insert(K&&)");
{
    sfl::small_unordered_flat_set< xobj,
                                   100,
                                   xobj::equal,
                                   TPARAM_ALLOCATOR<xobj> > set;

    {
        CHECK(set.insert(10) == std::make_pair(set.nth(0), true));
        CHECK(set.insert(20) == std::make_pair(set.nth(1), true));
        CHECK(set.insert(30) == std::make_pair(set.nth(2), true));

        CHECK(set.size() == 3);
        CHECK(set.nth(0)->value() == 10);
        CHECK(set.nth(1)->value() == 20);
        CHECK(set.nth(2)->value() == 30);
    }

    {
        CHECK(set.insert(10) == std::make_pair(set.nth(0), false));
        CHECK(set.insert(20) == std::make_pair(set.nth(1), false));
        CHECK(set.insert(30) == std::make_pair(set.nth(2), false));

        CHECK(set.size() == 3);
        CHECK(set.nth(0)->value() == 10);
        CHECK(set.nth(1)->value() == 20);
        CHECK(set.nth(2)->value() == 30);
    }
}

PRINT("Test insert(const_iterator, const value_type&)");
{
    sfl::small_unordered_flat_set< xint_xint,
                                   100,
                                   std::equal_to<xint_xint>,
                                   TPARAM_ALLOCATOR<xint_xint> > set;

    using value_type = xint_xint;

    {
        value_type value_10_1(10, 1);
        value_type value_20_1(20, 1);
        value_type value_30_1(30, 1);

        CHECK(set.insert(set.begin(), value_10_1) == set.nth(0));
        CHECK(set.insert(set.begin(), value_20_1) == set.nth(1));
        CHECK(set.insert(set.begin(), value_30_1) == set.nth(2));

        CHECK(set.size() == 3);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);

        CHECK(value_10_1.first == 10); CHECK(value_10_1.second == 1);
        CHECK(value_20_1.first == 20); CHECK(value_20_1.second == 1);
        CHECK(value_30_1.first == 30); CHECK(value_30_1.second == 1);
    }

    {
        value_type value_10_2(10, 2);
        value_type value_20_2(20, 2);
        value_type value_30_2(30, 2);

        CHECK(set.insert(set.begin(), value_10_2) == set.nth(0));
        CHECK(set.insert(set.begin(), value_20_2) == set.nth(1));
        CHECK(set.insert(set.begin(), value_30_2) == set.nth(2));

        CHECK(set.size() == 3);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);

        CHECK(value_10_2.first == 10); CHECK(value_10_2.second == 2);
        CHECK(value_20_2.first == 20); CHECK(value_20_2.second == 2);
        CHECK(value_30_2.first == 30); CHECK(value_30_2.second == 2);
    }
}

PRINT("Test insert(const_iterator, value_type&&)");
{
    sfl::small_unordered_flat_set< xint_xint,
                                   100,
                                   std::equal_to<xint_xint>,
                                   TPARAM_ALLOCATOR<xint_xint> > set;

    using value_type = xint_xint;

    {
        value_type value_10_1(10, 1);
        value_type value_20_1(20, 1);
        value_type value_30_1(30, 1);

        CHECK(set.insert(set.begin(), std::move(value_10_1)) == set.nth(0));
        CHECK(set.insert(set.begin(), std::move(value_20_1)) == set.nth(1));
        CHECK(set.insert(set.begin(), std::move(value_30_1)) == set.nth(2));

        CHECK(set.size() == 3);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);

        CHECK(value_10_1.first == -10); CHECK(value_10_1.second == -1);
        CHECK(value_20_1.first == -20); CHECK(value_20_1.second == -1);
        CHECK(value_30_1.first == -30); CHECK(value_30_1.second == -1);
    }

    {
        value_type value_10_2(10, 2);
        value_type value_20_2(20, 2);
        value_type value_30_2(30, 2);

        CHECK(set.insert(set.begin(), std::move(value_10_2)) == set.nth(0));
        CHECK(set.insert(set.begin(), std::move(value_20_2)) == set.nth(1));
        CHECK(set.insert(set.begin(), std::move(value_30_2)) == set.nth(2));

        CHECK(set.size() == 3);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);

        CHECK(value_10_2.first == +10); CHECK(value_10_2.second == +2);
        CHECK(value_20_2.first == +20); CHECK(value_20_2.second == +2);
        CHECK(value_30_2.first == +30); CHECK(value_30_2.second == +2);
    }
}

PRINT("Test insert(const_iterator, K&&)");
{
    sfl::small_unordered_flat_set< xobj,
                                   100,
                                   xobj::equal,
                                   TPARAM_ALLOCATOR<xobj> > set;

    {
        CHECK(set.insert(set.begin(), 10) == set.nth(0));
        CHECK(set.insert(set.begin(), 20) == set.nth(1));
        CHECK(set.insert(set.begin(), 30) == set.nth(2));

        CHECK(set.size() == 3);
        CHECK(set.nth(0)->value() == 10);
        CHECK(set.nth(1)->value() == 20);
        CHECK(set.nth(2)->value() == 30);
    }

    {
        CHECK(set.insert(set.begin(), 10) == set.nth(0));
        CHECK(set.insert(set.begin(), 20) == set.nth(1));
        CHECK(set.insert(set.begin(), 30) == set.nth(2));

        CHECK(set.size() == 3);
        CHECK(set.nth(0)->value() == 10);
        CHECK(set.nth(1)->value() == 20);
        CHECK(set.nth(2)->value() == 30);
    }
}

PRINT("Test insert(InputIt, InputIt)");
{
    sfl::small_unordered_flat_set< xint_xint,
                                   100,
                                   std::equal_to<xint_xint>,
                                   TPARAM_ALLOCATOR<xint_xint> > set;

    {
        std::vector<xint_xint> data
        (
            {
                {10, 1},
                {20, 1},
                {30, 1}
            }
        );

        set.insert(data.begin(), data.end());

        CHECK(set.size() == 3);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
    }

    {
        std::vector<xint_xint> data
        (
            {
                {10, 2},
                {20, 2},
                {30, 2}
            }
        );

        set.insert(data.begin(), data.end());

        CHECK(set.size() == 3);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
    }
}

PRINT("Test insert(std::initializer_list)");
{
    sfl::small_unordered_flat_set< xint_xint,
                                   100,
                                   std::equal_to<xint_xint>,
                                   TPARAM_ALLOCATOR<xint_xint> > set;

    {
        std::initializer_list<xint_xint> ilist
        {
            {10, 1},
            {20, 1},
            {30, 1}
        };

        set.insert(ilist);

        CHECK(set.size() == 3);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
    }

    {
        std::initializer_list<xint_xint> ilist
        {
            {10, 2},
            {20, 2},
            {30, 2}
        };

        set.insert(ilist);

        CHECK(set.size() == 3);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
    }
}

PRINT("Test insert_range(Range&&");
{
    // Input iterator (exactly)
    {
        std::istringstream iss("10 20 30 20 20");

        sfl::small_unordered_flat_set<xint, 32, std::equal_to<xint>, TPARAM_ALLOCATOR<xint>> set;

        #if SFL_CPP_VERSION >= SFL_CPP_20
        set.insert_range(std::views::istream<int>(iss));
        #else
        set.insert_range(sfl::test::istream_view<int>(iss));
        #endif

        CHECK(set.size() == 3);
        CHECK(*NTH(set, 0) == 10);
        CHECK(*NTH(set, 1) == 20);
        CHECK(*NTH(set, 2) == 30);
    }

    // Forward iterator
    {
        std::vector<xint> data({10, 20, 30, 20, 20});

        sfl::small_unordered_flat_set<xint, 32, std::equal_to<xint>, TPARAM_ALLOCATOR<xint>> set;

        #if SFL_CPP_VERSION >= SFL_CPP_20
        set.insert_range(std::views::all(data));
        #else
        set.insert_range(data);
        #endif

        CHECK(set.size() == 3);
        CHECK(*NTH(set, 0) == 10);
        CHECK(*NTH(set, 1) == 20);
        CHECK(*NTH(set, 2) == 30);
    }
}

PRINT("Test erase(const_iterator)");
{
    // Erase at the end
    {
        sfl::small_unordered_flat_set< xint_xint,
                                       100,
                                       std::equal_to<xint_xint>,
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
        sfl::small_unordered_flat_set< xint_xint,
                                       100,
                                       std::equal_to<xint_xint>,
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
        CHECK(set.nth(0)->first == 50); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
        CHECK(set.nth(3)->first == 40); CHECK(set.nth(3)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.erase(set.nth(0)) == set.nth(0));
        CHECK(set.size() == 3);
        CHECK(set.nth(0)->first == 40); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.erase(set.nth(0)) == set.nth(0));
        CHECK(set.size() == 2);
        CHECK(set.nth(0)->first == 30); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.erase(set.nth(0)) == set.nth(0));
        CHECK(set.size() == 1);
        CHECK(set.nth(0)->first == 20); CHECK(set.nth(0)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.erase(set.nth(0)) == set.nth(0));
        CHECK(set.size() == 0);
    }

    // Erase near the end
    {
        sfl::small_unordered_flat_set< xint_xint,
                                       100,
                                       std::equal_to<xint_xint>,
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
        sfl::small_unordered_flat_set< xint_xint,
                                       100,
                                       std::equal_to<xint_xint>,
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
        CHECK(set.nth(1)->first == 50); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
        CHECK(set.nth(3)->first == 40); CHECK(set.nth(3)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.erase(set.nth(1)) == set.nth(1));
        CHECK(set.size() == 3);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 40); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.erase(set.nth(1)) == set.nth(1));
        CHECK(set.size() == 2);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 30); CHECK(set.nth(1)->second == 1);

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
        sfl::small_unordered_flat_set< xint_xint,
                                       100,
                                       std::equal_to<xint_xint>,
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
        sfl::small_unordered_flat_set< xint_xint,
                                       100,
                                       std::equal_to<xint_xint>,
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
        CHECK(set.nth(0)->first == 70); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 80); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 90); CHECK(set.nth(2)->second == 1);
        CHECK(set.nth(3)->first == 40); CHECK(set.nth(3)->second == 1);
        CHECK(set.nth(4)->first == 50); CHECK(set.nth(4)->second == 1);
        CHECK(set.nth(5)->first == 60); CHECK(set.nth(5)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.erase(set.nth(0), set.nth(3)) == set.nth(0));
        CHECK(set.size() == 3);
        CHECK(set.nth(0)->first == 40); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 50); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 60); CHECK(set.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.erase(set.nth(0), set.nth(3)) == set.nth(0));
        CHECK(set.size() == 0);
    }

    // Erase near the end
    {
        sfl::small_unordered_flat_set< xint_xint,
                                       100,
                                       std::equal_to<xint_xint>,
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
        sfl::small_unordered_flat_set< xint_xint,
                                       100,
                                       std::equal_to<xint_xint>,
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
        CHECK(set.nth(1)->first == 70); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 80); CHECK(set.nth(2)->second == 1);
        CHECK(set.nth(3)->first == 90); CHECK(set.nth(3)->second == 1);
        CHECK(set.nth(4)->first == 50); CHECK(set.nth(4)->second == 1);
        CHECK(set.nth(5)->first == 60); CHECK(set.nth(5)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.erase(set.nth(1), set.nth(4)) == set.nth(1));
        CHECK(set.size() == 3);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 50); CHECK(set.nth(1)->second == 1);
        CHECK(set.nth(2)->first == 60); CHECK(set.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.erase(set.nth(1), set.nth(2)) == set.nth(1));
        CHECK(set.size() == 2);
        CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
        CHECK(set.nth(1)->first == 60); CHECK(set.nth(1)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(set.erase(set.nth(0), set.nth(2)) == set.nth(0));
        CHECK(set.size() == 0);
    }
}

PRINT("Test erase(const Key&)");
{
    sfl::small_unordered_flat_set< xint,
                                   100,
                                   std::equal_to<xint>,
                                   TPARAM_ALLOCATOR<xint> > set;

    set.emplace(10);
    set.emplace(20);
    set.emplace(30);

    CHECK(set.size() == 3);
    CHECK(*set.nth(0) == 10);
    CHECK(*set.nth(1) == 20);
    CHECK(*set.nth(2) == 30);

    CHECK(set.erase(30) == 1);
    CHECK(set.erase(30) == 0);
    CHECK(set.size() == 2);
    CHECK(*set.nth(0) == 10);
    CHECK(*set.nth(1) == 20);

    CHECK(set.erase(20) == 1);
    CHECK(set.erase(20) == 0);
    CHECK(set.size() == 1);
    CHECK(*set.nth(0) == 10);

    CHECK(set.erase(10) == 1);
    CHECK(set.erase(10) == 0);
    CHECK(set.size() == 0);
}

PRINT("Test erase(K&&)");
{
    sfl::small_unordered_flat_set< xobj,
                                   100,
                                   xobj::equal,
                                   TPARAM_ALLOCATOR<xobj> > set;

    set.emplace(10);
    set.emplace(20);
    set.emplace(30);

    CHECK(set.size() == 3);
    CHECK(set.nth(0)->value() == 10);
    CHECK(set.nth(1)->value() == 20);
    CHECK(set.nth(2)->value() == 30);

    CHECK(set.erase(30) == 1);
    CHECK(set.erase(30) == 0);
    CHECK(set.size() == 2);
    CHECK(set.nth(0)->value() == 10);
    CHECK(set.nth(1)->value() == 20);

    CHECK(set.erase(20) == 1);
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
        sfl::small_unordered_flat_set< xint_xint,
                                       5,
                                       std::equal_to<xint_xint>,
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
        sfl::small_unordered_flat_set< xint_xint,
                                       5,
                                       std::equal_to<xint_xint>,
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
        sfl::small_unordered_flat_set< xint_xint,
                                       0,
                                       std::equal_to<xint_xint>,
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
    sfl::small_unordered_flat_set< xint_xint,
                                   100,
                                   std::equal_to<xint_xint>,
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
        sfl::small_unordered_flat_set< xint_xint,
                                       0,
                                       std::equal_to<xint_xint>,
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
        sfl::small_unordered_flat_set< xint_xint,
                                       100,
                                       std::equal_to<xint_xint>,
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
    sfl::small_unordered_flat_set< xint_xint,
                                   100,
                                   std::equal_to<xint_xint>,
                                   TPARAM_ALLOCATOR<xint_xint> > set;

    CHECK(set.size() == 0);
    CHECK(set.capacity() == 100);
    CHECK(set.available() == 100);
}

PRINT("Test container(const KeyEqual&)");
{
    std::equal_to<xint_xint> equal;

    sfl::small_unordered_flat_set< xint_xint,
                                   100,
                                   std::equal_to<xint_xint>,
                                   TPARAM_ALLOCATOR<xint_xint> > set(equal);

    CHECK(set.size() == 0);
    CHECK(set.capacity() == 100);
    CHECK(set.available() == 100);
}

PRINT("Test container(const Allocator&)");
{
    TPARAM_ALLOCATOR<xint_xint> alloc;

    sfl::small_unordered_flat_set< xint_xint,
                                   100,
                                   std::equal_to<xint_xint>,
                                   TPARAM_ALLOCATOR<xint_xint> > set(alloc);

    CHECK(set.size() == 0);
    CHECK(set.capacity() == 100);
    CHECK(set.available() == 100);
}

PRINT("Test container(const KeyEqual&, const Allocator&)");
{
    std::equal_to<xint_xint> equal;

    TPARAM_ALLOCATOR<xint_xint> alloc;

    sfl::small_unordered_flat_set< xint_xint,
                                   100,
                                   std::equal_to<xint_xint>,
                                   TPARAM_ALLOCATOR<xint_xint> > set(equal, alloc);

    CHECK(set.size() == 0);
    CHECK(set.capacity() == 100);
    CHECK(set.available() == 100);
}

PRINT("Test container(InputIt, InputIt)");
{
    std::vector<xint_xint> data
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

    sfl::small_unordered_flat_set< xint_xint,
                                   100,
                                   std::equal_to<xint_xint>,
                                   TPARAM_ALLOCATOR<xint_xint> > set(data.begin(), data.end());

    CHECK(set.size() == 3);
    CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
    CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
    CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
}

PRINT("Test container(InputIt, InputIt, const KeyEqual&)");
{
    std::vector<xint_xint> data
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

    std::equal_to<xint_xint> equal;

    sfl::small_unordered_flat_set< xint_xint,
                                   100,
                                   std::equal_to<xint_xint>,
                                   TPARAM_ALLOCATOR<xint_xint> > set(data.begin(), data.end(), equal);

    CHECK(set.size() == 3);
    CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
    CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
    CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
}

PRINT("Test container(InputIt, InputIt, const Allocator&)");
{
    std::vector<xint_xint> data
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

    TPARAM_ALLOCATOR<xint_xint> alloc;

    sfl::small_unordered_flat_set< xint_xint,
                                   100,
                                   std::equal_to<xint_xint>,
                                   TPARAM_ALLOCATOR<xint_xint> > set(data.begin(), data.end(), alloc);

    CHECK(set.size() == 3);
    CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
    CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
    CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
}

PRINT("Test container(InputIt, InputIt, const KeyEqual&, const Allocator&)");
{
    std::vector<xint_xint> data
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

    std::equal_to<xint_xint> equal;

    TPARAM_ALLOCATOR<xint_xint> alloc;

    sfl::small_unordered_flat_set< xint_xint,
                                   100,
                                   std::equal_to<xint_xint>,
                                   TPARAM_ALLOCATOR<xint_xint> > set(data.begin(), data.end(), equal, alloc);

    CHECK(set.size() == 3);
    CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
    CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
    CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
}

PRINT("Test container(std::initializer_list)");
{
    std::initializer_list<xint_xint> ilist
    {
        {10, 1},
        {20, 1},
        {30, 1},

        {10, 2},
        {20, 2},
        {30, 2}
    };

    sfl::small_unordered_flat_set< xint_xint,
                                   100,
                                   std::equal_to<xint_xint>,
                                   TPARAM_ALLOCATOR<xint_xint> > set(ilist);

    CHECK(set.size() == 3);
    CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
    CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
    CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
}

PRINT("Test container(std::initializer_list, const KeyEqual&)");
{
    std::initializer_list<xint_xint> ilist
    {
        {10, 1},
        {20, 1},
        {30, 1},

        {10, 2},
        {20, 2},
        {30, 2}
    };

    std::equal_to<xint_xint> equal;

    sfl::small_unordered_flat_set< xint_xint,
                                   100,
                                   std::equal_to<xint_xint>,
                                   TPARAM_ALLOCATOR<xint_xint> > set(ilist, equal);

    CHECK(set.size() == 3);
    CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
    CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
    CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
}

PRINT("Test container(std::initializer_list, const Allocator&)");
{
    std::initializer_list<xint_xint> ilist
    {
        {10, 1},
        {20, 1},
        {30, 1},

        {10, 2},
        {20, 2},
        {30, 2}
    };

    TPARAM_ALLOCATOR<xint_xint> alloc;

    sfl::small_unordered_flat_set< xint_xint,
                                   100,
                                   std::equal_to<xint_xint>,
                                   TPARAM_ALLOCATOR<xint_xint> > set(ilist, alloc);

    CHECK(set.size() == 3);
    CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
    CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
    CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
}

PRINT("Test container(std::initializer_list, const KeyEqual&, const Allocator&)");
{
    std::initializer_list<xint_xint> ilist
    {
        {10, 1},
        {20, 1},
        {30, 1},

        {10, 2},
        {20, 2},
        {30, 2}
    };

    std::equal_to<xint_xint> equal;

    TPARAM_ALLOCATOR<xint_xint> alloc;

    sfl::small_unordered_flat_set< xint_xint,
                                   100,
                                   std::equal_to<xint_xint>,
                                   TPARAM_ALLOCATOR<xint_xint> > set(ilist, equal, alloc);

    CHECK(set.size() == 3);
    CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
    CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
    CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);
}

PRINT("Test container(const container&)");
{
    // N == 0
    {
        sfl::small_unordered_flat_set< xint_xint,
                                       0,
                                       std::equal_to<xint_xint>,
                                       TPARAM_ALLOCATOR<xint_xint> > set1;

        set1.emplace(10, 1);
        set1.emplace(20, 1);
        set1.emplace(30, 1);

        CHECK(set1.size() == 3);
        CHECK(set1.nth(0)->first == 10); CHECK(set1.nth(0)->second == 1);
        CHECK(set1.nth(1)->first == 20); CHECK(set1.nth(1)->second == 1);
        CHECK(set1.nth(2)->first == 30); CHECK(set1.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        sfl::small_unordered_flat_set< xint_xint,
                                       0,
                                       std::equal_to<xint_xint>,
                                       TPARAM_ALLOCATOR<xint_xint> > set2(set1);

        CHECK(set2.size() == 3);
        CHECK(set2.nth(0)->first == 10); CHECK(set2.nth(0)->second == 1);
        CHECK(set2.nth(1)->first == 20); CHECK(set2.nth(1)->second == 1);
        CHECK(set2.nth(2)->first == 30); CHECK(set2.nth(2)->second == 1);
    }

    // N == 100
    {
        sfl::small_unordered_flat_set< xint_xint,
                                       100,
                                       std::equal_to<xint_xint>,
                                       TPARAM_ALLOCATOR<xint_xint> > set1;

        set1.emplace(10, 1);
        set1.emplace(20, 1);
        set1.emplace(30, 1);

        CHECK(set1.size() == 3);
        CHECK(set1.nth(0)->first == 10); CHECK(set1.nth(0)->second == 1);
        CHECK(set1.nth(1)->first == 20); CHECK(set1.nth(1)->second == 1);
        CHECK(set1.nth(2)->first == 30); CHECK(set1.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        sfl::small_unordered_flat_set< xint_xint,
                                       100,
                                       std::equal_to<xint_xint>,
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
        sfl::small_unordered_flat_set< xint_xint,
                                       0,
                                       std::equal_to<xint_xint>,
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

        sfl::small_unordered_flat_set< xint_xint,
                                       0,
                                       std::equal_to<xint_xint>,
                                       TPARAM_ALLOCATOR<xint_xint> > set2(set1, alloc);

        CHECK(set2.size() == 3);
        CHECK(set2.nth(0)->first == 10); CHECK(set2.nth(0)->second == 1);
        CHECK(set2.nth(1)->first == 20); CHECK(set2.nth(1)->second == 1);
        CHECK(set2.nth(2)->first == 30); CHECK(set2.nth(2)->second == 1);
    }

    // N == 100
    {
        sfl::small_unordered_flat_set< xint_xint,
                                       100,
                                       std::equal_to<xint_xint>,
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

        sfl::small_unordered_flat_set< xint_xint,
                                       100,
                                       std::equal_to<xint_xint>,
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
        sfl::small_unordered_flat_set< xint_xint,
                                       0,
                                       std::equal_to<xint_xint>,
                                       TPARAM_ALLOCATOR<xint_xint> > set1;

        set1.emplace(10, 1);
        set1.emplace(20, 1);
        set1.emplace(30, 1);

        CHECK(set1.size() == 3);
        CHECK(set1.nth(0)->first == 10); CHECK(set1.nth(0)->second == 1);
        CHECK(set1.nth(1)->first == 20); CHECK(set1.nth(1)->second == 1);
        CHECK(set1.nth(2)->first == 30); CHECK(set1.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        sfl::small_unordered_flat_set< xint_xint,
                                       0,
                                       std::equal_to<xint_xint>,
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
        sfl::small_unordered_flat_set< xint_xint,
                                       100,
                                       std::equal_to<xint_xint>,
                                       TPARAM_ALLOCATOR<xint_xint> > set1;

        set1.emplace(10, 1);
        set1.emplace(20, 1);
        set1.emplace(30, 1);

        CHECK(set1.size() == 3);
        CHECK(set1.nth(0)->first == 10); CHECK(set1.nth(0)->second == 1);
        CHECK(set1.nth(1)->first == 20); CHECK(set1.nth(1)->second == 1);
        CHECK(set1.nth(2)->first == 30); CHECK(set1.nth(2)->second == 1);

        ///////////////////////////////////////////////////////////////////////

        sfl::small_unordered_flat_set< xint_xint,
                                       100,
                                       std::equal_to<xint_xint>,
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
        sfl::small_unordered_flat_set< xint_xint,
                                       0,
                                       std::equal_to<xint_xint>,
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

        sfl::small_unordered_flat_set< xint_xint,
                                       0,
                                       std::equal_to<xint_xint>,
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
        sfl::small_unordered_flat_set< xint_xint,
                                       100,
                                       std::equal_to<xint_xint>,
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

        sfl::small_unordered_flat_set< xint_xint,
                                       100,
                                       std::equal_to<xint_xint>,
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
        sfl::small_unordered_flat_set<xint, 32, std::equal_to<xint>, TPARAM_ALLOCATOR<xint>> set
        (
            (sfl::from_range_t()),
            (std::views::istream<int>(iss))
        );
        #else
        sfl::small_unordered_flat_set<xint, 32, std::equal_to<xint>, TPARAM_ALLOCATOR<xint>> set
        (
            (sfl::from_range_t()),
            (sfl::test::istream_view<int>(iss))
        );
        #endif

        CHECK(set.empty() == false);
        CHECK(set.size() == 3);
        CHECK(set.max_size() > 0);
        CHECK(*NTH(set, 0) == 10);
        CHECK(*NTH(set, 1) == 20);
        CHECK(*NTH(set, 2) == 30);
    }

    // Forward iterator
    {
        std::vector<xint> data({10, 20, 30, 20, 20});

        #if SFL_CPP_VERSION >= SFL_CPP_20
        sfl::small_unordered_flat_set<xint, 32, std::equal_to<xint>, TPARAM_ALLOCATOR<xint>> set
        (
            sfl::from_range_t(),
            std::views::all(data)
        );
        #else
        sfl::small_unordered_flat_set<xint, 32, std::equal_to<xint>, TPARAM_ALLOCATOR<xint>> set
        (
            sfl::from_range_t(),
            data
        );
        #endif

        CHECK(set.empty() == false);
        CHECK(set.size() == 3);
        CHECK(set.max_size() > 0);
        CHECK(*NTH(set, 0) == 10);
        CHECK(*NTH(set, 1) == 20);
        CHECK(*NTH(set, 2) == 30);
    }
}

PRINT("Test container(sfl::from_range_t, Range&&, const KeyEqual&)");
{
    // Input iterator (exactly)
    {
        std::istringstream iss("10 20 30 20 20");

        std::equal_to<xint> equal;

        #if SFL_CPP_VERSION >= SFL_CPP_20
        sfl::small_unordered_flat_set<xint, 32, std::equal_to<xint>, TPARAM_ALLOCATOR<xint>> set
        (
            (sfl::from_range_t()),
            (std::views::istream<int>(iss)),
            equal
        );
        #else
        sfl::small_unordered_flat_set<xint, 32, std::equal_to<xint>, TPARAM_ALLOCATOR<xint>> set
        (
            (sfl::from_range_t()),
            (sfl::test::istream_view<int>(iss)),
            equal
        );
        #endif

        CHECK(set.empty() == false);
        CHECK(set.size() == 3);
        CHECK(set.max_size() > 0);
        CHECK(*NTH(set, 0) == 10);
        CHECK(*NTH(set, 1) == 20);
        CHECK(*NTH(set, 2) == 30);
    }

    // Forward iterator
    {
        std::vector<xint> data({10, 20, 30, 20, 20});

        std::equal_to<xint> equal;

        #if SFL_CPP_VERSION >= SFL_CPP_20
        sfl::small_unordered_flat_set<xint, 32, std::equal_to<xint>, TPARAM_ALLOCATOR<xint>> set
        (
            sfl::from_range_t(),
            std::views::all(data),
            equal
        );
        #else
        sfl::small_unordered_flat_set<xint, 32, std::equal_to<xint>, TPARAM_ALLOCATOR<xint>> set
        (
            sfl::from_range_t(),
            data,
            equal
        );
        #endif

        CHECK(set.empty() == false);
        CHECK(set.size() == 3);
        CHECK(set.max_size() > 0);
        CHECK(*NTH(set, 0) == 10);
        CHECK(*NTH(set, 1) == 20);
        CHECK(*NTH(set, 2) == 30);
    }
}

PRINT("Test container(sfl::from_range_t, Range&&, const Allocator&)");
{
    // Input iterator (exactly)
    {
        std::istringstream iss("10 20 30 20 20");

        TPARAM_ALLOCATOR<xint> alloc;

        #if SFL_CPP_VERSION >= SFL_CPP_20
        sfl::small_unordered_flat_set<xint, 32, std::equal_to<xint>, TPARAM_ALLOCATOR<xint>> set
        (
            (sfl::from_range_t()),
            (std::views::istream<int>(iss)),
            alloc
        );
        #else
        sfl::small_unordered_flat_set<xint, 32, std::equal_to<xint>, TPARAM_ALLOCATOR<xint>> set
        (
            (sfl::from_range_t()),
            (sfl::test::istream_view<int>(iss)),
            alloc
        );
        #endif

        CHECK(set.empty() == false);
        CHECK(set.size() == 3);
        CHECK(set.max_size() > 0);
        CHECK(*NTH(set, 0) == 10);
        CHECK(*NTH(set, 1) == 20);
        CHECK(*NTH(set, 2) == 30);
    }

    // Forward iterator
    {
        std::vector<xint> data({10, 20, 30, 20, 20});

        TPARAM_ALLOCATOR<xint> alloc;

        #if SFL_CPP_VERSION >= SFL_CPP_20
        sfl::small_unordered_flat_set<xint, 32, std::equal_to<xint>, TPARAM_ALLOCATOR<xint>> set
        (
            sfl::from_range_t(),
            std::views::all(data),
            alloc
        );
        #else
        sfl::small_unordered_flat_set<xint, 32, std::equal_to<xint>, TPARAM_ALLOCATOR<xint>> set
        (
            sfl::from_range_t(),
            data,
            alloc
        );
        #endif

        CHECK(set.empty() == false);
        CHECK(set.size() == 3);
        CHECK(set.max_size() > 0);
        CHECK(*NTH(set, 0) == 10);
        CHECK(*NTH(set, 1) == 20);
        CHECK(*NTH(set, 2) == 30);
    }
}

PRINT("Test container(sfl::from_range_t, Range&&, const KeyEqual&, const Allocator&)");
{
    // Input iterator (exactly)
    {
        std::istringstream iss("10 20 30 20 20");

        std::equal_to<xint> equal;
        TPARAM_ALLOCATOR<xint> alloc;

        #if SFL_CPP_VERSION >= SFL_CPP_20
        sfl::small_unordered_flat_set<xint, 32, std::equal_to<xint>, TPARAM_ALLOCATOR<xint>> set
        (
            (sfl::from_range_t()),
            (std::views::istream<int>(iss)),
            equal,
            alloc
        );
        #else
        sfl::small_unordered_flat_set<xint, 32, std::equal_to<xint>, TPARAM_ALLOCATOR<xint>> set
        (
            (sfl::from_range_t()),
            (sfl::test::istream_view<int>(iss)),
            equal,
            alloc
        );
        #endif

        CHECK(set.empty() == false);
        CHECK(set.size() == 3);
        CHECK(set.max_size() > 0);
        CHECK(*NTH(set, 0) == 10);
        CHECK(*NTH(set, 1) == 20);
        CHECK(*NTH(set, 2) == 30);
    }

    // Forward iterator
    {
        std::vector<xint> data({10, 20, 30, 20, 20});

        std::equal_to<xint> equal;
        TPARAM_ALLOCATOR<xint> alloc;

        #if SFL_CPP_VERSION >= SFL_CPP_20
        sfl::small_unordered_flat_set<xint, 32, std::equal_to<xint>, TPARAM_ALLOCATOR<xint>> set
        (
            sfl::from_range_t(),
            std::views::all(data),
            equal,
            alloc
        );
        #else
        sfl::small_unordered_flat_set<xint, 32, std::equal_to<xint>, TPARAM_ALLOCATOR<xint>> set
        (
            sfl::from_range_t(),
            data,
            equal,
            alloc
        );
        #endif

        CHECK(set.empty() == false);
        CHECK(set.size() == 3);
        CHECK(set.max_size() > 0);
        CHECK(*NTH(set, 0) == 10);
        CHECK(*NTH(set, 1) == 20);
        CHECK(*NTH(set, 2) == 30);
    }
}

///////////////////////////////////////////////////////////////////////////////

PRINT("Test operator=(const container&)");
{
    // Permutation 1/8
    #define CONDITION set1.size() < set2.size()  &&  set1.size() < set2.capacity()  &&  set1.capacity() < set2.capacity()
    {
        sfl::small_unordered_flat_set< xint_xint,
                                       0,
                                       std::equal_to<xint_xint>,
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
        sfl::small_unordered_flat_set< xint_xint,
                                       0,
                                       std::equal_to<xint_xint>,
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
        sfl::small_unordered_flat_set< xint_xint,
                                       0,
                                       std::equal_to<xint_xint>,
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
        sfl::small_unordered_flat_set< xint_xint,
                                       0,
                                       std::equal_to<xint_xint>,
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
        sfl::small_unordered_flat_set< xint_xint,
                                       0,
                                       std::equal_to<xint_xint>,
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
            sfl::small_unordered_flat_set< xint_xint,
                                           N,
                                           std::equal_to<xint_xint>,
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
            sfl::small_unordered_flat_set< xint_xint,
                                           N,
                                           std::equal_to<xint_xint>,
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
            sfl::small_unordered_flat_set< xint_xint,
                                           N,
                                           std::equal_to<xint_xint>,
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
            sfl::small_unordered_flat_set< xint_xint,
                                           N,
                                           std::equal_to<xint_xint>,
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
            sfl::small_unordered_flat_set< xint_xint,
                                           N,
                                           std::equal_to<xint_xint>,
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
            sfl::small_unordered_flat_set< xint_xint,
                                           N,
                                           std::equal_to<xint_xint>,
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
            sfl::small_unordered_flat_set< xint_xint,
                                           N,
                                           std::equal_to<xint_xint>,
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
            sfl::small_unordered_flat_set< xint_xint,
                                           N,
                                           std::equal_to<xint_xint>,
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
        sfl::small_unordered_flat_set< xint_xint,
                                       5,
                                       std::equal_to<xint_xint>,
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
        sfl::small_unordered_flat_set< xint_xint,
                                       100,
                                       std::equal_to<xint_xint>,
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
        sfl::small_unordered_flat_set< xint_xint,
                                       3,
                                       std::equal_to<xint_xint>,
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
    sfl::small_unordered_flat_set< xint_xint,
                                   100,
                                   std::equal_to<xint_xint>,
                                   TPARAM_ALLOCATOR<xint_xint> > set1, set2, set3;

    set1.emplace(10, 1);
    set1.emplace(20, 1);
    set1.emplace(30, 1);

    set2.emplace(10, 1);
    set2.emplace(20, 1);
    set2.emplace(30, 1);
    set2.emplace(40, 1);
    set2.emplace(50, 1);

    set3.emplace(20, 1);
    set3.emplace(10, 1);
    set3.emplace(30, 1);
    set3.emplace(50, 1);
    set3.emplace(40, 1);

    CHECK((set1 == set1) == true);
    CHECK((set1 == set2) == false);
    CHECK((set2 == set1) == false);
    CHECK((set2 == set2) == true);

    CHECK((set1 != set1) == false);
    CHECK((set1 != set2) == true);
    CHECK((set2 != set1) == true);
    CHECK((set2 != set2) == false);

    CHECK((set2 == set3) == true);
    CHECK((set3 == set2) == true);
    CHECK((set2 != set3) == false);
    CHECK((set3 != set2) == false);
}

PRINT("Test NON-MEMBER swap(container&)");
{
    sfl::small_unordered_flat_set< xint_xint,
                                   100,
                                   std::equal_to<xint_xint>,
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
        sfl::small_unordered_flat_set< xint_xint,
                                       100,
                                       std::equal_to<xint_xint>,
                                       TPARAM_ALLOCATOR<xint_xint> >;

    using const_reference = typename container_type::const_reference;

    ///////////////////////////////////////////////////////////////////////////

    container_type set;

    set.emplace(10, 1);
    set.emplace(20, 1);
    set.emplace(30, 1);

    CHECK(set.size() == 3);
    CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
    CHECK(set.nth(1)->first == 20); CHECK(set.nth(1)->second == 1);
    CHECK(set.nth(2)->first == 30); CHECK(set.nth(2)->second == 1);

    ///////////////////////////////////////////////////////////////////////////

    CHECK(erase_if(set, [](const_reference& value){ return value.first == 20; }) == 1);
    CHECK(set.size() == 2);
    CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
    CHECK(set.nth(1)->first == 30); CHECK(set.nth(1)->second == 1);

    ///////////////////////////////////////////////////////////////////////////

    CHECK(erase_if(set, [](const_reference& value){ return value.first == 20; }) == 0);
    CHECK(set.size() == 2);
    CHECK(set.nth(0)->first == 10); CHECK(set.nth(0)->second == 1);
    CHECK(set.nth(1)->first == 30); CHECK(set.nth(1)->second == 1);
}
