#ifndef TPARAM_N
#error Please define macro TPARAM_N
#endif

#ifndef TPARAM_ALLOCATOR
#error Please define macro TPARAM_ALLOCATOR
#endif

using sfl::test::xint;

PRINT("Test emplace_front(Args&&...)");
{
    sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

    constexpr int n = 100;

    for (int i = 0; i < n; ++i)
    {
        PRINT(">");
        const auto res = vec.emplace_front(i);
        PRINT("<");

        CHECK(res == i);
        CHECK(int(vec.size()) == i + 1);

        for (int j = 0; j < int(vec.size()); ++j)
        {
            CHECK(*vec.nth(j) == i - j);
        }
    }
}

PRINT("Test emplace_back(Args&&...)");
{
    sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

    constexpr int n = 100;

    for (int i = 0; i < n; ++i)
    {
        PRINT(">");
        const auto res = vec.emplace_back(i);
        PRINT("<");

        CHECK(res == i);
        CHECK(int(vec.size()) == i + 1);

        for (int j = 0; j < int(vec.size()); ++j)
        {
            CHECK(*vec.nth(j) == j);
        }
    }
}

PRINT("Test emplace(const_iterator, Args&&...)");
{
    // Insert at the end
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        {
            PRINT(">");
            const auto res = vec.emplace(vec.end(), 10);
            PRINT("<");

            CHECK(res == vec.nth(0));
            CHECK(vec.size() == 1);
            CHECK(*vec.nth(0) == 10);
        }

        {
            PRINT(">");
            const auto res = vec.emplace(vec.end(), 20);
            PRINT("<");

            CHECK(res == vec.nth(1));
            CHECK(vec.size() == 2);
            CHECK(*vec.nth(0) == 10);
            CHECK(*vec.nth(1) == 20);
        }

        {
            PRINT(">");
            const auto res = vec.emplace(vec.end(), 30);
            PRINT("<");

            CHECK(res == vec.nth(2));
            CHECK(vec.size() == 3);
            CHECK(*vec.nth(0) == 10);
            CHECK(*vec.nth(1) == 20);
            CHECK(*vec.nth(2) == 30);
        }

        {
            PRINT(">");
            const auto res = vec.emplace(vec.end(), 40);
            PRINT("<");

            CHECK(res == vec.nth(3));
            CHECK(vec.size() == 4);
            CHECK(*vec.nth(0) == 10);
            CHECK(*vec.nth(1) == 20);
            CHECK(*vec.nth(2) == 30);
            CHECK(*vec.nth(3) == 40);
        }

        {
            PRINT(">");
            const auto res = vec.emplace(vec.end(), 50);
            PRINT("<");

            CHECK(res == vec.nth(4));
            CHECK(vec.size() == 5);
            CHECK(*vec.nth(0) == 10);
            CHECK(*vec.nth(1) == 20);
            CHECK(*vec.nth(2) == 30);
            CHECK(*vec.nth(3) == 40);
            CHECK(*vec.nth(4) == 50);
        }

        {
            PRINT(">");
            const auto res = vec.emplace(vec.end(), 60);
            PRINT("<");

            CHECK(res == vec.nth(5));
            CHECK(vec.size() == 6);
            CHECK(*vec.nth(0) == 10);
            CHECK(*vec.nth(1) == 20);
            CHECK(*vec.nth(2) == 30);
            CHECK(*vec.nth(3) == 40);
            CHECK(*vec.nth(4) == 50);
            CHECK(*vec.nth(5) == 60);
        }

        {
            PRINT(">");
            const auto res = vec.emplace(vec.end(), 70);
            PRINT("<");

            CHECK(res == vec.nth(6));
            CHECK(vec.size() == 7);
            CHECK(*vec.nth(0) == 10);
            CHECK(*vec.nth(1) == 20);
            CHECK(*vec.nth(2) == 30);
            CHECK(*vec.nth(3) == 40);
            CHECK(*vec.nth(4) == 50);
            CHECK(*vec.nth(5) == 60);
            CHECK(*vec.nth(6) == 70);
        }

        {
            PRINT(">");
            const auto res = vec.emplace(vec.end(), 80);
            PRINT("<");

            CHECK(res == vec.nth(7));
            CHECK(vec.size() == 8);
            CHECK(*vec.nth(0) == 10);
            CHECK(*vec.nth(1) == 20);
            CHECK(*vec.nth(2) == 30);
            CHECK(*vec.nth(3) == 40);
            CHECK(*vec.nth(4) == 50);
            CHECK(*vec.nth(5) == 60);
            CHECK(*vec.nth(6) == 70);
            CHECK(*vec.nth(7) == 80);
        }

        {
            PRINT(">");
            const auto res = vec.emplace(vec.end(), 90);
            PRINT("<");

            CHECK(res == vec.nth(8));
            CHECK(vec.size() == 9);
            CHECK(*vec.nth(0) == 10);
            CHECK(*vec.nth(1) == 20);
            CHECK(*vec.nth(2) == 30);
            CHECK(*vec.nth(3) == 40);
            CHECK(*vec.nth(4) == 50);
            CHECK(*vec.nth(5) == 60);
            CHECK(*vec.nth(6) == 70);
            CHECK(*vec.nth(7) == 80);
            CHECK(*vec.nth(8) == 90);
        }
    }

    // Insert at the begin
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        {
            PRINT(">");
            const auto res = vec.emplace(vec.begin(), 90);
            PRINT("<");

            CHECK(res == vec.nth(0));
            CHECK(vec.size() == 1);
            CHECK(*vec.nth(0) == 90);
        }

        {
            PRINT(">");
            const auto res = vec.emplace(vec.begin(), 80);
            PRINT("<");

            CHECK(res == vec.nth(0));
            CHECK(vec.size() == 2);
            CHECK(*vec.nth(0) == 80);
            CHECK(*vec.nth(1) == 90);
        }

        {
            PRINT(">");
            const auto res = vec.emplace(vec.begin(), 70);
            PRINT("<");

            CHECK(res == vec.nth(0));
            CHECK(vec.size() == 3);
            CHECK(*vec.nth(0) == 70);
            CHECK(*vec.nth(1) == 80);
            CHECK(*vec.nth(2) == 90);
        }

        {
            PRINT(">");
            const auto res = vec.emplace(vec.begin(), 60);
            PRINT("<");

            CHECK(res == vec.nth(0));
            CHECK(vec.size() == 4);
            CHECK(*vec.nth(0) == 60);
            CHECK(*vec.nth(1) == 70);
            CHECK(*vec.nth(2) == 80);
            CHECK(*vec.nth(3) == 90);
        }

        {
            PRINT(">");
            const auto res = vec.emplace(vec.begin(), 50);
            PRINT("<");

            CHECK(res == vec.nth(0));
            CHECK(vec.size() == 5);
            CHECK(*vec.nth(0) == 50);
            CHECK(*vec.nth(1) == 60);
            CHECK(*vec.nth(2) == 70);
            CHECK(*vec.nth(3) == 80);
            CHECK(*vec.nth(4) == 90);
        }

        {
            PRINT(">");
            const auto res = vec.emplace(vec.begin(), 40);
            PRINT("<");

            CHECK(res == vec.nth(0));
            CHECK(vec.size() == 6);
            CHECK(*vec.nth(0) == 40);
            CHECK(*vec.nth(1) == 50);
            CHECK(*vec.nth(2) == 60);
            CHECK(*vec.nth(3) == 70);
            CHECK(*vec.nth(4) == 80);
            CHECK(*vec.nth(5) == 90);
        }

        {
            PRINT(">");
            const auto res = vec.emplace(vec.begin(), 30);
            PRINT("<");

            CHECK(res == vec.nth(0));
            CHECK(vec.size() == 7);
            CHECK(*vec.nth(0) == 30);
            CHECK(*vec.nth(1) == 40);
            CHECK(*vec.nth(2) == 50);
            CHECK(*vec.nth(3) == 60);
            CHECK(*vec.nth(4) == 70);
            CHECK(*vec.nth(5) == 80);
            CHECK(*vec.nth(6) == 90);
        }

        {
            PRINT(">");
            const auto res = vec.emplace(vec.begin(), 20);
            PRINT("<");

            CHECK(res == vec.nth(0));
            CHECK(vec.size() == 8);
            CHECK(*vec.nth(0) == 20);
            CHECK(*vec.nth(1) == 30);
            CHECK(*vec.nth(2) == 40);
            CHECK(*vec.nth(3) == 50);
            CHECK(*vec.nth(4) == 60);
            CHECK(*vec.nth(5) == 70);
            CHECK(*vec.nth(6) == 80);
            CHECK(*vec.nth(7) == 90);
        }

        {
            PRINT(">");
            const auto res = vec.emplace(vec.begin(), 10);
            PRINT("<");

            CHECK(res == vec.nth(0));
            CHECK(vec.size() == 9);
            CHECK(*vec.nth(0) == 10);
            CHECK(*vec.nth(1) == 20);
            CHECK(*vec.nth(2) == 30);
            CHECK(*vec.nth(3) == 40);
            CHECK(*vec.nth(4) == 50);
            CHECK(*vec.nth(5) == 60);
            CHECK(*vec.nth(6) == 70);
            CHECK(*vec.nth(7) == 80);
            CHECK(*vec.nth(8) == 90);
        }
    }

    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        CHECK(vec.size() == 9);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
        CHECK(*vec.nth(6) == 70);
        CHECK(*vec.nth(7) == 80);
        CHECK(*vec.nth(8) == 90);

        auto res = vec.emplace(vec.nth(0), 5);

        CHECK(res == vec.nth(0));
        CHECK(vec.size() == 10);
        CHECK(*vec.nth(0) ==  5);
        CHECK(*vec.nth(1) == 10);
        CHECK(*vec.nth(2) == 20);
        CHECK(*vec.nth(3) == 30);
        CHECK(*vec.nth(4) == 40);
        CHECK(*vec.nth(5) == 50);
        CHECK(*vec.nth(6) == 60);
        CHECK(*vec.nth(7) == 70);
        CHECK(*vec.nth(8) == 80);
        CHECK(*vec.nth(9) == 90);
    }

    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        CHECK(vec.size() == 9);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
        CHECK(*vec.nth(6) == 70);
        CHECK(*vec.nth(7) == 80);
        CHECK(*vec.nth(8) == 90);

        auto res = vec.emplace(vec.nth(1), 15);

        CHECK(res == vec.nth(1));
        CHECK(vec.size() == 10);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 15);
        CHECK(*vec.nth(2) == 20);
        CHECK(*vec.nth(3) == 30);
        CHECK(*vec.nth(4) == 40);
        CHECK(*vec.nth(5) == 50);
        CHECK(*vec.nth(6) == 60);
        CHECK(*vec.nth(7) == 70);
        CHECK(*vec.nth(8) == 80);
        CHECK(*vec.nth(9) == 90);
    }

    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        CHECK(vec.size() == 9);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
        CHECK(*vec.nth(6) == 70);
        CHECK(*vec.nth(7) == 80);
        CHECK(*vec.nth(8) == 90);

        auto res = vec.emplace(vec.nth(2), 25);

        CHECK(res == vec.nth(2));
        CHECK(vec.size() == 10);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 25);
        CHECK(*vec.nth(3) == 30);
        CHECK(*vec.nth(4) == 40);
        CHECK(*vec.nth(5) == 50);
        CHECK(*vec.nth(6) == 60);
        CHECK(*vec.nth(7) == 70);
        CHECK(*vec.nth(8) == 80);
        CHECK(*vec.nth(9) == 90);
    }

    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        CHECK(vec.size() == 9);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
        CHECK(*vec.nth(6) == 70);
        CHECK(*vec.nth(7) == 80);
        CHECK(*vec.nth(8) == 90);

        auto res = vec.emplace(vec.nth(3), 35);

        CHECK(res == vec.nth(3));
        CHECK(vec.size() == 10);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 35);
        CHECK(*vec.nth(4) == 40);
        CHECK(*vec.nth(5) == 50);
        CHECK(*vec.nth(6) == 60);
        CHECK(*vec.nth(7) == 70);
        CHECK(*vec.nth(8) == 80);
        CHECK(*vec.nth(9) == 90);
    }

    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        CHECK(vec.size() == 9);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
        CHECK(*vec.nth(6) == 70);
        CHECK(*vec.nth(7) == 80);
        CHECK(*vec.nth(8) == 90);

        auto res = vec.emplace(vec.nth(4), 45);

        CHECK(res == vec.nth(4));
        CHECK(vec.size() == 10);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 45);
        CHECK(*vec.nth(5) == 50);
        CHECK(*vec.nth(6) == 60);
        CHECK(*vec.nth(7) == 70);
        CHECK(*vec.nth(8) == 80);
        CHECK(*vec.nth(9) == 90);
    }

    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        CHECK(vec.size() == 9);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
        CHECK(*vec.nth(6) == 70);
        CHECK(*vec.nth(7) == 80);
        CHECK(*vec.nth(8) == 90);

        auto res = vec.emplace(vec.nth(5), 55);

        CHECK(res == vec.nth(5));
        CHECK(vec.size() == 10);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 55);
        CHECK(*vec.nth(6) == 60);
        CHECK(*vec.nth(7) == 70);
        CHECK(*vec.nth(8) == 80);
        CHECK(*vec.nth(9) == 90);
    }

    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        CHECK(vec.size() == 9);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
        CHECK(*vec.nth(6) == 70);
        CHECK(*vec.nth(7) == 80);
        CHECK(*vec.nth(8) == 90);

        auto res = vec.emplace(vec.nth(6), 65);

        CHECK(res == vec.nth(6));
        CHECK(vec.size() == 10);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
        CHECK(*vec.nth(6) == 65);
        CHECK(*vec.nth(7) == 70);
        CHECK(*vec.nth(8) == 80);
        CHECK(*vec.nth(9) == 90);
    }

    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        CHECK(vec.size() == 9);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
        CHECK(*vec.nth(6) == 70);
        CHECK(*vec.nth(7) == 80);
        CHECK(*vec.nth(8) == 90);

        auto res = vec.emplace(vec.nth(7), 75);

        CHECK(res == vec.nth(7));
        CHECK(vec.size() == 10);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
        CHECK(*vec.nth(6) == 70);
        CHECK(*vec.nth(7) == 75);
        CHECK(*vec.nth(8) == 80);
        CHECK(*vec.nth(9) == 90);
    }

    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        CHECK(vec.size() == 9);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
        CHECK(*vec.nth(6) == 70);
        CHECK(*vec.nth(7) == 80);
        CHECK(*vec.nth(8) == 90);

        auto res = vec.emplace(vec.nth(8), 85);

        CHECK(res == vec.nth(8));
        CHECK(vec.size() == 10);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
        CHECK(*vec.nth(6) == 70);
        CHECK(*vec.nth(7) == 80);
        CHECK(*vec.nth(8) == 85);
        CHECK(*vec.nth(9) == 90);
    }

    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        CHECK(vec.size() == 9);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
        CHECK(*vec.nth(6) == 70);
        CHECK(*vec.nth(7) == 80);
        CHECK(*vec.nth(8) == 90);

        auto res = vec.emplace(vec.nth(9), 95);

        CHECK(res == vec.nth(9));
        CHECK(vec.size() == 10);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
        CHECK(*vec.nth(6) == 70);
        CHECK(*vec.nth(7) == 80);
        CHECK(*vec.nth(8) == 90);
        CHECK(*vec.nth(9) == 95);
    }
}

///////////////////////////////////////////////////////////////////////////////

PRINT("Test begin, end, cbegin, cend, rbegin, rend, crbegin, crend, nth, index_of");
{
    sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

    vec.emplace_back(10);
    vec.emplace_back(20);
    vec.emplace_back(30);
    vec.emplace_back(40);
    vec.emplace_back(50);
    vec.emplace_back(60);
    vec.emplace_back(70);
    vec.emplace_back(80);
    vec.emplace_back(90);

    CHECK(vec.size() == 9);
    CHECK(*vec.nth(0) == 10);
    CHECK(*vec.nth(1) == 20);
    CHECK(*vec.nth(2) == 30);
    CHECK(*vec.nth(3) == 40);
    CHECK(*vec.nth(4) == 50);
    CHECK(*vec.nth(5) == 60);
    CHECK(*vec.nth(6) == 70);
    CHECK(*vec.nth(7) == 80);
    CHECK(*vec.nth(8) == 90);

    ///////////////////////////////////////////////////////////////////////

    auto it = vec.begin();
    CHECK(*it == 10); ++it;
    CHECK(*it == 20); ++it;
    CHECK(*it == 30); ++it;
    CHECK(*it == 40); ++it;
    CHECK(*it == 50); ++it;
    CHECK(*it == 60); ++it;
    CHECK(*it == 70); ++it;
    CHECK(*it == 80); ++it;
    CHECK(*it == 90); ++it;
    CHECK(it == vec.end());

    ///////////////////////////////////////////////////////////////////////

    auto cit = vec.cbegin();
    CHECK(*cit == 10); ++cit;
    CHECK(*cit == 20); ++cit;
    CHECK(*cit == 30); ++cit;
    CHECK(*cit == 40); ++cit;
    CHECK(*cit == 50); ++cit;
    CHECK(*cit == 60); ++cit;
    CHECK(*cit == 70); ++cit;
    CHECK(*cit == 80); ++cit;
    CHECK(*cit == 90); ++cit;
    CHECK(cit == vec.cend());

    ///////////////////////////////////////////////////////////////////////

    auto rit = vec.rbegin();
    CHECK(*rit == 90); ++rit;
    CHECK(*rit == 80); ++rit;
    CHECK(*rit == 70); ++rit;
    CHECK(*rit == 60); ++rit;
    CHECK(*rit == 50); ++rit;
    CHECK(*rit == 40); ++rit;
    CHECK(*rit == 30); ++rit;
    CHECK(*rit == 20); ++rit;
    CHECK(*rit == 10); ++rit;
    CHECK(rit == vec.rend());

    ///////////////////////////////////////////////////////////////////////

    auto crit = vec.crbegin();
    CHECK(*crit == 90); ++crit;
    CHECK(*crit == 80); ++crit;
    CHECK(*crit == 70); ++crit;
    CHECK(*crit == 60); ++crit;
    CHECK(*crit == 50); ++crit;
    CHECK(*crit == 40); ++crit;
    CHECK(*crit == 30); ++crit;
    CHECK(*crit == 20); ++crit;
    CHECK(*crit == 10); ++crit;
    CHECK(crit == vec.crend());

    ///////////////////////////////////////////////////////////////////////

    CHECK(*vec.nth(0) == 10);
    CHECK(*vec.nth(1) == 20);
    CHECK(*vec.nth(2) == 30);
    CHECK(*vec.nth(3) == 40);
    CHECK(*vec.nth(4) == 50);
    CHECK(*vec.nth(5) == 60);
    CHECK(*vec.nth(6) == 70);
    CHECK(*vec.nth(7) == 80);
    CHECK(*vec.nth(8) == 90);
    CHECK(vec.nth(9) == vec.end());

    ///////////////////////////////////////////////////////////////////////

    CHECK(vec.begin() + 0 == vec.nth(0));
    CHECK(vec.begin() + 1 == vec.nth(1));
    CHECK(vec.begin() + 2 == vec.nth(2));
    CHECK(vec.begin() + 3 == vec.nth(3));
    CHECK(vec.begin() + 4 == vec.nth(4));
    CHECK(vec.begin() + 5 == vec.nth(5));
    CHECK(vec.begin() + 6 == vec.nth(6));
    CHECK(vec.begin() + 7 == vec.nth(7));
    CHECK(vec.begin() + 8 == vec.nth(8));
    CHECK(vec.begin() + 9 == vec.nth(9));

    ///////////////////////////////////////////////////////////////////////

    CHECK(vec.cbegin() + 0 == vec.nth(0));
    CHECK(vec.cbegin() + 1 == vec.nth(1));
    CHECK(vec.cbegin() + 2 == vec.nth(2));
    CHECK(vec.cbegin() + 3 == vec.nth(3));
    CHECK(vec.cbegin() + 4 == vec.nth(4));
    CHECK(vec.cbegin() + 5 == vec.nth(5));
    CHECK(vec.cbegin() + 6 == vec.nth(6));
    CHECK(vec.cbegin() + 7 == vec.nth(7));
    CHECK(vec.cbegin() + 8 == vec.nth(8));
    CHECK(vec.cbegin() + 9 == vec.nth(9));

    ///////////////////////////////////////////////////////////////////////

    CHECK(vec.end() - 0 == vec.nth(9));
    CHECK(vec.end() - 1 == vec.nth(8));
    CHECK(vec.end() - 2 == vec.nth(7));
    CHECK(vec.end() - 3 == vec.nth(6));
    CHECK(vec.end() - 4 == vec.nth(5));
    CHECK(vec.end() - 5 == vec.nth(4));
    CHECK(vec.end() - 6 == vec.nth(3));
    CHECK(vec.end() - 7 == vec.nth(2));
    CHECK(vec.end() - 8 == vec.nth(1));
    CHECK(vec.end() - 9 == vec.nth(0));

    ///////////////////////////////////////////////////////////////////////

    CHECK(vec.cend() - 0 == vec.nth(9));
    CHECK(vec.cend() - 1 == vec.nth(8));
    CHECK(vec.cend() - 2 == vec.nth(7));
    CHECK(vec.cend() - 3 == vec.nth(6));
    CHECK(vec.cend() - 4 == vec.nth(5));
    CHECK(vec.cend() - 5 == vec.nth(4));
    CHECK(vec.cend() - 6 == vec.nth(3));
    CHECK(vec.cend() - 7 == vec.nth(2));
    CHECK(vec.cend() - 8 == vec.nth(1));
    CHECK(vec.cend() - 9 == vec.nth(0));

    ///////////////////////////////////////////////////////////////////////

    CHECK(std::next(vec.begin(), 0) == vec.nth(0));
    CHECK(std::next(vec.begin(), 1) == vec.nth(1));
    CHECK(std::next(vec.begin(), 2) == vec.nth(2));
    CHECK(std::next(vec.begin(), 3) == vec.nth(3));
    CHECK(std::next(vec.begin(), 4) == vec.nth(4));
    CHECK(std::next(vec.begin(), 5) == vec.nth(5));
    CHECK(std::next(vec.begin(), 6) == vec.nth(6));
    CHECK(std::next(vec.begin(), 7) == vec.nth(7));
    CHECK(std::next(vec.begin(), 8) == vec.nth(8));
    CHECK(std::next(vec.begin(), 9) == vec.nth(9));

    ///////////////////////////////////////////////////////////////////////

    CHECK(std::next(vec.cbegin(), 0) == vec.nth(0));
    CHECK(std::next(vec.cbegin(), 1) == vec.nth(1));
    CHECK(std::next(vec.cbegin(), 2) == vec.nth(2));
    CHECK(std::next(vec.cbegin(), 3) == vec.nth(3));
    CHECK(std::next(vec.cbegin(), 4) == vec.nth(4));
    CHECK(std::next(vec.cbegin(), 5) == vec.nth(5));
    CHECK(std::next(vec.cbegin(), 6) == vec.nth(6));
    CHECK(std::next(vec.cbegin(), 7) == vec.nth(7));
    CHECK(std::next(vec.cbegin(), 8) == vec.nth(8));
    CHECK(std::next(vec.cbegin(), 9) == vec.nth(9));

    ///////////////////////////////////////////////////////////////////////

    CHECK(vec.nth(0) < vec.nth(1));
    CHECK(vec.nth(0) < vec.nth(2));
    CHECK(vec.nth(0) < vec.nth(3));
    CHECK(vec.nth(0) < vec.nth(4));
    CHECK(vec.nth(0) < vec.nth(5));
    CHECK(vec.nth(0) < vec.nth(6));
    CHECK(vec.nth(0) < vec.nth(7));
    CHECK(vec.nth(0) < vec.nth(8));
    CHECK(vec.nth(0) < vec.nth(9));

    CHECK(vec.nth(1) < vec.nth(2));
    CHECK(vec.nth(1) < vec.nth(3));
    CHECK(vec.nth(1) < vec.nth(4));
    CHECK(vec.nth(1) < vec.nth(5));
    CHECK(vec.nth(1) < vec.nth(6));
    CHECK(vec.nth(1) < vec.nth(7));
    CHECK(vec.nth(1) < vec.nth(8));
    CHECK(vec.nth(1) < vec.nth(9));

    CHECK(vec.nth(2) < vec.nth(3));
    CHECK(vec.nth(2) < vec.nth(4));
    CHECK(vec.nth(2) < vec.nth(5));
    CHECK(vec.nth(2) < vec.nth(6));
    CHECK(vec.nth(2) < vec.nth(7));
    CHECK(vec.nth(2) < vec.nth(8));
    CHECK(vec.nth(2) < vec.nth(9));

    CHECK(vec.nth(3) < vec.nth(4));
    CHECK(vec.nth(3) < vec.nth(5));
    CHECK(vec.nth(3) < vec.nth(6));
    CHECK(vec.nth(3) < vec.nth(7));
    CHECK(vec.nth(3) < vec.nth(8));
    CHECK(vec.nth(3) < vec.nth(9));

    CHECK(vec.nth(4) < vec.nth(5));
    CHECK(vec.nth(4) < vec.nth(6));
    CHECK(vec.nth(4) < vec.nth(7));
    CHECK(vec.nth(4) < vec.nth(8));
    CHECK(vec.nth(4) < vec.nth(9));

    CHECK(vec.nth(5) < vec.nth(6));
    CHECK(vec.nth(5) < vec.nth(7));
    CHECK(vec.nth(5) < vec.nth(8));
    CHECK(vec.nth(5) < vec.nth(9));

    CHECK(vec.nth(6) < vec.nth(7));
    CHECK(vec.nth(6) < vec.nth(8));
    CHECK(vec.nth(6) < vec.nth(9));

    CHECK(vec.nth(7) < vec.nth(8));
    CHECK(vec.nth(7) < vec.nth(9));

    CHECK(vec.nth(8) < vec.nth(9));

    ///////////////////////////////////////////////////////////////////////

    CHECK(vec.index_of(vec.nth(0)) == 0);
    CHECK(vec.index_of(vec.nth(1)) == 1);
    CHECK(vec.index_of(vec.nth(2)) == 2);
    CHECK(vec.index_of(vec.nth(3)) == 3);
    CHECK(vec.index_of(vec.nth(4)) == 4);
    CHECK(vec.index_of(vec.nth(5)) == 5);
    CHECK(vec.index_of(vec.nth(6)) == 6);
    CHECK(vec.index_of(vec.nth(7)) == 7);
    CHECK(vec.index_of(vec.nth(8)) == 8);
    CHECK(vec.index_of(vec.nth(9)) == 9);
}

///////////////////////////////////////////////////////////////////////////////

PRINT("Test segment_capacity");
{
    CHECK((sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::segment_capacity == TPARAM_N));
}

///////////////////////////////////////////////////////////////////////////////

PRINT("Test get_allocator()");
{
    sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

    auto alloc = vec.get_allocator();

    (void)alloc;
}

///////////////////////////////////////////////////////////////////////////////

PRINT("Test clear()");
{
    sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

    CHECK(vec.size() == 0);

    vec.emplace_back(10);
    vec.emplace_back(20);
    vec.emplace_back(30);
    vec.emplace_back(40);
    vec.emplace_back(50);

    CHECK(vec.size() == 5);
    CHECK(*vec.nth(0) == 10);
    CHECK(*vec.nth(1) == 20);
    CHECK(*vec.nth(2) == 30);
    CHECK(*vec.nth(3) == 40);
    CHECK(*vec.nth(4) == 50);

    vec.clear();

    CHECK(vec.size() == 0);

    vec.emplace_back(10);
    vec.emplace_back(20);
    vec.emplace_back(30);

    CHECK(vec.size() == 3);
    CHECK(*vec.nth(0) == 10);
    CHECK(*vec.nth(1) == 20);
    CHECK(*vec.nth(2) == 30);

    vec.clear();

    CHECK(vec.size() == 0);
}

PRINT("Test insert(const_iterator, const T&)");
{
    sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

    xint value_10(10);

    auto res = vec.insert(vec.nth(0), value_10);

    CHECK(res == vec.nth(0));
    CHECK(vec.size() == 1);
    CHECK(*vec.nth(0) == 10);
    CHECK(value_10 == 10);
}

PRINT("Test insert(const_iterator, T&&)");
{
    sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

    xint value_10(10);

    auto res = vec.insert(vec.nth(0), std::move(value_10));

    CHECK(res == vec.nth(0));
    CHECK(vec.size() == 1);
    CHECK(*vec.nth(0) == 10);
    CHECK(value_10 == -10);
}

PRINT("Test insert(const_iterator, size_type, const T&)");
{
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        CHECK(vec.size() == 9);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
        CHECK(*vec.nth(6) == 70);
        CHECK(*vec.nth(7) == 80);
        CHECK(*vec.nth(8) == 90);

        auto res = vec.insert(vec.nth(0), 3, 5);

        CHECK(res == vec.nth(0));
        CHECK(vec.size() == 12);
        CHECK(*vec.nth( 0) ==  5);
        CHECK(*vec.nth( 1) ==  5);
        CHECK(*vec.nth( 2) ==  5);
        CHECK(*vec.nth( 3) == 10);
        CHECK(*vec.nth( 4) == 20);
        CHECK(*vec.nth( 5) == 30);
        CHECK(*vec.nth( 6) == 40);
        CHECK(*vec.nth( 7) == 50);
        CHECK(*vec.nth( 8) == 60);
        CHECK(*vec.nth( 9) == 70);
        CHECK(*vec.nth(10) == 80);
        CHECK(*vec.nth(11) == 90);
    }

    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        CHECK(vec.size() == 9);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
        CHECK(*vec.nth(6) == 70);
        CHECK(*vec.nth(7) == 80);
        CHECK(*vec.nth(8) == 90);

        auto res = vec.insert(vec.nth(1), 3, 5);

        CHECK(res == vec.nth(1));
        CHECK(vec.size() == 12);
        CHECK(*vec.nth( 0) == 10);
        CHECK(*vec.nth( 1) ==  5);
        CHECK(*vec.nth( 2) ==  5);
        CHECK(*vec.nth( 3) ==  5);
        CHECK(*vec.nth( 4) == 20);
        CHECK(*vec.nth( 5) == 30);
        CHECK(*vec.nth( 6) == 40);
        CHECK(*vec.nth( 7) == 50);
        CHECK(*vec.nth( 8) == 60);
        CHECK(*vec.nth( 9) == 70);
        CHECK(*vec.nth(10) == 80);
        CHECK(*vec.nth(11) == 90);
    }

    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        CHECK(vec.size() == 9);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
        CHECK(*vec.nth(6) == 70);
        CHECK(*vec.nth(7) == 80);
        CHECK(*vec.nth(8) == 90);

        auto res = vec.insert(vec.nth(2), 3, 5);

        CHECK(res == vec.nth(2));
        CHECK(vec.size() == 12);
        CHECK(*vec.nth( 0) == 10);
        CHECK(*vec.nth( 1) == 20);
        CHECK(*vec.nth( 2) ==  5);
        CHECK(*vec.nth( 3) ==  5);
        CHECK(*vec.nth( 4) ==  5);
        CHECK(*vec.nth( 5) == 30);
        CHECK(*vec.nth( 6) == 40);
        CHECK(*vec.nth( 7) == 50);
        CHECK(*vec.nth( 8) == 60);
        CHECK(*vec.nth( 9) == 70);
        CHECK(*vec.nth(10) == 80);
        CHECK(*vec.nth(11) == 90);
    }

    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        CHECK(vec.size() == 9);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
        CHECK(*vec.nth(6) == 70);
        CHECK(*vec.nth(7) == 80);
        CHECK(*vec.nth(8) == 90);

        auto res = vec.insert(vec.nth(3), 3, 5);

        CHECK(res == vec.nth(3));
        CHECK(vec.size() == 12);
        CHECK(*vec.nth( 0) == 10);
        CHECK(*vec.nth( 1) == 20);
        CHECK(*vec.nth( 2) == 30);
        CHECK(*vec.nth( 3) ==  5);
        CHECK(*vec.nth( 4) ==  5);
        CHECK(*vec.nth( 5) ==  5);
        CHECK(*vec.nth( 6) == 40);
        CHECK(*vec.nth( 7) == 50);
        CHECK(*vec.nth( 8) == 60);
        CHECK(*vec.nth( 9) == 70);
        CHECK(*vec.nth(10) == 80);
        CHECK(*vec.nth(11) == 90);
    }

    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        CHECK(vec.size() == 9);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
        CHECK(*vec.nth(6) == 70);
        CHECK(*vec.nth(7) == 80);
        CHECK(*vec.nth(8) == 90);

        auto res = vec.insert(vec.nth(4), 3, 5);

        CHECK(res == vec.nth(4));
        CHECK(vec.size() == 12);
        CHECK(*vec.nth( 0) == 10);
        CHECK(*vec.nth( 1) == 20);
        CHECK(*vec.nth( 2) == 30);
        CHECK(*vec.nth( 3) == 40);
        CHECK(*vec.nth( 4) ==  5);
        CHECK(*vec.nth( 5) ==  5);
        CHECK(*vec.nth( 6) ==  5);
        CHECK(*vec.nth( 7) == 50);
        CHECK(*vec.nth( 8) == 60);
        CHECK(*vec.nth( 9) == 70);
        CHECK(*vec.nth(10) == 80);
        CHECK(*vec.nth(11) == 90);
    }

    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        CHECK(vec.size() == 9);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
        CHECK(*vec.nth(6) == 70);
        CHECK(*vec.nth(7) == 80);
        CHECK(*vec.nth(8) == 90);

        auto res = vec.insert(vec.nth(5), 3, 5);

        CHECK(res == vec.nth(5));
        CHECK(vec.size() == 12);
        CHECK(*vec.nth( 0) == 10);
        CHECK(*vec.nth( 1) == 20);
        CHECK(*vec.nth( 2) == 30);
        CHECK(*vec.nth( 3) == 40);
        CHECK(*vec.nth( 4) == 50);
        CHECK(*vec.nth( 5) ==  5);
        CHECK(*vec.nth( 6) ==  5);
        CHECK(*vec.nth( 7) ==  5);
        CHECK(*vec.nth( 8) == 60);
        CHECK(*vec.nth( 9) == 70);
        CHECK(*vec.nth(10) == 80);
        CHECK(*vec.nth(11) == 90);
    }

    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        CHECK(vec.size() == 9);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
        CHECK(*vec.nth(6) == 70);
        CHECK(*vec.nth(7) == 80);
        CHECK(*vec.nth(8) == 90);

        auto res = vec.insert(vec.nth(6), 3, 5);

        CHECK(res == vec.nth(6));
        CHECK(vec.size() == 12);
        CHECK(*vec.nth( 0) == 10);
        CHECK(*vec.nth( 1) == 20);
        CHECK(*vec.nth( 2) == 30);
        CHECK(*vec.nth( 3) == 40);
        CHECK(*vec.nth( 4) == 50);
        CHECK(*vec.nth( 5) == 60);
        CHECK(*vec.nth( 6) ==  5);
        CHECK(*vec.nth( 7) ==  5);
        CHECK(*vec.nth( 8) ==  5);
        CHECK(*vec.nth( 9) == 70);
        CHECK(*vec.nth(10) == 80);
        CHECK(*vec.nth(11) == 90);
    }

    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        CHECK(vec.size() == 9);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
        CHECK(*vec.nth(6) == 70);
        CHECK(*vec.nth(7) == 80);
        CHECK(*vec.nth(8) == 90);

        auto res = vec.insert(vec.nth(7), 3, 5);

        CHECK(res == vec.nth(7));
        CHECK(vec.size() == 12);
        CHECK(*vec.nth( 0) == 10);
        CHECK(*vec.nth( 1) == 20);
        CHECK(*vec.nth( 2) == 30);
        CHECK(*vec.nth( 3) == 40);
        CHECK(*vec.nth( 4) == 50);
        CHECK(*vec.nth( 5) == 60);
        CHECK(*vec.nth( 6) == 70);
        CHECK(*vec.nth( 7) ==  5);
        CHECK(*vec.nth( 8) ==  5);
        CHECK(*vec.nth( 9) ==  5);
        CHECK(*vec.nth(10) == 80);
        CHECK(*vec.nth(11) == 90);
    }

    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        CHECK(vec.size() == 9);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
        CHECK(*vec.nth(6) == 70);
        CHECK(*vec.nth(7) == 80);
        CHECK(*vec.nth(8) == 90);

        auto res = vec.insert(vec.nth(8), 3, 5);

        CHECK(res == vec.nth(8));
        CHECK(vec.size() == 12);
        CHECK(*vec.nth( 0) == 10);
        CHECK(*vec.nth( 1) == 20);
        CHECK(*vec.nth( 2) == 30);
        CHECK(*vec.nth( 3) == 40);
        CHECK(*vec.nth( 4) == 50);
        CHECK(*vec.nth( 5) == 60);
        CHECK(*vec.nth( 6) == 70);
        CHECK(*vec.nth( 7) == 80);
        CHECK(*vec.nth( 8) ==  5);
        CHECK(*vec.nth( 9) ==  5);
        CHECK(*vec.nth(10) ==  5);
        CHECK(*vec.nth(11) == 90);
    }

    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        CHECK(vec.size() == 9);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
        CHECK(*vec.nth(6) == 70);
        CHECK(*vec.nth(7) == 80);
        CHECK(*vec.nth(8) == 90);

        auto res = vec.insert(vec.nth(9), 3, 5);

        CHECK(res == vec.nth(9));
        CHECK(vec.size() == 12);
        CHECK(*vec.nth( 0) == 10);
        CHECK(*vec.nth( 1) == 20);
        CHECK(*vec.nth( 2) == 30);
        CHECK(*vec.nth( 3) == 40);
        CHECK(*vec.nth( 4) == 50);
        CHECK(*vec.nth( 5) == 60);
        CHECK(*vec.nth( 6) == 70);
        CHECK(*vec.nth( 7) == 80);
        CHECK(*vec.nth( 8) == 90);
        CHECK(*vec.nth( 9) ==  5);
        CHECK(*vec.nth(10) ==  5);
        CHECK(*vec.nth(11) ==  5);
    }
}

PRINT("Test insert(const_iterator, InputIt, InputIt)");
{
    // Input iterator (exactly)
    {
        {
            sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

            vec.emplace_back(10);
            vec.emplace_back(20);
            vec.emplace_back(30);
            vec.emplace_back(40);
            vec.emplace_back(50);
            vec.emplace_back(60);
            vec.emplace_back(70);
            vec.emplace_back(80);
            vec.emplace_back(90);

            CHECK(vec.size() == 9);
            CHECK(*vec.nth(0) == 10);
            CHECK(*vec.nth(1) == 20);
            CHECK(*vec.nth(2) == 30);
            CHECK(*vec.nth(3) == 40);
            CHECK(*vec.nth(4) == 50);
            CHECK(*vec.nth(5) == 60);
            CHECK(*vec.nth(6) == 70);
            CHECK(*vec.nth(7) == 80);
            CHECK(*vec.nth(8) == 90);

            std::istringstream iss("1 2 3");

            auto res = vec.insert
            (
                vec.nth(0),
                std::istream_iterator<int>(iss),
                std::istream_iterator<int>()
            );

            CHECK(res == vec.nth(0));
            CHECK(vec.size() == 12);
            CHECK(*vec.nth( 0) ==  1);
            CHECK(*vec.nth( 1) ==  2);
            CHECK(*vec.nth( 2) ==  3);
            CHECK(*vec.nth( 3) == 10);
            CHECK(*vec.nth( 4) == 20);
            CHECK(*vec.nth( 5) == 30);
            CHECK(*vec.nth( 6) == 40);
            CHECK(*vec.nth( 7) == 50);
            CHECK(*vec.nth( 8) == 60);
            CHECK(*vec.nth( 9) == 70);
            CHECK(*vec.nth(10) == 80);
            CHECK(*vec.nth(11) == 90);
        }

        {
            sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

            vec.emplace_back(10);
            vec.emplace_back(20);
            vec.emplace_back(30);
            vec.emplace_back(40);
            vec.emplace_back(50);
            vec.emplace_back(60);
            vec.emplace_back(70);
            vec.emplace_back(80);
            vec.emplace_back(90);

            CHECK(vec.size() == 9);
            CHECK(*vec.nth(0) == 10);
            CHECK(*vec.nth(1) == 20);
            CHECK(*vec.nth(2) == 30);
            CHECK(*vec.nth(3) == 40);
            CHECK(*vec.nth(4) == 50);
            CHECK(*vec.nth(5) == 60);
            CHECK(*vec.nth(6) == 70);
            CHECK(*vec.nth(7) == 80);
            CHECK(*vec.nth(8) == 90);

            std::istringstream iss("1 2 3");

            auto res = vec.insert
            (
                vec.nth(1),
                std::istream_iterator<int>(iss),
                std::istream_iterator<int>()
            );

            CHECK(res == vec.nth(1));
            CHECK(vec.size() == 12);
            CHECK(*vec.nth( 0) == 10);
            CHECK(*vec.nth( 1) ==  1);
            CHECK(*vec.nth( 2) ==  2);
            CHECK(*vec.nth( 3) ==  3);
            CHECK(*vec.nth( 4) == 20);
            CHECK(*vec.nth( 5) == 30);
            CHECK(*vec.nth( 6) == 40);
            CHECK(*vec.nth( 7) == 50);
            CHECK(*vec.nth( 8) == 60);
            CHECK(*vec.nth( 9) == 70);
            CHECK(*vec.nth(10) == 80);
            CHECK(*vec.nth(11) == 90);
        }

        {
            sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

            vec.emplace_back(10);
            vec.emplace_back(20);
            vec.emplace_back(30);
            vec.emplace_back(40);
            vec.emplace_back(50);
            vec.emplace_back(60);
            vec.emplace_back(70);
            vec.emplace_back(80);
            vec.emplace_back(90);

            CHECK(vec.size() == 9);
            CHECK(*vec.nth(0) == 10);
            CHECK(*vec.nth(1) == 20);
            CHECK(*vec.nth(2) == 30);
            CHECK(*vec.nth(3) == 40);
            CHECK(*vec.nth(4) == 50);
            CHECK(*vec.nth(5) == 60);
            CHECK(*vec.nth(6) == 70);
            CHECK(*vec.nth(7) == 80);
            CHECK(*vec.nth(8) == 90);

            std::istringstream iss("1 2 3");

            auto res = vec.insert
            (
                vec.nth(2),
                std::istream_iterator<int>(iss),
                std::istream_iterator<int>()
            );

            CHECK(res == vec.nth(2));
            CHECK(vec.size() == 12);
            CHECK(*vec.nth( 0) == 10);
            CHECK(*vec.nth( 1) == 20);
            CHECK(*vec.nth( 2) ==  1);
            CHECK(*vec.nth( 3) ==  2);
            CHECK(*vec.nth( 4) ==  3);
            CHECK(*vec.nth( 5) == 30);
            CHECK(*vec.nth( 6) == 40);
            CHECK(*vec.nth( 7) == 50);
            CHECK(*vec.nth( 8) == 60);
            CHECK(*vec.nth( 9) == 70);
            CHECK(*vec.nth(10) == 80);
            CHECK(*vec.nth(11) == 90);
        }

        {
            sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

            vec.emplace_back(10);
            vec.emplace_back(20);
            vec.emplace_back(30);
            vec.emplace_back(40);
            vec.emplace_back(50);
            vec.emplace_back(60);
            vec.emplace_back(70);
            vec.emplace_back(80);
            vec.emplace_back(90);

            CHECK(vec.size() == 9);
            CHECK(*vec.nth(0) == 10);
            CHECK(*vec.nth(1) == 20);
            CHECK(*vec.nth(2) == 30);
            CHECK(*vec.nth(3) == 40);
            CHECK(*vec.nth(4) == 50);
            CHECK(*vec.nth(5) == 60);
            CHECK(*vec.nth(6) == 70);
            CHECK(*vec.nth(7) == 80);
            CHECK(*vec.nth(8) == 90);

            std::istringstream iss("1 2 3");

            auto res = vec.insert
            (
                vec.nth(3),
                std::istream_iterator<int>(iss),
                std::istream_iterator<int>()
            );

            CHECK(res == vec.nth(3));
            CHECK(vec.size() == 12);
            CHECK(*vec.nth( 0) == 10);
            CHECK(*vec.nth( 1) == 20);
            CHECK(*vec.nth( 2) == 30);
            CHECK(*vec.nth( 3) ==  1);
            CHECK(*vec.nth( 4) ==  2);
            CHECK(*vec.nth( 5) ==  3);
            CHECK(*vec.nth( 6) == 40);
            CHECK(*vec.nth( 7) == 50);
            CHECK(*vec.nth( 8) == 60);
            CHECK(*vec.nth( 9) == 70);
            CHECK(*vec.nth(10) == 80);
            CHECK(*vec.nth(11) == 90);
        }

        {
            sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

            vec.emplace_back(10);
            vec.emplace_back(20);
            vec.emplace_back(30);
            vec.emplace_back(40);
            vec.emplace_back(50);
            vec.emplace_back(60);
            vec.emplace_back(70);
            vec.emplace_back(80);
            vec.emplace_back(90);

            CHECK(vec.size() == 9);
            CHECK(*vec.nth(0) == 10);
            CHECK(*vec.nth(1) == 20);
            CHECK(*vec.nth(2) == 30);
            CHECK(*vec.nth(3) == 40);
            CHECK(*vec.nth(4) == 50);
            CHECK(*vec.nth(5) == 60);
            CHECK(*vec.nth(6) == 70);
            CHECK(*vec.nth(7) == 80);
            CHECK(*vec.nth(8) == 90);

            std::istringstream iss("1 2 3");

            auto res = vec.insert
            (
                vec.nth(4),
                std::istream_iterator<int>(iss),
                std::istream_iterator<int>()
            );

            CHECK(res == vec.nth(4));
            CHECK(vec.size() == 12);
            CHECK(*vec.nth( 0) == 10);
            CHECK(*vec.nth( 1) == 20);
            CHECK(*vec.nth( 2) == 30);
            CHECK(*vec.nth( 3) == 40);
            CHECK(*vec.nth( 4) ==  1);
            CHECK(*vec.nth( 5) ==  2);
            CHECK(*vec.nth( 6) ==  3);
            CHECK(*vec.nth( 7) == 50);
            CHECK(*vec.nth( 8) == 60);
            CHECK(*vec.nth( 9) == 70);
            CHECK(*vec.nth(10) == 80);
            CHECK(*vec.nth(11) == 90);
        }

        {
            sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

            vec.emplace_back(10);
            vec.emplace_back(20);
            vec.emplace_back(30);
            vec.emplace_back(40);
            vec.emplace_back(50);
            vec.emplace_back(60);
            vec.emplace_back(70);
            vec.emplace_back(80);
            vec.emplace_back(90);

            CHECK(vec.size() == 9);
            CHECK(*vec.nth(0) == 10);
            CHECK(*vec.nth(1) == 20);
            CHECK(*vec.nth(2) == 30);
            CHECK(*vec.nth(3) == 40);
            CHECK(*vec.nth(4) == 50);
            CHECK(*vec.nth(5) == 60);
            CHECK(*vec.nth(6) == 70);
            CHECK(*vec.nth(7) == 80);
            CHECK(*vec.nth(8) == 90);

            std::istringstream iss("1 2 3");

            auto res = vec.insert
            (
                vec.nth(5),
                std::istream_iterator<int>(iss),
                std::istream_iterator<int>()
            );

            CHECK(res == vec.nth(5));
            CHECK(vec.size() == 12);
            CHECK(*vec.nth( 0) == 10);
            CHECK(*vec.nth( 1) == 20);
            CHECK(*vec.nth( 2) == 30);
            CHECK(*vec.nth( 3) == 40);
            CHECK(*vec.nth( 4) == 50);
            CHECK(*vec.nth( 5) ==  1);
            CHECK(*vec.nth( 6) ==  2);
            CHECK(*vec.nth( 7) ==  3);
            CHECK(*vec.nth( 8) == 60);
            CHECK(*vec.nth( 9) == 70);
            CHECK(*vec.nth(10) == 80);
            CHECK(*vec.nth(11) == 90);
        }

        {
            sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

            vec.emplace_back(10);
            vec.emplace_back(20);
            vec.emplace_back(30);
            vec.emplace_back(40);
            vec.emplace_back(50);
            vec.emplace_back(60);
            vec.emplace_back(70);
            vec.emplace_back(80);
            vec.emplace_back(90);

            CHECK(vec.size() == 9);
            CHECK(*vec.nth(0) == 10);
            CHECK(*vec.nth(1) == 20);
            CHECK(*vec.nth(2) == 30);
            CHECK(*vec.nth(3) == 40);
            CHECK(*vec.nth(4) == 50);
            CHECK(*vec.nth(5) == 60);
            CHECK(*vec.nth(6) == 70);
            CHECK(*vec.nth(7) == 80);
            CHECK(*vec.nth(8) == 90);

            std::istringstream iss("1 2 3");

            auto res = vec.insert
            (
                vec.nth(6),
                std::istream_iterator<int>(iss),
                std::istream_iterator<int>()
            );

            CHECK(res == vec.nth(6));
            CHECK(vec.size() == 12);
            CHECK(*vec.nth( 0) == 10);
            CHECK(*vec.nth( 1) == 20);
            CHECK(*vec.nth( 2) == 30);
            CHECK(*vec.nth( 3) == 40);
            CHECK(*vec.nth( 4) == 50);
            CHECK(*vec.nth( 5) == 60);
            CHECK(*vec.nth( 6) ==  1);
            CHECK(*vec.nth( 7) ==  2);
            CHECK(*vec.nth( 8) ==  3);
            CHECK(*vec.nth( 9) == 70);
            CHECK(*vec.nth(10) == 80);
            CHECK(*vec.nth(11) == 90);
        }

        {
            sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

            vec.emplace_back(10);
            vec.emplace_back(20);
            vec.emplace_back(30);
            vec.emplace_back(40);
            vec.emplace_back(50);
            vec.emplace_back(60);
            vec.emplace_back(70);
            vec.emplace_back(80);
            vec.emplace_back(90);

            CHECK(vec.size() == 9);
            CHECK(*vec.nth(0) == 10);
            CHECK(*vec.nth(1) == 20);
            CHECK(*vec.nth(2) == 30);
            CHECK(*vec.nth(3) == 40);
            CHECK(*vec.nth(4) == 50);
            CHECK(*vec.nth(5) == 60);
            CHECK(*vec.nth(6) == 70);
            CHECK(*vec.nth(7) == 80);
            CHECK(*vec.nth(8) == 90);

            std::istringstream iss("1 2 3");

            auto res = vec.insert
            (
                vec.nth(7),
                std::istream_iterator<int>(iss),
                std::istream_iterator<int>()
            );

            CHECK(res == vec.nth(7));
            CHECK(vec.size() == 12);
            CHECK(*vec.nth( 0) == 10);
            CHECK(*vec.nth( 1) == 20);
            CHECK(*vec.nth( 2) == 30);
            CHECK(*vec.nth( 3) == 40);
            CHECK(*vec.nth( 4) == 50);
            CHECK(*vec.nth( 5) == 60);
            CHECK(*vec.nth( 6) == 70);
            CHECK(*vec.nth( 7) ==  1);
            CHECK(*vec.nth( 8) ==  2);
            CHECK(*vec.nth( 9) ==  3);
            CHECK(*vec.nth(10) == 80);
            CHECK(*vec.nth(11) == 90);
        }

        {
            sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

            vec.emplace_back(10);
            vec.emplace_back(20);
            vec.emplace_back(30);
            vec.emplace_back(40);
            vec.emplace_back(50);
            vec.emplace_back(60);
            vec.emplace_back(70);
            vec.emplace_back(80);
            vec.emplace_back(90);

            CHECK(vec.size() == 9);
            CHECK(*vec.nth(0) == 10);
            CHECK(*vec.nth(1) == 20);
            CHECK(*vec.nth(2) == 30);
            CHECK(*vec.nth(3) == 40);
            CHECK(*vec.nth(4) == 50);
            CHECK(*vec.nth(5) == 60);
            CHECK(*vec.nth(6) == 70);
            CHECK(*vec.nth(7) == 80);
            CHECK(*vec.nth(8) == 90);

            std::istringstream iss("1 2 3");

            auto res = vec.insert
            (
                vec.nth(8),
                std::istream_iterator<int>(iss),
                std::istream_iterator<int>()
            );

            CHECK(res == vec.nth(8));
            CHECK(vec.size() == 12);
            CHECK(*vec.nth( 0) == 10);
            CHECK(*vec.nth( 1) == 20);
            CHECK(*vec.nth( 2) == 30);
            CHECK(*vec.nth( 3) == 40);
            CHECK(*vec.nth( 4) == 50);
            CHECK(*vec.nth( 5) == 60);
            CHECK(*vec.nth( 6) == 70);
            CHECK(*vec.nth( 7) == 80);
            CHECK(*vec.nth( 8) ==  1);
            CHECK(*vec.nth( 9) ==  2);
            CHECK(*vec.nth(10) ==  3);
            CHECK(*vec.nth(11) == 90);
        }

        {
            sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

            vec.emplace_back(10);
            vec.emplace_back(20);
            vec.emplace_back(30);
            vec.emplace_back(40);
            vec.emplace_back(50);
            vec.emplace_back(60);
            vec.emplace_back(70);
            vec.emplace_back(80);
            vec.emplace_back(90);

            CHECK(vec.size() == 9);
            CHECK(*vec.nth(0) == 10);
            CHECK(*vec.nth(1) == 20);
            CHECK(*vec.nth(2) == 30);
            CHECK(*vec.nth(3) == 40);
            CHECK(*vec.nth(4) == 50);
            CHECK(*vec.nth(5) == 60);
            CHECK(*vec.nth(6) == 70);
            CHECK(*vec.nth(7) == 80);
            CHECK(*vec.nth(8) == 90);

            std::istringstream iss("1 2 3");

            auto res = vec.insert
            (
                vec.nth(9),
                std::istream_iterator<int>(iss),
                std::istream_iterator<int>()
            );

            CHECK(res == vec.nth(9));
            CHECK(vec.size() == 12);
            CHECK(*vec.nth( 0) == 10);
            CHECK(*vec.nth( 1) == 20);
            CHECK(*vec.nth( 2) == 30);
            CHECK(*vec.nth( 3) == 40);
            CHECK(*vec.nth( 4) == 50);
            CHECK(*vec.nth( 5) == 60);
            CHECK(*vec.nth( 6) == 70);
            CHECK(*vec.nth( 7) == 80);
            CHECK(*vec.nth( 8) == 90);
            CHECK(*vec.nth( 9) ==  1);
            CHECK(*vec.nth(10) ==  2);
            CHECK(*vec.nth(11) ==  3);
        }
    }

    // Forward iterator
    {
        {
            sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

            vec.emplace_back(10);
            vec.emplace_back(20);
            vec.emplace_back(30);
            vec.emplace_back(40);
            vec.emplace_back(50);
            vec.emplace_back(60);
            vec.emplace_back(70);
            vec.emplace_back(80);
            vec.emplace_back(90);

            CHECK(vec.size() == 9);
            CHECK(*vec.nth(0) == 10);
            CHECK(*vec.nth(1) == 20);
            CHECK(*vec.nth(2) == 30);
            CHECK(*vec.nth(3) == 40);
            CHECK(*vec.nth(4) == 50);
            CHECK(*vec.nth(5) == 60);
            CHECK(*vec.nth(6) == 70);
            CHECK(*vec.nth(7) == 80);
            CHECK(*vec.nth(8) == 90);

            std::vector<xint> data({1, 2, 3});

            auto res = vec.insert(vec.nth(0), data.begin(), data.end());

            CHECK(res == vec.nth(0));
            CHECK(vec.size() == 12);
            CHECK(*vec.nth( 0) ==  1);
            CHECK(*vec.nth( 1) ==  2);
            CHECK(*vec.nth( 2) ==  3);
            CHECK(*vec.nth( 3) == 10);
            CHECK(*vec.nth( 4) == 20);
            CHECK(*vec.nth( 5) == 30);
            CHECK(*vec.nth( 6) == 40);
            CHECK(*vec.nth( 7) == 50);
            CHECK(*vec.nth( 8) == 60);
            CHECK(*vec.nth( 9) == 70);
            CHECK(*vec.nth(10) == 80);
            CHECK(*vec.nth(11) == 90);
        }

        {
            sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

            vec.emplace_back(10);
            vec.emplace_back(20);
            vec.emplace_back(30);
            vec.emplace_back(40);
            vec.emplace_back(50);
            vec.emplace_back(60);
            vec.emplace_back(70);
            vec.emplace_back(80);
            vec.emplace_back(90);

            CHECK(vec.size() == 9);
            CHECK(*vec.nth(0) == 10);
            CHECK(*vec.nth(1) == 20);
            CHECK(*vec.nth(2) == 30);
            CHECK(*vec.nth(3) == 40);
            CHECK(*vec.nth(4) == 50);
            CHECK(*vec.nth(5) == 60);
            CHECK(*vec.nth(6) == 70);
            CHECK(*vec.nth(7) == 80);
            CHECK(*vec.nth(8) == 90);

            std::vector<xint> data({1, 2, 3});

            auto res = vec.insert(vec.nth(1), data.begin(), data.end());

            CHECK(res == vec.nth(1));
            CHECK(vec.size() == 12);
            CHECK(*vec.nth( 0) == 10);
            CHECK(*vec.nth( 1) ==  1);
            CHECK(*vec.nth( 2) ==  2);
            CHECK(*vec.nth( 3) ==  3);
            CHECK(*vec.nth( 4) == 20);
            CHECK(*vec.nth( 5) == 30);
            CHECK(*vec.nth( 6) == 40);
            CHECK(*vec.nth( 7) == 50);
            CHECK(*vec.nth( 8) == 60);
            CHECK(*vec.nth( 9) == 70);
            CHECK(*vec.nth(10) == 80);
            CHECK(*vec.nth(11) == 90);
        }

        {
            sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

            vec.emplace_back(10);
            vec.emplace_back(20);
            vec.emplace_back(30);
            vec.emplace_back(40);
            vec.emplace_back(50);
            vec.emplace_back(60);
            vec.emplace_back(70);
            vec.emplace_back(80);
            vec.emplace_back(90);

            CHECK(vec.size() == 9);
            CHECK(*vec.nth(0) == 10);
            CHECK(*vec.nth(1) == 20);
            CHECK(*vec.nth(2) == 30);
            CHECK(*vec.nth(3) == 40);
            CHECK(*vec.nth(4) == 50);
            CHECK(*vec.nth(5) == 60);
            CHECK(*vec.nth(6) == 70);
            CHECK(*vec.nth(7) == 80);
            CHECK(*vec.nth(8) == 90);

            std::vector<xint> data({1, 2, 3});

            auto res = vec.insert(vec.nth(2), data.begin(), data.end());

            CHECK(res == vec.nth(2));
            CHECK(vec.size() == 12);
            CHECK(*vec.nth( 0) == 10);
            CHECK(*vec.nth( 1) == 20);
            CHECK(*vec.nth( 2) ==  1);
            CHECK(*vec.nth( 3) ==  2);
            CHECK(*vec.nth( 4) ==  3);
            CHECK(*vec.nth( 5) == 30);
            CHECK(*vec.nth( 6) == 40);
            CHECK(*vec.nth( 7) == 50);
            CHECK(*vec.nth( 8) == 60);
            CHECK(*vec.nth( 9) == 70);
            CHECK(*vec.nth(10) == 80);
            CHECK(*vec.nth(11) == 90);
        }

        {
            sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

            vec.emplace_back(10);
            vec.emplace_back(20);
            vec.emplace_back(30);
            vec.emplace_back(40);
            vec.emplace_back(50);
            vec.emplace_back(60);
            vec.emplace_back(70);
            vec.emplace_back(80);
            vec.emplace_back(90);

            CHECK(vec.size() == 9);
            CHECK(*vec.nth(0) == 10);
            CHECK(*vec.nth(1) == 20);
            CHECK(*vec.nth(2) == 30);
            CHECK(*vec.nth(3) == 40);
            CHECK(*vec.nth(4) == 50);
            CHECK(*vec.nth(5) == 60);
            CHECK(*vec.nth(6) == 70);
            CHECK(*vec.nth(7) == 80);
            CHECK(*vec.nth(8) == 90);

            std::vector<xint> data({1, 2, 3});

            auto res = vec.insert(vec.nth(3), data.begin(), data.end());

            CHECK(res == vec.nth(3));
            CHECK(vec.size() == 12);
            CHECK(*vec.nth( 0) == 10);
            CHECK(*vec.nth( 1) == 20);
            CHECK(*vec.nth( 2) == 30);
            CHECK(*vec.nth( 3) ==  1);
            CHECK(*vec.nth( 4) ==  2);
            CHECK(*vec.nth( 5) ==  3);
            CHECK(*vec.nth( 6) == 40);
            CHECK(*vec.nth( 7) == 50);
            CHECK(*vec.nth( 8) == 60);
            CHECK(*vec.nth( 9) == 70);
            CHECK(*vec.nth(10) == 80);
            CHECK(*vec.nth(11) == 90);
        }

        {
            sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

            vec.emplace_back(10);
            vec.emplace_back(20);
            vec.emplace_back(30);
            vec.emplace_back(40);
            vec.emplace_back(50);
            vec.emplace_back(60);
            vec.emplace_back(70);
            vec.emplace_back(80);
            vec.emplace_back(90);

            CHECK(vec.size() == 9);
            CHECK(*vec.nth(0) == 10);
            CHECK(*vec.nth(1) == 20);
            CHECK(*vec.nth(2) == 30);
            CHECK(*vec.nth(3) == 40);
            CHECK(*vec.nth(4) == 50);
            CHECK(*vec.nth(5) == 60);
            CHECK(*vec.nth(6) == 70);
            CHECK(*vec.nth(7) == 80);
            CHECK(*vec.nth(8) == 90);

            std::vector<xint> data({1, 2, 3});

            auto res = vec.insert(vec.nth(4), data.begin(), data.end());

            CHECK(res == vec.nth(4));
            CHECK(vec.size() == 12);
            CHECK(*vec.nth( 0) == 10);
            CHECK(*vec.nth( 1) == 20);
            CHECK(*vec.nth( 2) == 30);
            CHECK(*vec.nth( 3) == 40);
            CHECK(*vec.nth( 4) ==  1);
            CHECK(*vec.nth( 5) ==  2);
            CHECK(*vec.nth( 6) ==  3);
            CHECK(*vec.nth( 7) == 50);
            CHECK(*vec.nth( 8) == 60);
            CHECK(*vec.nth( 9) == 70);
            CHECK(*vec.nth(10) == 80);
            CHECK(*vec.nth(11) == 90);
        }

        {
            sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

            vec.emplace_back(10);
            vec.emplace_back(20);
            vec.emplace_back(30);
            vec.emplace_back(40);
            vec.emplace_back(50);
            vec.emplace_back(60);
            vec.emplace_back(70);
            vec.emplace_back(80);
            vec.emplace_back(90);

            CHECK(vec.size() == 9);
            CHECK(*vec.nth(0) == 10);
            CHECK(*vec.nth(1) == 20);
            CHECK(*vec.nth(2) == 30);
            CHECK(*vec.nth(3) == 40);
            CHECK(*vec.nth(4) == 50);
            CHECK(*vec.nth(5) == 60);
            CHECK(*vec.nth(6) == 70);
            CHECK(*vec.nth(7) == 80);
            CHECK(*vec.nth(8) == 90);

            std::vector<xint> data({1, 2, 3});

            auto res = vec.insert(vec.nth(5), data.begin(), data.end());

            CHECK(res == vec.nth(5));
            CHECK(vec.size() == 12);
            CHECK(*vec.nth( 0) == 10);
            CHECK(*vec.nth( 1) == 20);
            CHECK(*vec.nth( 2) == 30);
            CHECK(*vec.nth( 3) == 40);
            CHECK(*vec.nth( 4) == 50);
            CHECK(*vec.nth( 5) ==  1);
            CHECK(*vec.nth( 6) ==  2);
            CHECK(*vec.nth( 7) ==  3);
            CHECK(*vec.nth( 8) == 60);
            CHECK(*vec.nth( 9) == 70);
            CHECK(*vec.nth(10) == 80);
            CHECK(*vec.nth(11) == 90);
        }

        {
            sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

            vec.emplace_back(10);
            vec.emplace_back(20);
            vec.emplace_back(30);
            vec.emplace_back(40);
            vec.emplace_back(50);
            vec.emplace_back(60);
            vec.emplace_back(70);
            vec.emplace_back(80);
            vec.emplace_back(90);

            CHECK(vec.size() == 9);
            CHECK(*vec.nth(0) == 10);
            CHECK(*vec.nth(1) == 20);
            CHECK(*vec.nth(2) == 30);
            CHECK(*vec.nth(3) == 40);
            CHECK(*vec.nth(4) == 50);
            CHECK(*vec.nth(5) == 60);
            CHECK(*vec.nth(6) == 70);
            CHECK(*vec.nth(7) == 80);
            CHECK(*vec.nth(8) == 90);

            std::vector<xint> data({1, 2, 3});

            auto res = vec.insert(vec.nth(6), data.begin(), data.end());

            CHECK(res == vec.nth(6));
            CHECK(vec.size() == 12);
            CHECK(*vec.nth( 0) == 10);
            CHECK(*vec.nth( 1) == 20);
            CHECK(*vec.nth( 2) == 30);
            CHECK(*vec.nth( 3) == 40);
            CHECK(*vec.nth( 4) == 50);
            CHECK(*vec.nth( 5) == 60);
            CHECK(*vec.nth( 6) ==  1);
            CHECK(*vec.nth( 7) ==  2);
            CHECK(*vec.nth( 8) ==  3);
            CHECK(*vec.nth( 9) == 70);
            CHECK(*vec.nth(10) == 80);
            CHECK(*vec.nth(11) == 90);
        }

        {
            sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

            vec.emplace_back(10);
            vec.emplace_back(20);
            vec.emplace_back(30);
            vec.emplace_back(40);
            vec.emplace_back(50);
            vec.emplace_back(60);
            vec.emplace_back(70);
            vec.emplace_back(80);
            vec.emplace_back(90);

            CHECK(vec.size() == 9);
            CHECK(*vec.nth(0) == 10);
            CHECK(*vec.nth(1) == 20);
            CHECK(*vec.nth(2) == 30);
            CHECK(*vec.nth(3) == 40);
            CHECK(*vec.nth(4) == 50);
            CHECK(*vec.nth(5) == 60);
            CHECK(*vec.nth(6) == 70);
            CHECK(*vec.nth(7) == 80);
            CHECK(*vec.nth(8) == 90);

            std::vector<xint> data({1, 2, 3});

            auto res = vec.insert(vec.nth(7), data.begin(), data.end());

            CHECK(res == vec.nth(7));
            CHECK(vec.size() == 12);
            CHECK(*vec.nth( 0) == 10);
            CHECK(*vec.nth( 1) == 20);
            CHECK(*vec.nth( 2) == 30);
            CHECK(*vec.nth( 3) == 40);
            CHECK(*vec.nth( 4) == 50);
            CHECK(*vec.nth( 5) == 60);
            CHECK(*vec.nth( 6) == 70);
            CHECK(*vec.nth( 7) ==  1);
            CHECK(*vec.nth( 8) ==  2);
            CHECK(*vec.nth( 9) ==  3);
            CHECK(*vec.nth(10) == 80);
            CHECK(*vec.nth(11) == 90);
        }

        {
            sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

            vec.emplace_back(10);
            vec.emplace_back(20);
            vec.emplace_back(30);
            vec.emplace_back(40);
            vec.emplace_back(50);
            vec.emplace_back(60);
            vec.emplace_back(70);
            vec.emplace_back(80);
            vec.emplace_back(90);

            CHECK(vec.size() == 9);
            CHECK(*vec.nth(0) == 10);
            CHECK(*vec.nth(1) == 20);
            CHECK(*vec.nth(2) == 30);
            CHECK(*vec.nth(3) == 40);
            CHECK(*vec.nth(4) == 50);
            CHECK(*vec.nth(5) == 60);
            CHECK(*vec.nth(6) == 70);
            CHECK(*vec.nth(7) == 80);
            CHECK(*vec.nth(8) == 90);

            std::vector<xint> data({1, 2, 3});

            auto res = vec.insert(vec.nth(8), data.begin(), data.end());

            CHECK(res == vec.nth(8));
            CHECK(vec.size() == 12);
            CHECK(*vec.nth( 0) == 10);
            CHECK(*vec.nth( 1) == 20);
            CHECK(*vec.nth( 2) == 30);
            CHECK(*vec.nth( 3) == 40);
            CHECK(*vec.nth( 4) == 50);
            CHECK(*vec.nth( 5) == 60);
            CHECK(*vec.nth( 6) == 70);
            CHECK(*vec.nth( 7) == 80);
            CHECK(*vec.nth( 8) ==  1);
            CHECK(*vec.nth( 9) ==  2);
            CHECK(*vec.nth(10) ==  3);
            CHECK(*vec.nth(11) == 90);
        }

        {
            sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

            vec.emplace_back(10);
            vec.emplace_back(20);
            vec.emplace_back(30);
            vec.emplace_back(40);
            vec.emplace_back(50);
            vec.emplace_back(60);
            vec.emplace_back(70);
            vec.emplace_back(80);
            vec.emplace_back(90);

            CHECK(vec.size() == 9);
            CHECK(*vec.nth(0) == 10);
            CHECK(*vec.nth(1) == 20);
            CHECK(*vec.nth(2) == 30);
            CHECK(*vec.nth(3) == 40);
            CHECK(*vec.nth(4) == 50);
            CHECK(*vec.nth(5) == 60);
            CHECK(*vec.nth(6) == 70);
            CHECK(*vec.nth(7) == 80);
            CHECK(*vec.nth(8) == 90);

            std::vector<xint> data({1, 2, 3});

            auto res = vec.insert(vec.nth(9), data.begin(), data.end());

            CHECK(res == vec.nth(9));
            CHECK(vec.size() == 12);
            CHECK(*vec.nth( 0) == 10);
            CHECK(*vec.nth( 1) == 20);
            CHECK(*vec.nth( 2) == 30);
            CHECK(*vec.nth( 3) == 40);
            CHECK(*vec.nth( 4) == 50);
            CHECK(*vec.nth( 5) == 60);
            CHECK(*vec.nth( 6) == 70);
            CHECK(*vec.nth( 7) == 80);
            CHECK(*vec.nth( 8) == 90);
            CHECK(*vec.nth( 9) ==  1);
            CHECK(*vec.nth(10) ==  2);
            CHECK(*vec.nth(11) ==  3);
        }
    }
}

PRINT("Test insert(const_iterator, std::initializer_list");
{
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        CHECK(vec.size() == 9);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
        CHECK(*vec.nth(6) == 70);
        CHECK(*vec.nth(7) == 80);
        CHECK(*vec.nth(8) == 90);

        std::initializer_list<xint> ilist{1, 2, 3};

        auto res = vec.insert(vec.nth(0), ilist);

        CHECK(res == vec.nth(0));
        CHECK(vec.size() == 12);
        CHECK(*vec.nth( 0) ==  1);
        CHECK(*vec.nth( 1) ==  2);
        CHECK(*vec.nth( 2) ==  3);
        CHECK(*vec.nth( 3) == 10);
        CHECK(*vec.nth( 4) == 20);
        CHECK(*vec.nth( 5) == 30);
        CHECK(*vec.nth( 6) == 40);
        CHECK(*vec.nth( 7) == 50);
        CHECK(*vec.nth( 8) == 60);
        CHECK(*vec.nth( 9) == 70);
        CHECK(*vec.nth(10) == 80);
        CHECK(*vec.nth(11) == 90);
    }

    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        CHECK(vec.size() == 9);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
        CHECK(*vec.nth(6) == 70);
        CHECK(*vec.nth(7) == 80);
        CHECK(*vec.nth(8) == 90);

        std::initializer_list<xint> ilist{1, 2, 3};

        auto res = vec.insert(vec.nth(1), ilist);

        CHECK(res == vec.nth(1));
        CHECK(vec.size() == 12);
        CHECK(*vec.nth( 0) == 10);
        CHECK(*vec.nth( 1) ==  1);
        CHECK(*vec.nth( 2) ==  2);
        CHECK(*vec.nth( 3) ==  3);
        CHECK(*vec.nth( 4) == 20);
        CHECK(*vec.nth( 5) == 30);
        CHECK(*vec.nth( 6) == 40);
        CHECK(*vec.nth( 7) == 50);
        CHECK(*vec.nth( 8) == 60);
        CHECK(*vec.nth( 9) == 70);
        CHECK(*vec.nth(10) == 80);
        CHECK(*vec.nth(11) == 90);
    }

    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        CHECK(vec.size() == 9);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
        CHECK(*vec.nth(6) == 70);
        CHECK(*vec.nth(7) == 80);
        CHECK(*vec.nth(8) == 90);

        std::initializer_list<xint> ilist{1, 2, 3};

        auto res = vec.insert(vec.nth(2), ilist);

        CHECK(res == vec.nth(2));
        CHECK(vec.size() == 12);
        CHECK(*vec.nth( 0) == 10);
        CHECK(*vec.nth( 1) == 20);
        CHECK(*vec.nth( 2) ==  1);
        CHECK(*vec.nth( 3) ==  2);
        CHECK(*vec.nth( 4) ==  3);
        CHECK(*vec.nth( 5) == 30);
        CHECK(*vec.nth( 6) == 40);
        CHECK(*vec.nth( 7) == 50);
        CHECK(*vec.nth( 8) == 60);
        CHECK(*vec.nth( 9) == 70);
        CHECK(*vec.nth(10) == 80);
        CHECK(*vec.nth(11) == 90);
    }

    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        CHECK(vec.size() == 9);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
        CHECK(*vec.nth(6) == 70);
        CHECK(*vec.nth(7) == 80);
        CHECK(*vec.nth(8) == 90);

        std::initializer_list<xint> ilist{1, 2, 3};

        auto res = vec.insert(vec.nth(3), ilist);

        CHECK(res == vec.nth(3));
        CHECK(vec.size() == 12);
        CHECK(*vec.nth( 0) == 10);
        CHECK(*vec.nth( 1) == 20);
        CHECK(*vec.nth( 2) == 30);
        CHECK(*vec.nth( 3) ==  1);
        CHECK(*vec.nth( 4) ==  2);
        CHECK(*vec.nth( 5) ==  3);
        CHECK(*vec.nth( 6) == 40);
        CHECK(*vec.nth( 7) == 50);
        CHECK(*vec.nth( 8) == 60);
        CHECK(*vec.nth( 9) == 70);
        CHECK(*vec.nth(10) == 80);
        CHECK(*vec.nth(11) == 90);
    }

    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        CHECK(vec.size() == 9);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
        CHECK(*vec.nth(6) == 70);
        CHECK(*vec.nth(7) == 80);
        CHECK(*vec.nth(8) == 90);

        std::initializer_list<xint> ilist{1, 2, 3};

        auto res = vec.insert(vec.nth(4), ilist);

        CHECK(res == vec.nth(4));
        CHECK(vec.size() == 12);
        CHECK(*vec.nth( 0) == 10);
        CHECK(*vec.nth( 1) == 20);
        CHECK(*vec.nth( 2) == 30);
        CHECK(*vec.nth( 3) == 40);
        CHECK(*vec.nth( 4) ==  1);
        CHECK(*vec.nth( 5) ==  2);
        CHECK(*vec.nth( 6) ==  3);
        CHECK(*vec.nth( 7) == 50);
        CHECK(*vec.nth( 8) == 60);
        CHECK(*vec.nth( 9) == 70);
        CHECK(*vec.nth(10) == 80);
        CHECK(*vec.nth(11) == 90);
    }

    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        CHECK(vec.size() == 9);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
        CHECK(*vec.nth(6) == 70);
        CHECK(*vec.nth(7) == 80);
        CHECK(*vec.nth(8) == 90);

        std::initializer_list<xint> ilist{1, 2, 3};

        auto res = vec.insert(vec.nth(5), ilist);

        CHECK(res == vec.nth(5));
        CHECK(vec.size() == 12);
        CHECK(*vec.nth( 0) == 10);
        CHECK(*vec.nth( 1) == 20);
        CHECK(*vec.nth( 2) == 30);
        CHECK(*vec.nth( 3) == 40);
        CHECK(*vec.nth( 4) == 50);
        CHECK(*vec.nth( 5) ==  1);
        CHECK(*vec.nth( 6) ==  2);
        CHECK(*vec.nth( 7) ==  3);
        CHECK(*vec.nth( 8) == 60);
        CHECK(*vec.nth( 9) == 70);
        CHECK(*vec.nth(10) == 80);
        CHECK(*vec.nth(11) == 90);
    }

    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        CHECK(vec.size() == 9);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
        CHECK(*vec.nth(6) == 70);
        CHECK(*vec.nth(7) == 80);
        CHECK(*vec.nth(8) == 90);

        std::initializer_list<xint> ilist{1, 2, 3};

        auto res = vec.insert(vec.nth(6), ilist);

        CHECK(res == vec.nth(6));
        CHECK(vec.size() == 12);
        CHECK(*vec.nth( 0) == 10);
        CHECK(*vec.nth( 1) == 20);
        CHECK(*vec.nth( 2) == 30);
        CHECK(*vec.nth( 3) == 40);
        CHECK(*vec.nth( 4) == 50);
        CHECK(*vec.nth( 5) == 60);
        CHECK(*vec.nth( 6) ==  1);
        CHECK(*vec.nth( 7) ==  2);
        CHECK(*vec.nth( 8) ==  3);
        CHECK(*vec.nth( 9) == 70);
        CHECK(*vec.nth(10) == 80);
        CHECK(*vec.nth(11) == 90);
    }

    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        CHECK(vec.size() == 9);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
        CHECK(*vec.nth(6) == 70);
        CHECK(*vec.nth(7) == 80);
        CHECK(*vec.nth(8) == 90);

        std::initializer_list<xint> ilist{1, 2, 3};

        auto res = vec.insert(vec.nth(7), ilist);

        CHECK(res == vec.nth(7));
        CHECK(vec.size() == 12);
        CHECK(*vec.nth( 0) == 10);
        CHECK(*vec.nth( 1) == 20);
        CHECK(*vec.nth( 2) == 30);
        CHECK(*vec.nth( 3) == 40);
        CHECK(*vec.nth( 4) == 50);
        CHECK(*vec.nth( 5) == 60);
        CHECK(*vec.nth( 6) == 70);
        CHECK(*vec.nth( 7) ==  1);
        CHECK(*vec.nth( 8) ==  2);
        CHECK(*vec.nth( 9) ==  3);
        CHECK(*vec.nth(10) == 80);
        CHECK(*vec.nth(11) == 90);
    }

    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        CHECK(vec.size() == 9);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
        CHECK(*vec.nth(6) == 70);
        CHECK(*vec.nth(7) == 80);
        CHECK(*vec.nth(8) == 90);

        std::initializer_list<xint> ilist{1, 2, 3};

        auto res = vec.insert(vec.nth(8), ilist);

        CHECK(res == vec.nth(8));
        CHECK(vec.size() == 12);
        CHECK(*vec.nth( 0) == 10);
        CHECK(*vec.nth( 1) == 20);
        CHECK(*vec.nth( 2) == 30);
        CHECK(*vec.nth( 3) == 40);
        CHECK(*vec.nth( 4) == 50);
        CHECK(*vec.nth( 5) == 60);
        CHECK(*vec.nth( 6) == 70);
        CHECK(*vec.nth( 7) == 80);
        CHECK(*vec.nth( 8) ==  1);
        CHECK(*vec.nth( 9) ==  2);
        CHECK(*vec.nth(10) ==  3);
        CHECK(*vec.nth(11) == 90);
    }

    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        CHECK(vec.size() == 9);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
        CHECK(*vec.nth(6) == 70);
        CHECK(*vec.nth(7) == 80);
        CHECK(*vec.nth(8) == 90);

        std::initializer_list<xint> ilist{1, 2, 3};

        auto res = vec.insert(vec.nth(9), ilist);

        CHECK(res == vec.nth(9));
        CHECK(vec.size() == 12);
        CHECK(*vec.nth( 0) == 10);
        CHECK(*vec.nth( 1) == 20);
        CHECK(*vec.nth( 2) == 30);
        CHECK(*vec.nth( 3) == 40);
        CHECK(*vec.nth( 4) == 50);
        CHECK(*vec.nth( 5) == 60);
        CHECK(*vec.nth( 6) == 70);
        CHECK(*vec.nth( 7) == 80);
        CHECK(*vec.nth( 8) == 90);
        CHECK(*vec.nth( 9) ==  1);
        CHECK(*vec.nth(10) ==  2);
        CHECK(*vec.nth(11) ==  3);
    }
}

PRINT("Test insert(const_iterator, Range&&)");
{
    // Input iterator (exactly)
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace_back(10);
        vec.emplace_back(60);

        std::istringstream iss("20 30 40 50");

        #if SFL_CPP_VERSION >= SFL_CPP_20
        vec.insert_range(vec.nth(1), std::views::istream<int>(iss));
        #else
        vec.insert_range(vec.nth(1), sfl::test::istream_view<int>(iss));
        #endif

        CHECK(vec.size() == 6);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
    }

    // Forward iterator
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace_back(10);
        vec.emplace_back(60);

        std::vector<int> data({20, 30, 40, 50});

        #if SFL_CPP_VERSION >= SFL_CPP_20
        vec.insert_range(vec.nth(1), std::views::all(data));
        #else
        vec.insert_range(vec.nth(1), data);
        #endif

        CHECK(vec.size() == 6);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
    }
}

PRINT("Test push_front(const T&)");
{
    sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

    xint value_10(10);

    vec.push_front(value_10);

    CHECK(vec.size() == 1);
    CHECK(*vec.nth(0) == 10);
    CHECK(value_10 == 10);
}

PRINT("Test push_front(T&&)");
{
    sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

    xint value_10(10);

    vec.push_front(std::move(value_10));

    CHECK(vec.size() == 1);
    CHECK(*vec.nth(0) == 10);
    CHECK(value_10 == -10);
}

PRINT("Test push_back(const T&)");
{
    sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

    xint value_10(10);

    vec.push_back(value_10);

    CHECK(vec.size() == 1);
    CHECK(*vec.nth(0) == 10);
    CHECK(value_10 == 10);
}

PRINT("Test push_back(T&&)");
{
    sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

    xint value_10(10);

    vec.push_back(std::move(value_10));

    CHECK(vec.size() == 1);
    CHECK(*vec.nth(0) == 10);
    CHECK(value_10 == -10);
}

PRINT("Test prepend_range(Range&&)");
{
    // Input iterator (exactly)
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace_back(50);
        vec.emplace_back(60);

        std::istringstream iss("10 20 30 40");

        #if SFL_CPP_VERSION >= SFL_CPP_20
        vec.prepend_range(std::views::istream<int>(iss));
        #else
        vec.prepend_range(sfl::test::istream_view<int>(iss));
        #endif

        CHECK(vec.size() == 6);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
    }

    // Forward iterator
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace_back(50);
        vec.emplace_back(60);

        std::vector<int> data({10, 20, 30, 40});

        #if SFL_CPP_VERSION >= SFL_CPP_20
        vec.prepend_range(std::views::all(data));
        #else
        vec.prepend_range(data);
        #endif

        CHECK(vec.size() == 6);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
    }
}

PRINT("Test append_range(Range&&)");
{
    // Input iterator (exactly)
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace_back(10);
        vec.emplace_back(20);

        std::istringstream iss("30 40 50 60");

        #if SFL_CPP_VERSION >= SFL_CPP_20
        vec.append_range(std::views::istream<int>(iss));
        #else
        vec.append_range(sfl::test::istream_view<int>(iss));
        #endif

        CHECK(vec.size() == 6);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
    }

    // Forward iterator
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace_back(10);
        vec.emplace_back(20);

        std::vector<int> data({30, 40, 50, 60});

        #if SFL_CPP_VERSION >= SFL_CPP_20
        vec.append_range(std::views::all(data));
        #else
        vec.append_range(data);
        #endif

        CHECK(vec.size() == 6);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
    }
}

PRINT("Test pop_front()");
{
    sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

    vec.emplace_back(10);
    vec.emplace_back(20);
    vec.emplace_back(30);

    CHECK(vec.size() == 3);
    CHECK(*vec.nth(0) == 10);
    CHECK(*vec.nth(1) == 20);
    CHECK(*vec.nth(2) == 30);

    ///////////////////////////////////////////////////////////////////////////

    vec.pop_front();

    CHECK(vec.size() == 2);
    CHECK(*vec.nth(0) == 20);
    CHECK(*vec.nth(1) == 30);

    ///////////////////////////////////////////////////////////////////////////

    vec.pop_front();

    CHECK(vec.size() == 1);
    CHECK(*vec.nth(0) == 30);

    ///////////////////////////////////////////////////////////////////////////

    vec.pop_front();

    CHECK(vec.size() == 0);
}

PRINT("Test pop_back()");
{
    sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

    vec.emplace_back(10);
    vec.emplace_back(20);
    vec.emplace_back(30);

    CHECK(vec.size() == 3);
    CHECK(*vec.nth(0) == 10);
    CHECK(*vec.nth(1) == 20);
    CHECK(*vec.nth(2) == 30);

    ///////////////////////////////////////////////////////////////////////////

    vec.pop_back();

    CHECK(vec.size() == 2);
    CHECK(*vec.nth(0) == 10);
    CHECK(*vec.nth(1) == 20);

    ///////////////////////////////////////////////////////////////////////////

    vec.pop_back();

    CHECK(vec.size() == 1);
    CHECK(*vec.nth(0) == 10);

    ///////////////////////////////////////////////////////////////////////////

    vec.pop_back();

    CHECK(vec.size() == 0);
}

PRINT("Test erase(const_iterator)");
{
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        CHECK(vec.size() == 9);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
        CHECK(*vec.nth(6) == 70);
        CHECK(*vec.nth(7) == 80);
        CHECK(*vec.nth(8) == 90);

        auto res = vec.erase(vec.nth(0));

        CHECK(res == vec.nth(0));
        CHECK(vec.size() == 8);
        CHECK(*vec.nth(0) == 20);
        CHECK(*vec.nth(1) == 30);
        CHECK(*vec.nth(2) == 40);
        CHECK(*vec.nth(3) == 50);
        CHECK(*vec.nth(4) == 60);
        CHECK(*vec.nth(5) == 70);
        CHECK(*vec.nth(6) == 80);
        CHECK(*vec.nth(7) == 90);
    }

    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        CHECK(vec.size() == 9);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
        CHECK(*vec.nth(6) == 70);
        CHECK(*vec.nth(7) == 80);
        CHECK(*vec.nth(8) == 90);

        auto res = vec.erase(vec.nth(1));

        CHECK(res == vec.nth(1));
        CHECK(vec.size() == 8);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 30);
        CHECK(*vec.nth(2) == 40);
        CHECK(*vec.nth(3) == 50);
        CHECK(*vec.nth(4) == 60);
        CHECK(*vec.nth(5) == 70);
        CHECK(*vec.nth(6) == 80);
        CHECK(*vec.nth(7) == 90);
    }

    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        CHECK(vec.size() == 9);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
        CHECK(*vec.nth(6) == 70);
        CHECK(*vec.nth(7) == 80);
        CHECK(*vec.nth(8) == 90);

        auto res = vec.erase(vec.nth(2));

        CHECK(res == vec.nth(2));
        CHECK(vec.size() == 8);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 40);
        CHECK(*vec.nth(3) == 50);
        CHECK(*vec.nth(4) == 60);
        CHECK(*vec.nth(5) == 70);
        CHECK(*vec.nth(6) == 80);
        CHECK(*vec.nth(7) == 90);
    }

    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        CHECK(vec.size() == 9);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
        CHECK(*vec.nth(6) == 70);
        CHECK(*vec.nth(7) == 80);
        CHECK(*vec.nth(8) == 90);

        auto res = vec.erase(vec.nth(3));

        CHECK(res == vec.nth(3));
        CHECK(vec.size() == 8);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 50);
        CHECK(*vec.nth(4) == 60);
        CHECK(*vec.nth(5) == 70);
        CHECK(*vec.nth(6) == 80);
        CHECK(*vec.nth(7) == 90);
    }

    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        CHECK(vec.size() == 9);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
        CHECK(*vec.nth(6) == 70);
        CHECK(*vec.nth(7) == 80);
        CHECK(*vec.nth(8) == 90);

        auto res = vec.erase(vec.nth(4));

        CHECK(res == vec.nth(4));
        CHECK(vec.size() == 8);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 60);
        CHECK(*vec.nth(5) == 70);
        CHECK(*vec.nth(6) == 80);
        CHECK(*vec.nth(7) == 90);
    }

    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        CHECK(vec.size() == 9);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
        CHECK(*vec.nth(6) == 70);
        CHECK(*vec.nth(7) == 80);
        CHECK(*vec.nth(8) == 90);

        auto res = vec.erase(vec.nth(5));

        CHECK(res == vec.nth(5));
        CHECK(vec.size() == 8);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 70);
        CHECK(*vec.nth(6) == 80);
        CHECK(*vec.nth(7) == 90);
    }

    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        CHECK(vec.size() == 9);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
        CHECK(*vec.nth(6) == 70);
        CHECK(*vec.nth(7) == 80);
        CHECK(*vec.nth(8) == 90);

        auto res = vec.erase(vec.nth(6));

        CHECK(res == vec.nth(6));
        CHECK(vec.size() == 8);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
        CHECK(*vec.nth(6) == 80);
        CHECK(*vec.nth(7) == 90);
    }

    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        CHECK(vec.size() == 9);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
        CHECK(*vec.nth(6) == 70);
        CHECK(*vec.nth(7) == 80);
        CHECK(*vec.nth(8) == 90);

        auto res = vec.erase(vec.nth(7));

        CHECK(res == vec.nth(7));
        CHECK(vec.size() == 8);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
        CHECK(*vec.nth(6) == 70);
        CHECK(*vec.nth(7) == 90);
    }

    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        CHECK(vec.size() == 9);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
        CHECK(*vec.nth(6) == 70);
        CHECK(*vec.nth(7) == 80);
        CHECK(*vec.nth(8) == 90);

        auto res = vec.erase(vec.nth(8));

        CHECK(res == vec.nth(8));
        CHECK(vec.size() == 8);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
        CHECK(*vec.nth(6) == 70);
        CHECK(*vec.nth(7) == 80);
    }
}

PRINT("Test erase(const_iterator, const_iterator)");
{
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        CHECK(vec.size() == 0);

        CHECK(vec.erase(vec.nth(0), vec.nth(0)) == vec.nth(0));
    }

    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        CHECK(vec.size() == 9);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
        CHECK(*vec.nth(6) == 70);
        CHECK(*vec.nth(7) == 80);
        CHECK(*vec.nth(8) == 90);

        CHECK(vec.erase(vec.nth(0), vec.nth(0)) == vec.nth(0));
        CHECK(vec.erase(vec.nth(1), vec.nth(1)) == vec.nth(1));
        CHECK(vec.erase(vec.nth(2), vec.nth(2)) == vec.nth(2));
        CHECK(vec.erase(vec.nth(3), vec.nth(3)) == vec.nth(3));
        CHECK(vec.erase(vec.nth(4), vec.nth(4)) == vec.nth(4));
        CHECK(vec.erase(vec.nth(5), vec.nth(5)) == vec.nth(5));
        CHECK(vec.erase(vec.nth(6), vec.nth(6)) == vec.nth(6));
        CHECK(vec.erase(vec.nth(7), vec.nth(7)) == vec.nth(7));
        CHECK(vec.erase(vec.nth(8), vec.nth(8)) == vec.nth(8));
        CHECK(vec.erase(vec.nth(9), vec.nth(9)) == vec.nth(9));
    }

    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        CHECK(vec.size() == 9);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
        CHECK(*vec.nth(6) == 70);
        CHECK(*vec.nth(7) == 80);
        CHECK(*vec.nth(8) == 90);

        auto res = vec.erase(vec.nth(0), vec.nth(3));

        CHECK(res == vec.nth(0));
        CHECK(vec.size() == 6);
        CHECK(*vec.nth(0) == 40);
        CHECK(*vec.nth(1) == 50);
        CHECK(*vec.nth(2) == 60);
        CHECK(*vec.nth(3) == 70);
        CHECK(*vec.nth(4) == 80);
        CHECK(*vec.nth(5) == 90);
    }

    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        CHECK(vec.size() == 9);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
        CHECK(*vec.nth(6) == 70);
        CHECK(*vec.nth(7) == 80);
        CHECK(*vec.nth(8) == 90);

        auto res = vec.erase(vec.nth(1), vec.nth(4));

        CHECK(res == vec.nth(1));
        CHECK(vec.size() == 6);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 50);
        CHECK(*vec.nth(2) == 60);
        CHECK(*vec.nth(3) == 70);
        CHECK(*vec.nth(4) == 80);
        CHECK(*vec.nth(5) == 90);
    }

    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        CHECK(vec.size() == 9);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
        CHECK(*vec.nth(6) == 70);
        CHECK(*vec.nth(7) == 80);
        CHECK(*vec.nth(8) == 90);

        auto res = vec.erase(vec.nth(2), vec.nth(5));

        CHECK(res == vec.nth(2));
        CHECK(vec.size() == 6);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 60);
        CHECK(*vec.nth(3) == 70);
        CHECK(*vec.nth(4) == 80);
        CHECK(*vec.nth(5) == 90);
    }

    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        CHECK(vec.size() == 9);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
        CHECK(*vec.nth(6) == 70);
        CHECK(*vec.nth(7) == 80);
        CHECK(*vec.nth(8) == 90);

        auto res = vec.erase(vec.nth(3), vec.nth(6));

        CHECK(res == vec.nth(3));
        CHECK(vec.size() == 6);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 70);
        CHECK(*vec.nth(4) == 80);
        CHECK(*vec.nth(5) == 90);
    }

    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        CHECK(vec.size() == 9);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
        CHECK(*vec.nth(6) == 70);
        CHECK(*vec.nth(7) == 80);
        CHECK(*vec.nth(8) == 90);

        auto res = vec.erase(vec.nth(4), vec.nth(7));

        CHECK(res == vec.nth(4));
        CHECK(vec.size() == 6);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 80);
        CHECK(*vec.nth(5) == 90);
    }

    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        CHECK(vec.size() == 9);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
        CHECK(*vec.nth(6) == 70);
        CHECK(*vec.nth(7) == 80);
        CHECK(*vec.nth(8) == 90);

        auto res = vec.erase(vec.nth(5), vec.nth(8));

        CHECK(res == vec.nth(5));
        CHECK(vec.size() == 6);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 90);
    }

    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        CHECK(vec.size() == 9);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
        CHECK(*vec.nth(6) == 70);
        CHECK(*vec.nth(7) == 80);
        CHECK(*vec.nth(8) == 90);

        auto res = vec.erase(vec.nth(6), vec.nth(9));

        CHECK(res == vec.nth(6));
        CHECK(vec.size() == 6);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        CHECK(*vec.nth(5) == 60);
    }
}

PRINT("Test resize(size_type)");
{
    #define CONDITION n < vec.size()
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        using size_type = typename sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::size_type;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        vec.pop_front();
        vec.pop_front();

        vec.pop_back();
        vec.pop_back();

        CHECK(vec.size() == 5);
        CHECK(*vec.nth(0) == 30);
        CHECK(*vec.nth(1) == 40);
        CHECK(*vec.nth(2) == 50);
        CHECK(*vec.nth(3) == 60);
        CHECK(*vec.nth(4) == 70);

        const size_type n = vec.size() - 1;

        CHECK(CONDITION);

        vec.resize(n);

        CHECK(vec.size() == 4);
        CHECK(*vec.nth(0) == 30);
        CHECK(*vec.nth(1) == 40);
        CHECK(*vec.nth(2) == 50);
        CHECK(*vec.nth(3) == 60);
    }
    #undef CONDITION

    #define CONDITION n == vec.size()
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        using size_type = typename sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::size_type;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        vec.pop_front();
        vec.pop_front();

        vec.pop_back();
        vec.pop_back();

        CHECK(vec.size() == 5);
        CHECK(*vec.nth(0) == 30);
        CHECK(*vec.nth(1) == 40);
        CHECK(*vec.nth(2) == 50);
        CHECK(*vec.nth(3) == 60);
        CHECK(*vec.nth(4) == 70);

        const size_type n = vec.size();

        CHECK(CONDITION);

        vec.resize(n);

        CHECK(vec.size() == 5);
        CHECK(*vec.nth(0) == 30);
        CHECK(*vec.nth(1) == 40);
        CHECK(*vec.nth(2) == 50);
        CHECK(*vec.nth(3) == 60);
        CHECK(*vec.nth(4) == 70);
    }
    #undef CONDITION

    #define CONDITION n > vec.size() && \
                      n < vec.size() + vec.available_back()
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        using size_type = typename sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::size_type;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        vec.pop_front();
        vec.pop_front();

        vec.pop_back();
        vec.pop_back();

        CHECK(vec.size() == 5);
        CHECK(*vec.nth(0) == 30);
        CHECK(*vec.nth(1) == 40);
        CHECK(*vec.nth(2) == 50);
        CHECK(*vec.nth(3) == 60);
        CHECK(*vec.nth(4) == 70);

        const size_type size = vec.size();

        const size_type available_back = vec.available_back();

        const size_type n = size + available_back - 1;

        CHECK(CONDITION);

        vec.resize(n);

        CHECK(vec.size() == n);
        CHECK(*vec.nth(0) == 30);
        CHECK(*vec.nth(1) == 40);
        CHECK(*vec.nth(2) == 50);
        CHECK(*vec.nth(3) == 60);
        CHECK(*vec.nth(4) == 70);
        for (size_type i = size; i < n; ++i)
        {
            CHECK(*vec.nth(i) == SFL_TEST_XINT_DEFAULT_VALUE);
        }
    }
    #undef CONDITION

    #define CONDITION n > vec.size() && \
                      n == vec.size() + vec.available_back()
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        using size_type = typename sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::size_type;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        vec.pop_front();
        vec.pop_front();

        vec.pop_back();
        vec.pop_back();

        CHECK(vec.size() == 5);
        CHECK(*vec.nth(0) == 30);
        CHECK(*vec.nth(1) == 40);
        CHECK(*vec.nth(2) == 50);
        CHECK(*vec.nth(3) == 60);
        CHECK(*vec.nth(4) == 70);

        const size_type size = vec.size();

        const size_type available_back = vec.available_back();

        const size_type n = size + available_back;

        CHECK(CONDITION);

        vec.resize(n);

        CHECK(vec.size() == n);
        CHECK(*vec.nth(0) == 30);
        CHECK(*vec.nth(1) == 40);
        CHECK(*vec.nth(2) == 50);
        CHECK(*vec.nth(3) == 60);
        CHECK(*vec.nth(4) == 70);
        for (size_type i = size; i < n; ++i)
        {
            CHECK(*vec.nth(i) == SFL_TEST_XINT_DEFAULT_VALUE);
        }
    }
    #undef CONDITION

    #define CONDITION n > vec.size() && \
                      n > vec.size() + vec.available_back() && \
                      n < vec.available_front() + vec.size() + vec.available_back()
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        using size_type = typename sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::size_type;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        vec.pop_front();
        vec.pop_front();

        vec.pop_back();
        vec.pop_back();

        CHECK(vec.size() == 5);
        CHECK(*vec.nth(0) == 30);
        CHECK(*vec.nth(1) == 40);
        CHECK(*vec.nth(2) == 50);
        CHECK(*vec.nth(3) == 60);
        CHECK(*vec.nth(4) == 70);

        const size_type available_front = vec.available_front();

        const size_type size = vec.size();

        const size_type available_back = vec.available_back();

        const size_type n = available_front + size + available_back - 1;

        CHECK(CONDITION);

        vec.resize(n);

        CHECK(vec.size() == n);
        for (size_type i = 0; i < n - (size + available_back); ++i)
        {
            CHECK(*vec.nth(i) == SFL_TEST_XINT_DEFAULT_VALUE);
        }
        CHECK(*vec.nth(n - (size + available_back) + 0) == 30);
        CHECK(*vec.nth(n - (size + available_back) + 1) == 40);
        CHECK(*vec.nth(n - (size + available_back) + 2) == 50);
        CHECK(*vec.nth(n - (size + available_back) + 3) == 60);
        CHECK(*vec.nth(n - (size + available_back) + 4) == 70);
        for (size_type i = n - available_back; i < available_back; ++i)
        {
            CHECK(*vec.nth(i) == SFL_TEST_XINT_DEFAULT_VALUE);
        }
    }
    #undef CONDITION

    #define CONDITION n > vec.size() && \
                      n > vec.size() + vec.available_back() && \
                      n == vec.available_front() + vec.size() + vec.available_back()
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        using size_type = typename sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::size_type;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        vec.pop_front();
        vec.pop_front();

        vec.pop_back();
        vec.pop_back();

        CHECK(vec.size() == 5);
        CHECK(*vec.nth(0) == 30);
        CHECK(*vec.nth(1) == 40);
        CHECK(*vec.nth(2) == 50);
        CHECK(*vec.nth(3) == 60);
        CHECK(*vec.nth(4) == 70);

        const size_type available_front = vec.available_front();

        const size_type size = vec.size();

        const size_type available_back = vec.available_back();

        const size_type n = available_front + size + available_back;

        CHECK(CONDITION);

        vec.resize(n);

        CHECK(vec.size() == n);
        for (size_type i = 0; i < n - (size + available_back); ++i)
        {
            CHECK(*vec.nth(i) == SFL_TEST_XINT_DEFAULT_VALUE);
        }
        CHECK(*vec.nth(n - (size + available_back) + 0) == 30);
        CHECK(*vec.nth(n - (size + available_back) + 1) == 40);
        CHECK(*vec.nth(n - (size + available_back) + 2) == 50);
        CHECK(*vec.nth(n - (size + available_back) + 3) == 60);
        CHECK(*vec.nth(n - (size + available_back) + 4) == 70);
        for (size_type i = n - available_back; i < available_back; ++i)
        {
            CHECK(*vec.nth(i) == SFL_TEST_XINT_DEFAULT_VALUE);
        }
    }
    #undef CONDITION

    #define CONDITION n > vec.size() && \
                      n > vec.size() + vec.available_back() && \
                      n > vec.available_front() + vec.size() + vec.available_back()
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        using size_type = typename sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::size_type;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        vec.pop_front();
        vec.pop_front();

        vec.pop_back();
        vec.pop_back();

        CHECK(vec.size() == 5);
        CHECK(*vec.nth(0) == 30);
        CHECK(*vec.nth(1) == 40);
        CHECK(*vec.nth(2) == 50);
        CHECK(*vec.nth(3) == 60);
        CHECK(*vec.nth(4) == 70);

        const size_type available_front = vec.available_front();

        const size_type size = vec.size();

        const size_type available_back = vec.available_back();

        const size_type n = available_front + size + available_back + 1;

        CHECK(CONDITION);

        vec.resize(n);

        CHECK(vec.size() == n);
        for (size_type i = 0; i < available_front; ++i)
        {
            CHECK(*vec.nth(i) == SFL_TEST_XINT_DEFAULT_VALUE);
        }
        CHECK(*vec.nth(available_front + 0) == 30);
        CHECK(*vec.nth(available_front + 1) == 40);
        CHECK(*vec.nth(available_front + 2) == 50);
        CHECK(*vec.nth(available_front + 3) == 60);
        CHECK(*vec.nth(available_front + 4) == 70);
        for (size_type i = available_front + size; i < n; ++i)
        {
            CHECK(*vec.nth(i) == SFL_TEST_XINT_DEFAULT_VALUE);
        }
    }
    #undef CONDITION
}

PRINT("Test resize(size_type, const T&)");
{
    #define CONDITION n < vec.size()
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        using size_type = typename sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::size_type;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        vec.pop_front();
        vec.pop_front();

        vec.pop_back();
        vec.pop_back();

        CHECK(vec.size() == 5);
        CHECK(*vec.nth(0) == 30);
        CHECK(*vec.nth(1) == 40);
        CHECK(*vec.nth(2) == 50);
        CHECK(*vec.nth(3) == 60);
        CHECK(*vec.nth(4) == 70);

        const size_type n = vec.size() - 1;

        CHECK(CONDITION);

        xint value(987654);

        vec.resize(n, value);

        CHECK(vec.size() == 4);
        CHECK(*vec.nth(0) == 30);
        CHECK(*vec.nth(1) == 40);
        CHECK(*vec.nth(2) == 50);
        CHECK(*vec.nth(3) == 60);
    }
    #undef CONDITION

    #define CONDITION n == vec.size()
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        using size_type = typename sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::size_type;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        vec.pop_front();
        vec.pop_front();

        vec.pop_back();
        vec.pop_back();

        CHECK(vec.size() == 5);
        CHECK(*vec.nth(0) == 30);
        CHECK(*vec.nth(1) == 40);
        CHECK(*vec.nth(2) == 50);
        CHECK(*vec.nth(3) == 60);
        CHECK(*vec.nth(4) == 70);

        const size_type n = vec.size();

        CHECK(CONDITION);

        xint value(987654);

        vec.resize(n, value);

        CHECK(vec.size() == 5);
        CHECK(*vec.nth(0) == 30);
        CHECK(*vec.nth(1) == 40);
        CHECK(*vec.nth(2) == 50);
        CHECK(*vec.nth(3) == 60);
        CHECK(*vec.nth(4) == 70);
    }
    #undef CONDITION

    #define CONDITION n > vec.size() && \
                      n < vec.size() + vec.available_back()
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        using size_type = typename sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::size_type;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        vec.pop_front();
        vec.pop_front();

        vec.pop_back();
        vec.pop_back();

        CHECK(vec.size() == 5);
        CHECK(*vec.nth(0) == 30);
        CHECK(*vec.nth(1) == 40);
        CHECK(*vec.nth(2) == 50);
        CHECK(*vec.nth(3) == 60);
        CHECK(*vec.nth(4) == 70);

        const size_type size = vec.size();

        const size_type available_back = vec.available_back();

        const size_type n = size + available_back - 1;

        CHECK(CONDITION);

        xint value(987654);

        vec.resize(n, value);

        CHECK(vec.size() == n);
        CHECK(*vec.nth(0) == 30);
        CHECK(*vec.nth(1) == 40);
        CHECK(*vec.nth(2) == 50);
        CHECK(*vec.nth(3) == 60);
        CHECK(*vec.nth(4) == 70);
        for (size_type i = size; i < n; ++i)
        {
            CHECK(*vec.nth(i) == value);
        }
    }
    #undef CONDITION

    #define CONDITION n > vec.size() && \
                      n == vec.size() + vec.available_back()
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        using size_type = typename sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::size_type;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        vec.pop_front();
        vec.pop_front();

        vec.pop_back();
        vec.pop_back();

        CHECK(vec.size() == 5);
        CHECK(*vec.nth(0) == 30);
        CHECK(*vec.nth(1) == 40);
        CHECK(*vec.nth(2) == 50);
        CHECK(*vec.nth(3) == 60);
        CHECK(*vec.nth(4) == 70);

        const size_type size = vec.size();

        const size_type available_back = vec.available_back();

        const size_type n = size + available_back;

        CHECK(CONDITION);

        xint value(987654);

        vec.resize(n, value);

        CHECK(vec.size() == n);
        CHECK(*vec.nth(0) == 30);
        CHECK(*vec.nth(1) == 40);
        CHECK(*vec.nth(2) == 50);
        CHECK(*vec.nth(3) == 60);
        CHECK(*vec.nth(4) == 70);
        for (size_type i = size; i < n; ++i)
        {
            CHECK(*vec.nth(i) == value);
        }
    }
    #undef CONDITION

    #define CONDITION n > vec.size() && \
                      n > vec.size() + vec.available_back() && \
                      n < vec.available_front() + vec.size() + vec.available_back()
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        using size_type = typename sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::size_type;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        vec.pop_front();
        vec.pop_front();

        vec.pop_back();
        vec.pop_back();

        CHECK(vec.size() == 5);
        CHECK(*vec.nth(0) == 30);
        CHECK(*vec.nth(1) == 40);
        CHECK(*vec.nth(2) == 50);
        CHECK(*vec.nth(3) == 60);
        CHECK(*vec.nth(4) == 70);

        const size_type available_front = vec.available_front();

        const size_type size = vec.size();

        const size_type available_back = vec.available_back();

        const size_type n = available_front + size + available_back - 1;

        CHECK(CONDITION);

        xint value(987654);

        vec.resize(n, value);

        CHECK(vec.size() == n);
        for (size_type i = 0; i < n - (size + available_back); ++i)
        {
            CHECK(*vec.nth(i) == value);
        }
        CHECK(*vec.nth(n - (size + available_back) + 0) == 30);
        CHECK(*vec.nth(n - (size + available_back) + 1) == 40);
        CHECK(*vec.nth(n - (size + available_back) + 2) == 50);
        CHECK(*vec.nth(n - (size + available_back) + 3) == 60);
        CHECK(*vec.nth(n - (size + available_back) + 4) == 70);
        for (size_type i = n - available_back; i < available_back; ++i)
        {
            CHECK(*vec.nth(i) == value);
        }
    }
    #undef CONDITION

    #define CONDITION n > vec.size() && \
                      n > vec.size() + vec.available_back() && \
                      n == vec.available_front() + vec.size() + vec.available_back()
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        using size_type = typename sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::size_type;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        vec.pop_front();
        vec.pop_front();

        vec.pop_back();
        vec.pop_back();

        CHECK(vec.size() == 5);
        CHECK(*vec.nth(0) == 30);
        CHECK(*vec.nth(1) == 40);
        CHECK(*vec.nth(2) == 50);
        CHECK(*vec.nth(3) == 60);
        CHECK(*vec.nth(4) == 70);

        const size_type available_front = vec.available_front();

        const size_type size = vec.size();

        const size_type available_back = vec.available_back();

        const size_type n = available_front + size + available_back;

        CHECK(CONDITION);

        xint value(987654);

        vec.resize(n, value);

        CHECK(vec.size() == n);
        for (size_type i = 0; i < n - (size + available_back); ++i)
        {
            CHECK(*vec.nth(i) == value);
        }
        CHECK(*vec.nth(n - (size + available_back) + 0) == 30);
        CHECK(*vec.nth(n - (size + available_back) + 1) == 40);
        CHECK(*vec.nth(n - (size + available_back) + 2) == 50);
        CHECK(*vec.nth(n - (size + available_back) + 3) == 60);
        CHECK(*vec.nth(n - (size + available_back) + 4) == 70);
        for (size_type i = n - available_back; i < available_back; ++i)
        {
            CHECK(*vec.nth(i) == value);
        }
    }
    #undef CONDITION

    #define CONDITION n > vec.size() && \
                      n > vec.size() + vec.available_back() && \
                      n > vec.available_front() + vec.size() + vec.available_back()
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        using size_type = typename sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::size_type;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        vec.pop_front();
        vec.pop_front();

        vec.pop_back();
        vec.pop_back();

        CHECK(vec.size() == 5);
        CHECK(*vec.nth(0) == 30);
        CHECK(*vec.nth(1) == 40);
        CHECK(*vec.nth(2) == 50);
        CHECK(*vec.nth(3) == 60);
        CHECK(*vec.nth(4) == 70);

        const size_type available_front = vec.available_front();

        const size_type size = vec.size();

        const size_type available_back = vec.available_back();

        const size_type n = available_front + size + available_back + 1;

        CHECK(CONDITION);

        xint value(987654);

        vec.resize(n, value);

        CHECK(vec.size() == n);
        for (size_type i = 0; i < available_front; ++i)
        {
            CHECK(*vec.nth(i) == value);
        }
        CHECK(*vec.nth(available_front + 0) == 30);
        CHECK(*vec.nth(available_front + 1) == 40);
        CHECK(*vec.nth(available_front + 2) == 50);
        CHECK(*vec.nth(available_front + 3) == 60);
        CHECK(*vec.nth(available_front + 4) == 70);
        for (size_type i = available_front + size; i < n; ++i)
        {
            CHECK(*vec.nth(i) == value);
        }
    }
    #undef CONDITION
}

PRINT("Test resize_front(size_type)");
{
    #define CONDITION n < vec.size()
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        using size_type = typename sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::size_type;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);

        vec.pop_front();
        vec.pop_front();

        CHECK(vec.size() == 5);
        CHECK(*vec.nth(0) == 30);
        CHECK(*vec.nth(1) == 40);
        CHECK(*vec.nth(2) == 50);
        CHECK(*vec.nth(3) == 60);
        CHECK(*vec.nth(4) == 70);

        const size_type n = vec.size() - 1;

        CHECK(CONDITION);

        vec.resize_front(n);

        CHECK(vec.size() == 4);
        CHECK(*vec.nth(0) == 40);
        CHECK(*vec.nth(1) == 50);
        CHECK(*vec.nth(2) == 60);
        CHECK(*vec.nth(3) == 70);
    }
    #undef CONDITION

    #define CONDITION n == vec.size()
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        using size_type = typename sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::size_type;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);

        vec.pop_front();
        vec.pop_front();

        CHECK(vec.size() == 5);
        CHECK(*vec.nth(0) == 30);
        CHECK(*vec.nth(1) == 40);
        CHECK(*vec.nth(2) == 50);
        CHECK(*vec.nth(3) == 60);
        CHECK(*vec.nth(4) == 70);

        const size_type n = vec.size();

        CHECK(CONDITION);

        vec.resize_front(n);

        CHECK(vec.size() == 5);
        CHECK(*vec.nth(0) == 30);
        CHECK(*vec.nth(1) == 40);
        CHECK(*vec.nth(2) == 50);
        CHECK(*vec.nth(3) == 60);
        CHECK(*vec.nth(4) == 70);
    }
    #undef CONDITION

    #define CONDITION n > vec.size() && n < vec.size() + vec.available_front()
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        using size_type = typename sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::size_type;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);

        vec.pop_front();
        vec.pop_front();

        CHECK(vec.size() == 5);
        CHECK(*vec.nth(0) == 30);
        CHECK(*vec.nth(1) == 40);
        CHECK(*vec.nth(2) == 50);
        CHECK(*vec.nth(3) == 60);
        CHECK(*vec.nth(4) == 70);

        const size_type size = vec.size();

        const size_type available_front = vec.available_front();

        const size_type n = size + available_front - 1;

        CHECK(CONDITION);

        vec.resize_front(n);

        CHECK(vec.size() == n);
        for (size_type i = 0; i < n - size; ++i)
        {
            CHECK(*vec.nth(i) == SFL_TEST_XINT_DEFAULT_VALUE);
        }
        CHECK(*vec.nth((n - size) + 0) == 30);
        CHECK(*vec.nth((n - size) + 1) == 40);
        CHECK(*vec.nth((n - size) + 2) == 50);
        CHECK(*vec.nth((n - size) + 3) == 60);
        CHECK(*vec.nth((n - size) + 4) == 70);
    }
    #undef CONDITION

    #define CONDITION n > vec.size() && n == vec.size() + vec.available_front()
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        using size_type = typename sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::size_type;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);

        vec.pop_front();
        vec.pop_front();

        CHECK(vec.size() == 5);
        CHECK(*vec.nth(0) == 30);
        CHECK(*vec.nth(1) == 40);
        CHECK(*vec.nth(2) == 50);
        CHECK(*vec.nth(3) == 60);
        CHECK(*vec.nth(4) == 70);

        const size_type size = vec.size();

        const size_type available_front = vec.available_front();

        const size_type n = size + available_front;

        CHECK(CONDITION);

        vec.resize_front(n);

        CHECK(vec.size() == n);
        for (size_type i = 0; i < n - size; ++i)
        {
            CHECK(*vec.nth(i) == SFL_TEST_XINT_DEFAULT_VALUE);
        }
        CHECK(*vec.nth((n - size) + 0) == 30);
        CHECK(*vec.nth((n - size) + 1) == 40);
        CHECK(*vec.nth((n - size) + 2) == 50);
        CHECK(*vec.nth((n - size) + 3) == 60);
        CHECK(*vec.nth((n - size) + 4) == 70);
    }
    #undef CONDITION

    #define CONDITION n > vec.size() && n > vec.size() + vec.available_front()
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        using size_type = typename sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::size_type;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);

        vec.pop_front();
        vec.pop_front();

        CHECK(vec.size() == 5);
        CHECK(*vec.nth(0) == 30);
        CHECK(*vec.nth(1) == 40);
        CHECK(*vec.nth(2) == 50);
        CHECK(*vec.nth(3) == 60);
        CHECK(*vec.nth(4) == 70);

        const size_type size = vec.size();

        const size_type available_front = vec.available_front();

        const size_type n = size + available_front + 1;

        CHECK(CONDITION);

        vec.resize_front(n);

        CHECK(vec.size() == n);
        for (size_type i = 0; i < n - size; ++i)
        {
            CHECK(*vec.nth(i) == SFL_TEST_XINT_DEFAULT_VALUE);
        }
        CHECK(*vec.nth((n - size) + 0) == 30);
        CHECK(*vec.nth((n - size) + 1) == 40);
        CHECK(*vec.nth((n - size) + 2) == 50);
        CHECK(*vec.nth((n - size) + 3) == 60);
        CHECK(*vec.nth((n - size) + 4) == 70);
    }
    #undef CONDITION
}

PRINT("Test resize_front(size_type, const T&)");
{
    #define CONDITION n < vec.size()
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        using size_type = typename sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::size_type;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);

        vec.pop_front();
        vec.pop_front();

        CHECK(vec.size() == 5);
        CHECK(*vec.nth(0) == 30);
        CHECK(*vec.nth(1) == 40);
        CHECK(*vec.nth(2) == 50);
        CHECK(*vec.nth(3) == 60);
        CHECK(*vec.nth(4) == 70);

        const size_type n = vec.size() - 1;

        CHECK(CONDITION);

        xint value(987654);

        vec.resize_front(n, value);

        CHECK(vec.size() == 4);
        CHECK(*vec.nth(0) == 40);
        CHECK(*vec.nth(1) == 50);
        CHECK(*vec.nth(2) == 60);
        CHECK(*vec.nth(3) == 70);
    }
    #undef CONDITION

    #define CONDITION n == vec.size()
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        using size_type = typename sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::size_type;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);

        vec.pop_front();
        vec.pop_front();

        CHECK(vec.size() == 5);
        CHECK(*vec.nth(0) == 30);
        CHECK(*vec.nth(1) == 40);
        CHECK(*vec.nth(2) == 50);
        CHECK(*vec.nth(3) == 60);
        CHECK(*vec.nth(4) == 70);

        const size_type n = vec.size();

        CHECK(CONDITION);

        xint value(987654);

        vec.resize_front(n, value);

        CHECK(vec.size() == 5);
        CHECK(*vec.nth(0) == 30);
        CHECK(*vec.nth(1) == 40);
        CHECK(*vec.nth(2) == 50);
        CHECK(*vec.nth(3) == 60);
        CHECK(*vec.nth(4) == 70);
    }
    #undef CONDITION

    #define CONDITION n > vec.size() && n < vec.size() + vec.available_front()
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        using size_type = typename sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::size_type;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);

        vec.pop_front();
        vec.pop_front();

        CHECK(vec.size() == 5);
        CHECK(*vec.nth(0) == 30);
        CHECK(*vec.nth(1) == 40);
        CHECK(*vec.nth(2) == 50);
        CHECK(*vec.nth(3) == 60);
        CHECK(*vec.nth(4) == 70);

        const size_type size = vec.size();

        const size_type available_front = vec.available_front();

        const size_type n = size + available_front - 1;

        CHECK(CONDITION);

        xint value(987654);

        vec.resize_front(n, value);

        CHECK(vec.size() == n);
        for (size_type i = 0; i < n - size; ++i)
        {
            CHECK(*vec.nth(i) == value);
        }
        CHECK(*vec.nth((n - size) + 0) == 30);
        CHECK(*vec.nth((n - size) + 1) == 40);
        CHECK(*vec.nth((n - size) + 2) == 50);
        CHECK(*vec.nth((n - size) + 3) == 60);
        CHECK(*vec.nth((n - size) + 4) == 70);
    }
    #undef CONDITION

    #define CONDITION n > vec.size() && n == vec.size() + vec.available_front()
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        using size_type = typename sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::size_type;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);

        vec.pop_front();
        vec.pop_front();

        CHECK(vec.size() == 5);
        CHECK(*vec.nth(0) == 30);
        CHECK(*vec.nth(1) == 40);
        CHECK(*vec.nth(2) == 50);
        CHECK(*vec.nth(3) == 60);
        CHECK(*vec.nth(4) == 70);

        const size_type size = vec.size();

        const size_type available_front = vec.available_front();

        const size_type n = size + available_front;

        CHECK(CONDITION);

        xint value(987654);

        vec.resize_front(n, value);

        CHECK(vec.size() == n);
        for (size_type i = 0; i < n - size; ++i)
        {
            CHECK(*vec.nth(i) == value);
        }
        CHECK(*vec.nth((n - size) + 0) == 30);
        CHECK(*vec.nth((n - size) + 1) == 40);
        CHECK(*vec.nth((n - size) + 2) == 50);
        CHECK(*vec.nth((n - size) + 3) == 60);
        CHECK(*vec.nth((n - size) + 4) == 70);
    }
    #undef CONDITION

    #define CONDITION n > vec.size() && n > vec.size() + vec.available_front()
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        using size_type = typename sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::size_type;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);

        vec.pop_front();
        vec.pop_front();

        CHECK(vec.size() == 5);
        CHECK(*vec.nth(0) == 30);
        CHECK(*vec.nth(1) == 40);
        CHECK(*vec.nth(2) == 50);
        CHECK(*vec.nth(3) == 60);
        CHECK(*vec.nth(4) == 70);

        const size_type size = vec.size();

        const size_type available_front = vec.available_front();

        const size_type n = size + available_front + 1;

        CHECK(CONDITION);

        xint value(987654);

        vec.resize_front(n, value);

        CHECK(vec.size() == n);
        for (size_type i = 0; i < n - size; ++i)
        {
            CHECK(*vec.nth(i) == value);
        }
        CHECK(*vec.nth((n - size) + 0) == 30);
        CHECK(*vec.nth((n - size) + 1) == 40);
        CHECK(*vec.nth((n - size) + 2) == 50);
        CHECK(*vec.nth((n - size) + 3) == 60);
        CHECK(*vec.nth((n - size) + 4) == 70);
    }
    #undef CONDITION
}

PRINT("Test resize_back(size_type)");
{
    #define CONDITION n < vec.size()
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        using size_type = typename sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::size_type;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);

        vec.pop_back();
        vec.pop_back();

        CHECK(vec.size() == 5);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);

        const size_type n = vec.size() - 1;

        CHECK(CONDITION);

        vec.resize_back(n);

        CHECK(vec.size() == 4);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
    }
    #undef CONDITION

    #define CONDITION n == vec.size()
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        using size_type = typename sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::size_type;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);

        vec.pop_back();
        vec.pop_back();

        CHECK(vec.size() == 5);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);

        const size_type n = vec.size();

        CHECK(CONDITION);

        vec.resize_back(n);

        CHECK(vec.size() == 5);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
    }
    #undef CONDITION

    #define CONDITION n > vec.size() && n < vec.size() + vec.available_back()
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        using size_type = typename sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::size_type;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);

        vec.pop_back();
        vec.pop_back();

        CHECK(vec.size() == 5);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);

        const size_type size = vec.size();

        const size_type available_back = vec.available_back();

        const size_type n = size + available_back - 1;

        CHECK(CONDITION);

        vec.resize_back(n);

        CHECK(vec.size() == n);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        for (size_type i = 5; i < n; ++i)
        {
            CHECK(*vec.nth(i) == SFL_TEST_XINT_DEFAULT_VALUE);
        }
    }
    #undef CONDITION

    #define CONDITION n > vec.size() && n == vec.size() + vec.available_back()
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        using size_type = typename sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::size_type;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);

        vec.pop_back();
        vec.pop_back();

        CHECK(vec.size() == 5);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);

        const size_type size = vec.size();

        const size_type available_back = vec.available_back();

        const size_type n = size + available_back;

        CHECK(CONDITION);

        vec.resize_back(n);

        CHECK(vec.size() == n);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        for (size_type i = 5; i < n; ++i)
        {
            CHECK(*vec.nth(i) == SFL_TEST_XINT_DEFAULT_VALUE);
        }
    }
    #undef CONDITION

    #define CONDITION n > vec.size() && n > vec.size() + vec.available_back()
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        using size_type = typename sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::size_type;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);

        vec.pop_back();
        vec.pop_back();

        CHECK(vec.size() == 5);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);

        const size_type size = vec.size();

        const size_type available_back = vec.available_back();

        const size_type n = size + available_back + 1;

        CHECK(CONDITION);

        vec.resize_back(n);

        CHECK(vec.size() == n);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        for (size_type i = 5; i < n; ++i)
        {
            CHECK(*vec.nth(i) == SFL_TEST_XINT_DEFAULT_VALUE);
        }
    }
    #undef CONDITION
}

PRINT("Test resize_back(size_type, const T&)");
{
    #define CONDITION n < vec.size()
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        using size_type = typename sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::size_type;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);

        vec.pop_back();
        vec.pop_back();

        CHECK(vec.size() == 5);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);

        const size_type n = vec.size() - 1;

        CHECK(CONDITION);

        xint value(987654);

        vec.resize_back(n, value);

        CHECK(vec.size() == 4);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
    }
    #undef CONDITION

    #define CONDITION n == vec.size()
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        using size_type = typename sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::size_type;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);

        vec.pop_back();
        vec.pop_back();

        CHECK(vec.size() == 5);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);

        const size_type n = vec.size();

        CHECK(CONDITION);

        xint value(987654);

        vec.resize_back(n, value);

        CHECK(vec.size() == 5);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
    }
    #undef CONDITION

    #define CONDITION n > vec.size() && n < vec.size() + vec.available_back()
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        using size_type = typename sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::size_type;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);

        vec.pop_back();
        vec.pop_back();

        CHECK(vec.size() == 5);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);

        const size_type size = vec.size();

        const size_type available_back = vec.available_back();

        const size_type n = size + available_back - 1;

        CHECK(CONDITION);

        xint value(987654);

        vec.resize_back(n, value);

        CHECK(vec.size() == n);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        for (size_type i = 5; i < n; ++i)
        {
            CHECK(*vec.nth(i) == value);
        }
    }
    #undef CONDITION

    #define CONDITION n > vec.size() && n == vec.size() + vec.available_back()
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        using size_type = typename sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::size_type;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);

        vec.pop_back();
        vec.pop_back();

        CHECK(vec.size() == 5);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);

        const size_type size = vec.size();

        const size_type available_back = vec.available_back();

        const size_type n = size + available_back;

        CHECK(CONDITION);

        xint value(987654);

        vec.resize_back(n, value);

        CHECK(vec.size() == n);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        for (size_type i = 5; i < n; ++i)
        {
            CHECK(*vec.nth(i) == value);
        }
    }
    #undef CONDITION

    #define CONDITION n > vec.size() && n > vec.size() + vec.available_back()
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        using size_type = typename sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::size_type;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);

        vec.pop_back();
        vec.pop_back();

        CHECK(vec.size() == 5);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);

        const size_type size = vec.size();

        const size_type available_back = vec.available_back();

        const size_type n = size + available_back + 1;

        CHECK(CONDITION);

        xint value(987654);

        vec.resize_back(n, value);

        CHECK(vec.size() == n);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);
        for (size_type i = 5; i < n; ++i)
        {
            CHECK(*vec.nth(i) == value);
        }
    }
    #undef CONDITION
}

PRINT("Test swap(container&)");
{
    sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec1, vec2;

    vec1.emplace(vec1.end(), 10);
    vec1.emplace(vec1.end(), 20);
    vec1.emplace(vec1.end(), 30);

    vec2.emplace(vec2.end(), 40);
    vec2.emplace(vec2.end(), 50);
    vec2.emplace(vec2.end(), 60);
    vec2.emplace(vec2.end(), 70);

    CHECK(vec1.size() == 3);
    CHECK(*vec1.nth(0) == 10);
    CHECK(*vec1.nth(1) == 20);
    CHECK(*vec1.nth(2) == 30);

    CHECK(vec2.size() == 4);
    CHECK(*vec2.nth(0) == 40);
    CHECK(*vec2.nth(1) == 50);
    CHECK(*vec2.nth(2) == 60);
    CHECK(*vec2.nth(3) == 70);

    ///////////////////////////////////////////////////////////////////////////

    vec1.swap(vec2);

    CHECK(vec1.size() == 4);
    CHECK(*vec1.nth(0) == 40);
    CHECK(*vec1.nth(1) == 50);
    CHECK(*vec1.nth(2) == 60);
    CHECK(*vec1.nth(3) == 70);

    CHECK(vec2.size() == 3);
    CHECK(*vec2.nth(0) == 10);
    CHECK(*vec2.nth(1) == 20);
    CHECK(*vec2.nth(2) == 30);

    ///////////////////////////////////////////////////////////////////////////

    vec1.swap(vec2);

    CHECK(vec1.size() == 3);
    CHECK(*vec1.nth(0) == 10);
    CHECK(*vec1.nth(1) == 20);
    CHECK(*vec1.nth(2) == 30);

    CHECK(vec2.size() == 4);
    CHECK(*vec2.nth(0) == 40);
    CHECK(*vec2.nth(1) == 50);
    CHECK(*vec2.nth(2) == 60);
    CHECK(*vec2.nth(3) == 70);
}

///////////////////////////////////////////////////////////////////////////////

PRINT("Test at(size_type)");
{
    sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

    vec.emplace_back(10);
    vec.emplace_back(20);
    vec.emplace_back(30);

    CHECK(vec.at(0) == 10);
    CHECK(vec.at(1) == 20);
    CHECK(vec.at(2) == 30);

    CHECK(vec.size() == 3);
    CHECK(*vec.nth(0) == 10);
    CHECK(*vec.nth(1) == 20);
    CHECK(*vec.nth(2) == 30);

    vec.at(0) = 40;
    vec.at(1) = 50;
    vec.at(2) = 60;

    CHECK(vec.size() == 3);
    CHECK(*vec.nth(0) == 40);
    CHECK(*vec.nth(1) == 50);
    CHECK(*vec.nth(2) == 60);

    #if !defined(SFL_NO_EXCEPTIONS)
    bool caught_exception = false;

    try
    {
        vec.at(3) = 1;
    }
    catch (...)
    {
        caught_exception = true;
    }

    CHECK(caught_exception == true);
    #endif

    CHECK(vec.size() == 3);
    CHECK(*vec.nth(0) == 40);
    CHECK(*vec.nth(1) == 50);
    CHECK(*vec.nth(2) == 60);
}

PRINT("Test operator[](size_type)");
{
    sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

    vec.emplace_back(10);
    vec.emplace_back(20);
    vec.emplace_back(30);

    CHECK(vec[0] == 10);
    CHECK(vec[1] == 20);
    CHECK(vec[2] == 30);

    CHECK(vec.size() == 3);
    CHECK(*vec.nth(0) == 10);
    CHECK(*vec.nth(1) == 20);
    CHECK(*vec.nth(2) == 30);

    vec[0] = 40;
    vec[1] = 50;
    vec[2] = 60;

    CHECK(vec.size() == 3);
    CHECK(*vec.nth(0) == 40);
    CHECK(*vec.nth(1) == 50);
    CHECK(*vec.nth(2) == 60);
}

PRINT("Test front() and back()");
{
    sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

    vec.emplace_back(10);
    vec.emplace_back(20);
    vec.emplace_back(30);

    CHECK(vec.front() == 10);
    CHECK(vec.back()  == 30);

    CHECK(vec.size() == 3);
    CHECK(*vec.nth(0) == 10);
    CHECK(*vec.nth(1) == 20);
    CHECK(*vec.nth(2) == 30);

    vec.front() = 40;
    vec.back()  = 60;

    CHECK(vec.size() == 3);
    CHECK(*vec.nth(0) == 40);
    CHECK(*vec.nth(1) == 20);
    CHECK(*vec.nth(2) == 60);
}

///////////////////////////////////////////////////////////////////////////////

PRINT("Test reserve_front(size_type) and shrink_to_fit()");
{
    sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

    vec.emplace_back(8);
    vec.emplace_back(9);
    vec.emplace_back(10);
    vec.emplace_back(20);
    vec.emplace_back(30);

    vec.pop_front();
    vec.pop_front();

    CHECK(vec.size() == 3);
    CHECK(*vec.nth(0) == 10);
    CHECK(*vec.nth(1) == 20);
    CHECK(*vec.nth(2) == 30);

    {
        const auto available_front_before = vec.available_front();
        vec.shrink_to_fit();
        const auto available_front_after = vec.available_front();

        CHECK(available_front_after <= available_front_before);

        CHECK(vec.size() == 3);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
    }

    {
        const auto available_front_before = vec.available_front();
        vec.reserve_front(9999);
        const auto available_front_after = vec.available_front();

        CHECK(available_front_after > available_front_before);

        CHECK(vec.size() == 3);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
    }

    {
        const auto available_front_before = vec.available_front();
        vec.shrink_to_fit();
        const auto available_front_after = vec.available_front();

        CHECK(available_front_after < available_front_before);

        CHECK(vec.size() == 3);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
    }
}

PRINT("Test reserve_back(size_type) and shrink_to_fit()");
{
    sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

    vec.emplace_back(10);
    vec.emplace_back(20);
    vec.emplace_back(30);
    vec.emplace_back(40);
    vec.emplace_back(50);

    vec.pop_back();
    vec.pop_back();

    CHECK(vec.size() == 3);
    CHECK(*vec.nth(0) == 10);
    CHECK(*vec.nth(1) == 20);
    CHECK(*vec.nth(2) == 30);

    {
        const auto available_back_before = vec.available_back();
        vec.shrink_to_fit();
        const auto available_back_after = vec.available_back();

        CHECK(available_back_after <= available_back_before);

        CHECK(vec.size() == 3);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
    }

    {
        const auto available_back_before = vec.available_back();
        vec.reserve_back(9999);
        const auto available_back_after = vec.available_back();

        CHECK(available_back_after > available_back_before);

        CHECK(vec.size() == 3);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
    }

    {
        const auto available_back_before = vec.available_back();
        vec.shrink_to_fit();
        const auto available_back_after = vec.available_back();

        CHECK(available_back_after < available_back_before);

        CHECK(vec.size() == 3);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
    }
}

///////////////////////////////////////////////////////////////////////////////

PRINT("Test container()");
{
    sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

    CHECK(vec.size() == 0);
}

PRINT("Test container(const Allocator&)");
{
    TPARAM_ALLOCATOR<xint> alloc;

    sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec(alloc);

    CHECK(vec.size() == 0);
}

PRINT("Test container(size_type)");
{
    sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec(4);

    CHECK(vec.size() == 4);
    CHECK(*vec.nth(0) == SFL_TEST_XINT_DEFAULT_VALUE);
    CHECK(*vec.nth(1) == SFL_TEST_XINT_DEFAULT_VALUE);
    CHECK(*vec.nth(2) == SFL_TEST_XINT_DEFAULT_VALUE);
    CHECK(*vec.nth(3) == SFL_TEST_XINT_DEFAULT_VALUE);
}

PRINT("Test container(size_type, const Allocator&)");
{
    TPARAM_ALLOCATOR<xint> alloc;

    sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec(4, alloc);

    CHECK(vec.size() == 4);
    CHECK(*vec.nth(0) == SFL_TEST_XINT_DEFAULT_VALUE);
    CHECK(*vec.nth(1) == SFL_TEST_XINT_DEFAULT_VALUE);
    CHECK(*vec.nth(2) == SFL_TEST_XINT_DEFAULT_VALUE);
    CHECK(*vec.nth(3) == SFL_TEST_XINT_DEFAULT_VALUE);
}

PRINT("Test container(size_type, const T&)");
{
    xint value_99(99);

    sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec(4, value_99);

    CHECK(vec.size() == 4);
    CHECK(*vec.nth(0) == value_99);
    CHECK(*vec.nth(1) == value_99);
    CHECK(*vec.nth(2) == value_99);
    CHECK(*vec.nth(3) == value_99);
}

PRINT("Test container(size_type, const T&, const Allocator&)");
{
    TPARAM_ALLOCATOR<xint> alloc;

    xint value_99(99);

    sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec(4, value_99, alloc);

    CHECK(vec.size() == 4);
    CHECK(*vec.nth(0) == value_99);
    CHECK(*vec.nth(1) == value_99);
    CHECK(*vec.nth(2) == value_99);
    CHECK(*vec.nth(3) == value_99);
}

PRINT("Test container(InputIt, InputIt)");
{
    // Input iterator (exactly)
    {
        std::istringstream iss("10 20 30 40");

        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec
        (
            (std::istream_iterator<int>(iss)),
            (std::istream_iterator<int>())
        );

        CHECK(vec.size() == 4);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
    }

    // Forward iterator
    {
        std::vector<xint> data({10, 20, 30, 40});

        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec(data.begin(), data.end());

        CHECK(vec.size() == 4);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
    }
}

PRINT("Test container(InputIt, InputIt, const Allocator&)");
{
    // Input iterator (exactly)
    {
        std::istringstream iss("10 20 30 40");

        TPARAM_ALLOCATOR<xint> alloc;

        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec
        (
            (std::istream_iterator<int>(iss)),
            (std::istream_iterator<int>()),
            alloc
        );

        CHECK(vec.size() == 4);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
    }

    // Forward iterator
    {
        std::vector<xint> data({10, 20, 30, 40});

        TPARAM_ALLOCATOR<xint> alloc;

        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec(data.begin(), data.end(), alloc);

        CHECK(vec.size() == 4);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
    }
}

PRINT("Test container(std::initializer_list)");
{
    std::initializer_list<xint> ilist{10, 20, 30, 40};

    sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec(ilist);

    CHECK(vec.size() == 4);
    CHECK(*vec.nth(0) == 10);
    CHECK(*vec.nth(1) == 20);
    CHECK(*vec.nth(2) == 30);
    CHECK(*vec.nth(3) == 40);
}

PRINT("Test container(std::initializer_list, const Allocator&)");
{
    std::initializer_list<xint> ilist{10, 20, 30, 40};

    TPARAM_ALLOCATOR<xint> alloc;

    sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec(ilist, alloc);

    CHECK(vec.size() == 4);
    CHECK(*vec.nth(0) == 10);
    CHECK(*vec.nth(1) == 20);
    CHECK(*vec.nth(2) == 30);
    CHECK(*vec.nth(3) == 40);
}

PRINT("Test container(const container&)");
{
    sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec1;

    vec1.emplace_back(10);
    vec1.emplace_back(20);
    vec1.emplace_back(30);

    CHECK(vec1.size() == 3);
    CHECK(*vec1.nth(0) == 10);
    CHECK(*vec1.nth(1) == 20);
    CHECK(*vec1.nth(2) == 30);

    ///////////////////////////////////////////////////////////////////////////

    sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec2(vec1);

    CHECK(vec2.size() == 3);
    CHECK(*vec2.nth(0) == 10);
    CHECK(*vec2.nth(1) == 20);
    CHECK(*vec2.nth(2) == 30);
}

PRINT("Test container(const container&, const Allocator&)");
{
    sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec1;

    vec1.emplace_back(10);
    vec1.emplace_back(20);
    vec1.emplace_back(30);

    CHECK(vec1.size() == 3);
    CHECK(*vec1.nth(0) == 10);
    CHECK(*vec1.nth(1) == 20);
    CHECK(*vec1.nth(2) == 30);

    ///////////////////////////////////////////////////////////////////////////

    TPARAM_ALLOCATOR<xint> alloc;

    sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec2(vec1, alloc);

    CHECK(vec2.size() == 3);
    CHECK(*vec2.nth(0) == 10);
    CHECK(*vec2.nth(1) == 20);
    CHECK(*vec2.nth(2) == 30);
}

PRINT("Test container(container&&)");
{
    sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec1;

    vec1.emplace_back(10);
    vec1.emplace_back(20);
    vec1.emplace_back(30);

    CHECK(vec1.size() == 3);
    CHECK(*vec1.nth(0) == 10);
    CHECK(*vec1.nth(1) == 20);
    CHECK(*vec1.nth(2) == 30);

    ///////////////////////////////////////////////////////////////////////////

    sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec2(std::move(vec1));

    CHECK(vec2.size() == 3);
    CHECK(*vec2.nth(0) == 10);
    CHECK(*vec2.nth(1) == 20);
    CHECK(*vec2.nth(2) == 30);

    ///////////////////////////////////////////////////////////////////////////

    if (vec1.size() == 3)
    {
        // Elements were moved one-by-one
        CHECK(*vec1.nth(0) == -10);
        CHECK(*vec1.nth(1) == -20);
        CHECK(*vec1.nth(2) == -30);
    }
    else
    {
        // Storage was moved
        CHECK(vec1.size() == 0);
    }
}

PRINT("Test container(container&&, const Allocator&)");
{
    sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec1;

    vec1.emplace_back(10);
    vec1.emplace_back(20);
    vec1.emplace_back(30);

    CHECK(vec1.size() == 3);
    CHECK(*vec1.nth(0) == 10);
    CHECK(*vec1.nth(1) == 20);
    CHECK(*vec1.nth(2) == 30);

    ///////////////////////////////////////////////////////////////////////////

    TPARAM_ALLOCATOR<xint> alloc;

    sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec2(std::move(vec1), alloc);

    CHECK(vec2.size() == 3);
    CHECK(*vec2.nth(0) == 10);
    CHECK(*vec2.nth(1) == 20);
    CHECK(*vec2.nth(2) == 30);

    ///////////////////////////////////////////////////////////////////////////

    if (vec1.size() == 3)
    {
        // Elements were moved one-by-one
        CHECK(*vec1.nth(0) == -10);
        CHECK(*vec1.nth(1) == -20);
        CHECK(*vec1.nth(2) == -30);
    }
    else
    {
        // Storage was moved
        CHECK(vec1.size() == 0);
    }
}

PRINT("Test container(sfl::from_range_t, Range&&)");
{
    // Input iterator (exactly)
    {
        std::istringstream iss("10 20 30 40");

        #if SFL_CPP_VERSION >= SFL_CPP_20
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec(sfl::from_range_t(), (std::views::istream<int>(iss)));
        #else
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec(sfl::from_range_t(), (sfl::test::istream_view<int>(iss)));
        #endif

        CHECK(vec.size() == 4);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
    }

    // Forward iterator
    {
        std::vector<int> data({10, 20, 30, 40});

        #if SFL_CPP_VERSION >= SFL_CPP_20
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec(sfl::from_range_t(), std::views::all(data));
        #else
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec(sfl::from_range_t(), data);
        #endif

        CHECK(vec.size() == 4);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
    }
}

PRINT("Test container(sfl::from_range_t, Range&&, const Allocator&)");
{
    // Input iterator (exactly)
    {
        std::istringstream iss("10 20 30 40");

        TPARAM_ALLOCATOR<xint> alloc;

        #if SFL_CPP_VERSION >= SFL_CPP_20
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec(sfl::from_range_t(), std::views::istream<int>(iss), alloc);
        #else
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec(sfl::from_range_t(), sfl::test::istream_view<int>(iss), alloc);
        #endif

        CHECK(vec.size() == 4);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
    }

    // Forward iterator
    {
        std::vector<int> data({10, 20, 30, 40});

        TPARAM_ALLOCATOR<xint> alloc;

        #if SFL_CPP_VERSION >= SFL_CPP_20
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec(sfl::from_range_t(), std::views::all(data), alloc);
        #else
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec(sfl::from_range_t(), data, alloc);
        #endif

        CHECK(vec.size() == 4);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
    }
}

///////////////////////////////////////////////////////////////////////////////

PRINT("Test operator=(const container&)");
{
    #define CONDITION vec1.size() == vec2.size()
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec1, vec2;

        vec1.emplace(vec1.end(), 10);
        vec1.emplace(vec1.end(), 20);
        vec1.emplace(vec1.end(), 30);

        CHECK(vec1.size() == 3);
        CHECK(*vec1.nth(0) == 10);
        CHECK(*vec1.nth(1) == 20);
        CHECK(*vec1.nth(2) == 30);

        vec2.emplace(vec2.end(), 40);
        vec2.emplace(vec2.end(), 50);
        vec2.emplace(vec2.end(), 60);

        CHECK(vec2.size() == 3);
        CHECK(*vec2.nth(0) == 40);
        CHECK(*vec2.nth(1) == 50);
        CHECK(*vec2.nth(2) == 60);

        ///////////////////////////////////////////////////////////////////////

        CHECK(CONDITION);

        vec1 = vec2;

        CHECK(vec1.size() == 3);
        CHECK(*vec1.nth(0) == 40);
        CHECK(*vec1.nth(1) == 50);
        CHECK(*vec1.nth(2) == 60);

        CHECK(vec2.size() == 3);
        CHECK(*vec2.nth(0) == 40);
        CHECK(*vec2.nth(1) == 50);
        CHECK(*vec2.nth(2) == 60);
    }
    #undef CONDITION

    #define CONDITION vec1.size() < vec2.size()
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec1, vec2;

        vec1.emplace(vec1.end(), 10);
        vec1.emplace(vec1.end(), 20);
        vec1.emplace(vec1.end(), 30);

        CHECK(vec1.size() == 3);
        CHECK(*vec1.nth(0) == 10);
        CHECK(*vec1.nth(1) == 20);
        CHECK(*vec1.nth(2) == 30);

        vec2.emplace(vec2.end(), 40);
        vec2.emplace(vec2.end(), 50);
        vec2.emplace(vec2.end(), 60);
        vec2.emplace(vec2.end(), 70);
        vec2.emplace(vec2.end(), 80);

        CHECK(vec2.size() == 5);
        CHECK(*vec2.nth(0) == 40);
        CHECK(*vec2.nth(1) == 50);
        CHECK(*vec2.nth(2) == 60);
        CHECK(*vec2.nth(3) == 70);
        CHECK(*vec2.nth(4) == 80);

        ///////////////////////////////////////////////////////////////////////

        CHECK(CONDITION);

        vec1 = vec2;

        CHECK(vec1.size() == 5);
        CHECK(*vec1.nth(0) == 40);
        CHECK(*vec1.nth(1) == 50);
        CHECK(*vec1.nth(2) == 60);
        CHECK(*vec1.nth(3) == 70);
        CHECK(*vec1.nth(4) == 80);

        CHECK(vec2.size() == 5);
        CHECK(*vec2.nth(0) == 40);
        CHECK(*vec2.nth(1) == 50);
        CHECK(*vec2.nth(2) == 60);
        CHECK(*vec2.nth(3) == 70);
        CHECK(*vec2.nth(4) == 80);
    }
    #undef CONDITION

    #define CONDITION vec1.size() > vec2.size()
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec1, vec2;

        vec1.emplace(vec1.end(), 10);
        vec1.emplace(vec1.end(), 20);
        vec1.emplace(vec1.end(), 30);
        vec1.emplace(vec1.end(), 40);
        vec1.emplace(vec1.end(), 50);

        CHECK(vec1.size() == 5);
        CHECK(*vec1.nth(0) == 10);
        CHECK(*vec1.nth(1) == 20);
        CHECK(*vec1.nth(2) == 30);
        CHECK(*vec1.nth(3) == 40);
        CHECK(*vec1.nth(4) == 50);

        vec2.emplace(vec2.end(), 60);
        vec2.emplace(vec2.end(), 70);
        vec2.emplace(vec2.end(), 80);

        CHECK(vec2.size() == 3);
        CHECK(*vec2.nth(0) == 60);
        CHECK(*vec2.nth(1) == 70);
        CHECK(*vec2.nth(2) == 80);

        ///////////////////////////////////////////////////////////////////////

        CHECK(CONDITION);

        vec1 = vec2;

        CHECK(vec1.size() == 3);
        CHECK(*vec1.nth(0) == 60);
        CHECK(*vec1.nth(1) == 70);
        CHECK(*vec1.nth(2) == 80);

        CHECK(vec2.size() == 3);
        CHECK(*vec2.nth(0) == 60);
        CHECK(*vec2.nth(1) == 70);
        CHECK(*vec2.nth(2) == 80);
    }
    #undef CONDITION
}

PRINT("Test operator=(container&&)");
{
    #define CONDITION vec1.size() == vec2.size()
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec1, vec2;

        vec1.emplace(vec1.end(), 10);
        vec1.emplace(vec1.end(), 20);
        vec1.emplace(vec1.end(), 30);

        CHECK(vec1.size() == 3);
        CHECK(*vec1.nth(0) == 10);
        CHECK(*vec1.nth(1) == 20);
        CHECK(*vec1.nth(2) == 30);

        vec2.emplace(vec2.end(), 40);
        vec2.emplace(vec2.end(), 50);
        vec2.emplace(vec2.end(), 60);

        CHECK(vec2.size() == 3);
        CHECK(*vec2.nth(0) == 40);
        CHECK(*vec2.nth(1) == 50);
        CHECK(*vec2.nth(2) == 60);

        ///////////////////////////////////////////////////////////////////////

        CHECK(CONDITION);

        vec1 = std::move(vec2);

        CHECK(vec1.size() == 3);
        CHECK(*vec1.nth(0) == 40);
        CHECK(*vec1.nth(1) == 50);
        CHECK(*vec1.nth(2) == 60);

        if (vec2.size() == 3)
        {
            // Elements were moved one-by-one
            CHECK(*vec2.nth(0) == -40);
            CHECK(*vec2.nth(1) == -50);
            CHECK(*vec2.nth(2) == -60);
        }
        else
        {
            // Storage was moved
            CHECK(vec2.size() == 0);
        }
    }
    #undef CONDITION

    #define CONDITION vec1.size() < vec2.size()
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec1, vec2;

        vec1.emplace(vec1.end(), 10);
        vec1.emplace(vec1.end(), 20);
        vec1.emplace(vec1.end(), 30);

        CHECK(vec1.size() == 3);
        CHECK(*vec1.nth(0) == 10);
        CHECK(*vec1.nth(1) == 20);
        CHECK(*vec1.nth(2) == 30);

        vec2.emplace(vec2.end(), 40);
        vec2.emplace(vec2.end(), 50);
        vec2.emplace(vec2.end(), 60);
        vec2.emplace(vec2.end(), 70);
        vec2.emplace(vec2.end(), 80);

        CHECK(vec2.size() == 5);
        CHECK(*vec2.nth(0) == 40);
        CHECK(*vec2.nth(1) == 50);
        CHECK(*vec2.nth(2) == 60);
        CHECK(*vec2.nth(3) == 70);
        CHECK(*vec2.nth(4) == 80);

        ///////////////////////////////////////////////////////////////////////

        CHECK(CONDITION);

        vec1 = std::move(vec2);

        CHECK(vec1.size() == 5);
        CHECK(*vec1.nth(0) == 40);
        CHECK(*vec1.nth(1) == 50);
        CHECK(*vec1.nth(2) == 60);
        CHECK(*vec1.nth(3) == 70);
        CHECK(*vec1.nth(4) == 80);

        if (vec2.size() == 5)
        {
            // Elements were moved one-by-one
            CHECK(*vec2.nth(0) == -40);
            CHECK(*vec2.nth(1) == -50);
            CHECK(*vec2.nth(2) == -60);
            CHECK(*vec2.nth(3) == -70);
            CHECK(*vec2.nth(4) == -80);
        }
        else
        {
            // Storage was moved
            CHECK(vec2.size() == 0);
        }
    }
    #undef CONDITION

    #define CONDITION vec1.size() > vec2.size()
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec1, vec2;

        vec1.emplace(vec1.end(), 10);
        vec1.emplace(vec1.end(), 20);
        vec1.emplace(vec1.end(), 30);
        vec1.emplace(vec1.end(), 40);
        vec1.emplace(vec1.end(), 50);

        CHECK(vec1.size() == 5);
        CHECK(*vec1.nth(0) == 10);
        CHECK(*vec1.nth(1) == 20);
        CHECK(*vec1.nth(2) == 30);
        CHECK(*vec1.nth(3) == 40);
        CHECK(*vec1.nth(4) == 50);

        vec2.emplace(vec2.end(), 60);
        vec2.emplace(vec2.end(), 70);
        vec2.emplace(vec2.end(), 80);

        CHECK(vec2.size() == 3);
        CHECK(*vec2.nth(0) == 60);
        CHECK(*vec2.nth(1) == 70);
        CHECK(*vec2.nth(2) == 80);

        ///////////////////////////////////////////////////////////////////////

        CHECK(CONDITION);

        vec1 = std::move(vec2);

        CHECK(vec1.size() == 3);
        CHECK(*vec1.nth(0) == 60);
        CHECK(*vec1.nth(1) == 70);
        CHECK(*vec1.nth(2) == 80);

        if (vec2.size() == 3)
        {
            // Elements were moved one-by-one
            CHECK(*vec2.nth(0) == -60);
            CHECK(*vec2.nth(1) == -70);
            CHECK(*vec2.nth(2) == -80);
        }
        else
        {
            // Storage was moved
            CHECK(vec2.size() == 0);
        }
    }
    #undef CONDITION
}

PRINT("Test operator=(std::initializer_list)");
{
    #define CONDITION vec.size() == ilist.size()
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace(vec.end(), 10);
        vec.emplace(vec.end(), 20);
        vec.emplace(vec.end(), 30);

        CHECK(vec.size() == 3);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);

        std::initializer_list<xint> ilist{40, 50, 60};

        ///////////////////////////////////////////////////////////////////////

        CHECK(CONDITION);

        vec = ilist;

        CHECK(vec.size() == 3);
        CHECK(*vec.nth(0) == 40);
        CHECK(*vec.nth(1) == 50);
        CHECK(*vec.nth(2) == 60);
    }
    #undef CONDITION

    #define CONDITION vec.size() < ilist.size()
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace(vec.end(), 10);
        vec.emplace(vec.end(), 20);
        vec.emplace(vec.end(), 30);

        CHECK(vec.size() == 3);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);

        std::initializer_list<xint> ilist{40, 50, 60, 70, 80};

        ///////////////////////////////////////////////////////////////////////

        CHECK(CONDITION);

        vec = ilist;

        CHECK(vec.size() == 5);
        CHECK(*vec.nth(0) == 40);
        CHECK(*vec.nth(1) == 50);
        CHECK(*vec.nth(2) == 60);
        CHECK(*vec.nth(3) == 70);
        CHECK(*vec.nth(4) == 80);
    }
    #undef CONDITION

    #define CONDITION vec.size() > ilist.size()
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        vec.emplace(vec.end(), 10);
        vec.emplace(vec.end(), 20);
        vec.emplace(vec.end(), 30);
        vec.emplace(vec.end(), 40);
        vec.emplace(vec.end(), 50);

        CHECK(vec.size() == 5);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
        CHECK(*vec.nth(4) == 50);

        std::initializer_list<xint> ilist{60, 70, 80};

        ///////////////////////////////////////////////////////////////////////

        CHECK(CONDITION);

        vec = ilist;

        CHECK(vec.size() == 3);
        CHECK(*vec.nth(0) == 60);
        CHECK(*vec.nth(1) == 70);
        CHECK(*vec.nth(2) == 80);
    }
    #undef CONDITION
}

///////////////////////////////////////////////////////////////////////////////

PRINT("Test assign(size_type, const T&)");
{
    #define CONDITION n < vec.size()
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        using size_type = typename sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::size_type;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        vec.pop_front();
        vec.pop_front();

        vec.pop_back();
        vec.pop_back();

        CHECK(vec.size() == 5);
        CHECK(*vec.nth(0) == 30);
        CHECK(*vec.nth(1) == 40);
        CHECK(*vec.nth(2) == 50);
        CHECK(*vec.nth(3) == 60);
        CHECK(*vec.nth(4) == 70);

        const size_type n = vec.size() - 1;

        CHECK(CONDITION);

        xint value(99);

        vec.assign(n, value);

        CHECK(vec.size() == n);
        for (const auto& elem : vec)
        {
            CHECK(elem == value);
        }
    }
    #undef CONDITION

    #define CONDITION n == vec.size()
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        using size_type = typename sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::size_type;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        vec.pop_front();
        vec.pop_front();

        vec.pop_back();
        vec.pop_back();

        CHECK(vec.size() == 5);
        CHECK(*vec.nth(0) == 30);
        CHECK(*vec.nth(1) == 40);
        CHECK(*vec.nth(2) == 50);
        CHECK(*vec.nth(3) == 60);
        CHECK(*vec.nth(4) == 70);

        const size_type n = vec.size();

        CHECK(CONDITION);

        xint value(99);

        vec.assign(n, value);

        CHECK(vec.size() == n);
        for (const auto& elem : vec)
        {
            CHECK(elem == value);
        }
    }
    #undef CONDITION

    #define CONDITION n > vec.size() && \
                      n < vec.size() + vec.available_back()
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        using size_type = typename sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::size_type;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        vec.pop_front();
        vec.pop_front();

        vec.pop_back();
        vec.pop_back();

        CHECK(vec.size() == 5);
        CHECK(*vec.nth(0) == 30);
        CHECK(*vec.nth(1) == 40);
        CHECK(*vec.nth(2) == 50);
        CHECK(*vec.nth(3) == 60);
        CHECK(*vec.nth(4) == 70);

        const size_type n = vec.size() + vec.available_back() - 1;

        CHECK(CONDITION);

        xint value(99);

        vec.assign(n, value);

        CHECK(vec.size() == n);
        for (const auto& elem : vec)
        {
            CHECK(elem == value);
        }
    }
    #undef CONDITION

    #define CONDITION n > vec.size() && \
                      n == vec.size() + vec.available_back()
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        using size_type = typename sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::size_type;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        vec.pop_front();
        vec.pop_front();

        vec.pop_back();
        vec.pop_back();

        CHECK(vec.size() == 5);
        CHECK(*vec.nth(0) == 30);
        CHECK(*vec.nth(1) == 40);
        CHECK(*vec.nth(2) == 50);
        CHECK(*vec.nth(3) == 60);
        CHECK(*vec.nth(4) == 70);

        const size_type n = vec.size() + vec.available_back();

        CHECK(CONDITION);

        xint value(99);

        vec.assign(n, value);

        CHECK(vec.size() == n);
        for (const auto& elem : vec)
        {
            CHECK(elem == value);
        }
    }
    #undef CONDITION

    #define CONDITION n > vec.size() && \
                      n > vec.size() + vec.available_back() && \
                      n < vec.available_front() + vec.size() + vec.available_back()
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        using size_type = typename sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::size_type;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        vec.pop_front();
        vec.pop_front();

        vec.pop_back();
        vec.pop_back();

        CHECK(vec.size() == 5);
        CHECK(*vec.nth(0) == 30);
        CHECK(*vec.nth(1) == 40);
        CHECK(*vec.nth(2) == 50);
        CHECK(*vec.nth(3) == 60);
        CHECK(*vec.nth(4) == 70);

        const size_type n = vec.available_front() + vec.size() + vec.available_back() - 1;

        CHECK(CONDITION);

        xint value(99);

        vec.assign(n, value);

        CHECK(vec.size() == n);
        for (const auto& elem : vec)
        {
            CHECK(elem == value);
        }
    }
    #undef CONDITION

    #define CONDITION n > vec.size() && \
                      n > vec.size() + vec.available_back() && \
                      n == vec.available_front() + vec.size() + vec.available_back()
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        using size_type = typename sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::size_type;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        vec.pop_front();
        vec.pop_front();

        vec.pop_back();
        vec.pop_back();

        CHECK(vec.size() == 5);
        CHECK(*vec.nth(0) == 30);
        CHECK(*vec.nth(1) == 40);
        CHECK(*vec.nth(2) == 50);
        CHECK(*vec.nth(3) == 60);
        CHECK(*vec.nth(4) == 70);

        const size_type n = vec.available_front() + vec.size() + vec.available_back();

        CHECK(CONDITION);

        xint value(99);

        vec.assign(n, value);

        CHECK(vec.size() == n);
        for (const auto& elem : vec)
        {
            CHECK(elem == value);
        }
    }
    #undef CONDITION

    #define CONDITION n > vec.size() && \
                      n > vec.size() + vec.available_back() && \
                      n > vec.available_front() + vec.size() + vec.available_back()
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        using size_type = typename sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::size_type;

        vec.emplace_back(10);
        vec.emplace_back(20);
        vec.emplace_back(30);
        vec.emplace_back(40);
        vec.emplace_back(50);
        vec.emplace_back(60);
        vec.emplace_back(70);
        vec.emplace_back(80);
        vec.emplace_back(90);

        vec.pop_front();
        vec.pop_front();

        vec.pop_back();
        vec.pop_back();

        CHECK(vec.size() == 5);
        CHECK(*vec.nth(0) == 30);
        CHECK(*vec.nth(1) == 40);
        CHECK(*vec.nth(2) == 50);
        CHECK(*vec.nth(3) == 60);
        CHECK(*vec.nth(4) == 70);

        const size_type n = vec.available_front() + vec.size() + vec.available_back() + 1;

        CHECK(CONDITION);

        xint value(99);

        vec.assign(n, value);

        CHECK(vec.size() == n);
        for (const auto& elem : vec)
        {
            CHECK(elem == value);
        }
    }
    #undef CONDITION
}

PRINT("Test assign(InputIt, InputIt)");
{
    // Input iterator (exactly)
    {
        #define CONDITION n < vec.size()
        {
            sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

            using size_type = typename sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::size_type;

            vec.emplace_back(10);
            vec.emplace_back(20);
            vec.emplace_back(30);
            vec.emplace_back(40);
            vec.emplace_back(50);
            vec.emplace_back(60);
            vec.emplace_back(70);
            vec.emplace_back(80);
            vec.emplace_back(90);

            vec.pop_front();
            vec.pop_front();

            vec.pop_back();
            vec.pop_back();

            CHECK(vec.size() == 5);
            CHECK(*vec.nth(0) == 30);
            CHECK(*vec.nth(1) == 40);
            CHECK(*vec.nth(2) == 50);
            CHECK(*vec.nth(3) == 60);
            CHECK(*vec.nth(4) == 70);

            const size_type n = vec.size() - 1;

            CHECK(CONDITION);

            std::stringstream ss;
            for (int i = 0; i < int(n); ++i)
            {
                ss << i << " ";
            }

            vec.assign
            (
                (std::istream_iterator<int>(ss)),
                (std::istream_iterator<int>())
            );

            CHECK(vec.size() == n);
            for (int i = 0; i < int(n); ++i)
            {
                CHECK(*vec.nth(i) == i);
            }
        }
        #undef CONDITION

        #define CONDITION n == vec.size()
        {
            sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

            using size_type = typename sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::size_type;

            vec.emplace_back(10);
            vec.emplace_back(20);
            vec.emplace_back(30);
            vec.emplace_back(40);
            vec.emplace_back(50);
            vec.emplace_back(60);
            vec.emplace_back(70);
            vec.emplace_back(80);
            vec.emplace_back(90);

            vec.pop_front();
            vec.pop_front();

            vec.pop_back();
            vec.pop_back();

            CHECK(vec.size() == 5);
            CHECK(*vec.nth(0) == 30);
            CHECK(*vec.nth(1) == 40);
            CHECK(*vec.nth(2) == 50);
            CHECK(*vec.nth(3) == 60);
            CHECK(*vec.nth(4) == 70);

            const size_type n = vec.size();

            CHECK(CONDITION);

            std::stringstream ss;
            for (int i = 0; i < int(n); ++i)
            {
                ss << i << " ";
            }

            vec.assign
            (
                (std::istream_iterator<int>(ss)),
                (std::istream_iterator<int>())
            );

            CHECK(vec.size() == n);
            for (int i = 0; i < int(n); ++i)
            {
                CHECK(*vec.nth(i) == i);
            }
        }
        #undef CONDITION

        #define CONDITION n > vec.size() && \
                          n < vec.size() + vec.available_back()
        {
            sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

            using size_type = typename sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::size_type;

            vec.emplace_back(10);
            vec.emplace_back(20);
            vec.emplace_back(30);
            vec.emplace_back(40);
            vec.emplace_back(50);
            vec.emplace_back(60);
            vec.emplace_back(70);
            vec.emplace_back(80);
            vec.emplace_back(90);

            vec.pop_front();
            vec.pop_front();

            vec.pop_back();
            vec.pop_back();

            CHECK(vec.size() == 5);
            CHECK(*vec.nth(0) == 30);
            CHECK(*vec.nth(1) == 40);
            CHECK(*vec.nth(2) == 50);
            CHECK(*vec.nth(3) == 60);
            CHECK(*vec.nth(4) == 70);

            const size_type n = vec.size() + vec.available_back() - 1;

            CHECK(CONDITION);

            std::stringstream ss;
            for (int i = 0; i < int(n); ++i)
            {
                ss << i << " ";
            }

            vec.assign
            (
                (std::istream_iterator<int>(ss)),
                (std::istream_iterator<int>())
            );

            CHECK(vec.size() == n);
            for (int i = 0; i < int(n); ++i)
            {
                CHECK(*vec.nth(i) == i);
            }
        }
        #undef CONDITION

        #define CONDITION n > vec.size() && \
                          n == vec.size() + vec.available_back()
        {
            sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

            using size_type = typename sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::size_type;

            vec.emplace_back(10);
            vec.emplace_back(20);
            vec.emplace_back(30);
            vec.emplace_back(40);
            vec.emplace_back(50);
            vec.emplace_back(60);
            vec.emplace_back(70);
            vec.emplace_back(80);
            vec.emplace_back(90);

            vec.pop_front();
            vec.pop_front();

            vec.pop_back();
            vec.pop_back();

            CHECK(vec.size() == 5);
            CHECK(*vec.nth(0) == 30);
            CHECK(*vec.nth(1) == 40);
            CHECK(*vec.nth(2) == 50);
            CHECK(*vec.nth(3) == 60);
            CHECK(*vec.nth(4) == 70);

            const size_type n = vec.size() + vec.available_back();

            CHECK(CONDITION);

            std::stringstream ss;
            for (int i = 0; i < int(n); ++i)
            {
                ss << i << " ";
            }

            vec.assign
            (
                (std::istream_iterator<int>(ss)),
                (std::istream_iterator<int>())
            );

            CHECK(vec.size() == n);
            for (int i = 0; i < int(n); ++i)
            {
                CHECK(*vec.nth(i) == i);
            }
        }
        #undef CONDITION

        #define CONDITION n > vec.size() && \
                          n > vec.size() + vec.available_back() && \
                          n < vec.available_front() + vec.size() + vec.available_back()
        {
            sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

            using size_type = typename sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::size_type;

            vec.emplace_back(10);
            vec.emplace_back(20);
            vec.emplace_back(30);
            vec.emplace_back(40);
            vec.emplace_back(50);
            vec.emplace_back(60);
            vec.emplace_back(70);
            vec.emplace_back(80);
            vec.emplace_back(90);

            vec.pop_front();
            vec.pop_front();

            vec.pop_back();
            vec.pop_back();

            CHECK(vec.size() == 5);
            CHECK(*vec.nth(0) == 30);
            CHECK(*vec.nth(1) == 40);
            CHECK(*vec.nth(2) == 50);
            CHECK(*vec.nth(3) == 60);
            CHECK(*vec.nth(4) == 70);

            const size_type n = vec.available_front() + vec.size() + vec.available_back() - 1;

            CHECK(CONDITION);

            std::stringstream ss;
            for (int i = 0; i < int(n); ++i)
            {
                ss << i << " ";
            }

            vec.assign
            (
                (std::istream_iterator<int>(ss)),
                (std::istream_iterator<int>())
            );

            CHECK(vec.size() == n);
            for (int i = 0; i < int(n); ++i)
            {
                CHECK(*vec.nth(i) == i);
            }
        }
        #undef CONDITION

        #define CONDITION n > vec.size() && \
                          n > vec.size() + vec.available_back() && \
                          n == vec.available_front() + vec.size() + vec.available_back()
        {
            sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

            using size_type = typename sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::size_type;

            vec.emplace_back(10);
            vec.emplace_back(20);
            vec.emplace_back(30);
            vec.emplace_back(40);
            vec.emplace_back(50);
            vec.emplace_back(60);
            vec.emplace_back(70);
            vec.emplace_back(80);
            vec.emplace_back(90);

            vec.pop_front();
            vec.pop_front();

            vec.pop_back();
            vec.pop_back();

            CHECK(vec.size() == 5);
            CHECK(*vec.nth(0) == 30);
            CHECK(*vec.nth(1) == 40);
            CHECK(*vec.nth(2) == 50);
            CHECK(*vec.nth(3) == 60);
            CHECK(*vec.nth(4) == 70);

            const size_type n = vec.available_front() + vec.size() + vec.available_back();

            CHECK(CONDITION);

            std::stringstream ss;
            for (int i = 0; i < int(n); ++i)
            {
                ss << i << " ";
            }

            vec.assign
            (
                (std::istream_iterator<int>(ss)),
                (std::istream_iterator<int>())
            );

            CHECK(vec.size() == n);
            for (int i = 0; i < int(n); ++i)
            {
                CHECK(*vec.nth(i) == i);
            }
        }
        #undef CONDITION

        #define CONDITION n > vec.size() && \
                          n > vec.size() + vec.available_back() && \
                          n > vec.available_front() + vec.size() + vec.available_back()
        {
            sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

            using size_type = typename sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::size_type;

            vec.emplace_back(10);
            vec.emplace_back(20);
            vec.emplace_back(30);
            vec.emplace_back(40);
            vec.emplace_back(50);
            vec.emplace_back(60);
            vec.emplace_back(70);
            vec.emplace_back(80);
            vec.emplace_back(90);

            vec.pop_front();
            vec.pop_front();

            vec.pop_back();
            vec.pop_back();

            CHECK(vec.size() == 5);
            CHECK(*vec.nth(0) == 30);
            CHECK(*vec.nth(1) == 40);
            CHECK(*vec.nth(2) == 50);
            CHECK(*vec.nth(3) == 60);
            CHECK(*vec.nth(4) == 70);

            const size_type n = vec.available_front() + vec.size() + vec.available_back() + 1;

            CHECK(CONDITION);

            std::stringstream ss;
            for (int i = 0; i < int(n); ++i)
            {
                ss << i << " ";
            }

            vec.assign
            (
                (std::istream_iterator<int>(ss)),
                (std::istream_iterator<int>())
            );

            CHECK(vec.size() == n);
            for (int i = 0; i < int(n); ++i)
            {
                CHECK(*vec.nth(i) == i);
            }
        }
        #undef CONDITION
    }

    // Forward iterator
    {
        #define CONDITION n < vec.size()
        {
            sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

            using size_type = typename sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::size_type;

            vec.emplace_back(10);
            vec.emplace_back(20);
            vec.emplace_back(30);
            vec.emplace_back(40);
            vec.emplace_back(50);
            vec.emplace_back(60);
            vec.emplace_back(70);
            vec.emplace_back(80);
            vec.emplace_back(90);

            vec.pop_front();
            vec.pop_front();

            vec.pop_back();
            vec.pop_back();

            CHECK(vec.size() == 5);
            CHECK(*vec.nth(0) == 30);
            CHECK(*vec.nth(1) == 40);
            CHECK(*vec.nth(2) == 50);
            CHECK(*vec.nth(3) == 60);
            CHECK(*vec.nth(4) == 70);

            const size_type n = vec.size() - 1;

            CHECK(CONDITION);

            std::vector<xint> data;
            data.reserve(n);
            for (int i = 0; i < int(n); ++i)
            {
                data.emplace_back(i);
            }

            vec.assign(data.begin(), data.end());

            CHECK(vec.size() == n);
            for (int i = 0; i < int(n); ++i)
            {
                CHECK(*vec.nth(i) == i);
            }
        }
        #undef CONDITION

        #define CONDITION n == vec.size()
        {
            sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

            using size_type = typename sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::size_type;

            vec.emplace_back(10);
            vec.emplace_back(20);
            vec.emplace_back(30);
            vec.emplace_back(40);
            vec.emplace_back(50);
            vec.emplace_back(60);
            vec.emplace_back(70);
            vec.emplace_back(80);
            vec.emplace_back(90);

            vec.pop_front();
            vec.pop_front();

            vec.pop_back();
            vec.pop_back();

            CHECK(vec.size() == 5);
            CHECK(*vec.nth(0) == 30);
            CHECK(*vec.nth(1) == 40);
            CHECK(*vec.nth(2) == 50);
            CHECK(*vec.nth(3) == 60);
            CHECK(*vec.nth(4) == 70);

            const size_type n = vec.size();

            CHECK(CONDITION);

            std::vector<xint> data;
            data.reserve(n);
            for (int i = 0; i < int(n); ++i)
            {
                data.emplace_back(i);
            }

            vec.assign(data.begin(), data.end());

            CHECK(vec.size() == n);
            for (int i = 0; i < int(n); ++i)
            {
                CHECK(*vec.nth(i) == i);
            }
        }
        #undef CONDITION

        #define CONDITION n > vec.size() && \
                          n < vec.size() + vec.available_back()
        {
            sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

            using size_type = typename sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::size_type;

            vec.emplace_back(10);
            vec.emplace_back(20);
            vec.emplace_back(30);
            vec.emplace_back(40);
            vec.emplace_back(50);
            vec.emplace_back(60);
            vec.emplace_back(70);
            vec.emplace_back(80);
            vec.emplace_back(90);

            vec.pop_front();
            vec.pop_front();

            vec.pop_back();
            vec.pop_back();

            CHECK(vec.size() == 5);
            CHECK(*vec.nth(0) == 30);
            CHECK(*vec.nth(1) == 40);
            CHECK(*vec.nth(2) == 50);
            CHECK(*vec.nth(3) == 60);
            CHECK(*vec.nth(4) == 70);

            const size_type n = vec.size() + vec.available_back() - 1;

            CHECK(CONDITION);

            std::vector<xint> data;
            data.reserve(n);
            for (int i = 0; i < int(n); ++i)
            {
                data.emplace_back(i);
            }

            vec.assign(data.begin(), data.end());

            CHECK(vec.size() == n);
            for (int i = 0; i < int(n); ++i)
            {
                CHECK(*vec.nth(i) == i);
            }
        }
        #undef CONDITION

        #define CONDITION n > vec.size() && \
                          n == vec.size() + vec.available_back()
        {
            sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

            using size_type = typename sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::size_type;

            vec.emplace_back(10);
            vec.emplace_back(20);
            vec.emplace_back(30);
            vec.emplace_back(40);
            vec.emplace_back(50);
            vec.emplace_back(60);
            vec.emplace_back(70);
            vec.emplace_back(80);
            vec.emplace_back(90);

            vec.pop_front();
            vec.pop_front();

            vec.pop_back();
            vec.pop_back();

            CHECK(vec.size() == 5);
            CHECK(*vec.nth(0) == 30);
            CHECK(*vec.nth(1) == 40);
            CHECK(*vec.nth(2) == 50);
            CHECK(*vec.nth(3) == 60);
            CHECK(*vec.nth(4) == 70);

            const size_type n = vec.size() + vec.available_back();

            CHECK(CONDITION);

            std::vector<xint> data;
            data.reserve(n);
            for (int i = 0; i < int(n); ++i)
            {
                data.emplace_back(i);
            }

            vec.assign(data.begin(), data.end());

            CHECK(vec.size() == n);
            for (int i = 0; i < int(n); ++i)
            {
                CHECK(*vec.nth(i) == i);
            }
        }
        #undef CONDITION

        #define CONDITION n > vec.size() && \
                          n > vec.size() + vec.available_back() && \
                          n < vec.available_front() + vec.size() + vec.available_back()
        {
            sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

            using size_type = typename sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::size_type;

            vec.emplace_back(10);
            vec.emplace_back(20);
            vec.emplace_back(30);
            vec.emplace_back(40);
            vec.emplace_back(50);
            vec.emplace_back(60);
            vec.emplace_back(70);
            vec.emplace_back(80);
            vec.emplace_back(90);

            vec.pop_front();
            vec.pop_front();

            vec.pop_back();
            vec.pop_back();

            CHECK(vec.size() == 5);
            CHECK(*vec.nth(0) == 30);
            CHECK(*vec.nth(1) == 40);
            CHECK(*vec.nth(2) == 50);
            CHECK(*vec.nth(3) == 60);
            CHECK(*vec.nth(4) == 70);

            const size_type n = vec.available_front() + vec.size() + vec.available_back() - 1;

            CHECK(CONDITION);

            std::vector<xint> data;
            data.reserve(n);
            for (int i = 0; i < int(n); ++i)
            {
                data.emplace_back(i);
            }

            vec.assign(data.begin(), data.end());

            CHECK(vec.size() == n);
            for (int i = 0; i < int(n); ++i)
            {
                CHECK(*vec.nth(i) == i);
            }
        }
        #undef CONDITION

        #define CONDITION n > vec.size() && \
                          n > vec.size() + vec.available_back() && \
                          n == vec.available_front() + vec.size() + vec.available_back()
        {
            sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

            using size_type = typename sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::size_type;

            vec.emplace_back(10);
            vec.emplace_back(20);
            vec.emplace_back(30);
            vec.emplace_back(40);
            vec.emplace_back(50);
            vec.emplace_back(60);
            vec.emplace_back(70);
            vec.emplace_back(80);
            vec.emplace_back(90);

            vec.pop_front();
            vec.pop_front();

            vec.pop_back();
            vec.pop_back();

            CHECK(vec.size() == 5);
            CHECK(*vec.nth(0) == 30);
            CHECK(*vec.nth(1) == 40);
            CHECK(*vec.nth(2) == 50);
            CHECK(*vec.nth(3) == 60);
            CHECK(*vec.nth(4) == 70);

            const size_type n = vec.available_front() + vec.size() + vec.available_back();

            CHECK(CONDITION);

            std::vector<xint> data;
            data.reserve(n);
            for (int i = 0; i < int(n); ++i)
            {
                data.emplace_back(i);
            }

            vec.assign(data.begin(), data.end());

            CHECK(vec.size() == n);
            for (int i = 0; i < int(n); ++i)
            {
                CHECK(*vec.nth(i) == i);
            }
        }
        #undef CONDITION

        #define CONDITION n > vec.size() && \
                          n > vec.size() + vec.available_back() && \
                          n > vec.available_front() + vec.size() + vec.available_back()
        {
            sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

            using size_type = typename sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>::size_type;

            vec.emplace_back(10);
            vec.emplace_back(20);
            vec.emplace_back(30);
            vec.emplace_back(40);
            vec.emplace_back(50);
            vec.emplace_back(60);
            vec.emplace_back(70);
            vec.emplace_back(80);
            vec.emplace_back(90);

            vec.pop_front();
            vec.pop_front();

            vec.pop_back();
            vec.pop_back();

            CHECK(vec.size() == 5);
            CHECK(*vec.nth(0) == 30);
            CHECK(*vec.nth(1) == 40);
            CHECK(*vec.nth(2) == 50);
            CHECK(*vec.nth(3) == 60);
            CHECK(*vec.nth(4) == 70);

            const size_type n = vec.available_front() + vec.size() + vec.available_back() + 1;

            CHECK(CONDITION);

            std::vector<xint> data;
            data.reserve(n);
            for (int i = 0; i < int(n); ++i)
            {
                data.emplace_back(i);
            }

            vec.assign(data.begin(), data.end());

            CHECK(vec.size() == n);
            for (int i = 0; i < int(n); ++i)
            {
                CHECK(*vec.nth(i) == i);
            }
        }
        #undef CONDITION
    }
}

PRINT("Test assign(std::initializer_list");
{
    sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

    vec.emplace(vec.end(), 10);
    vec.emplace(vec.end(), 20);
    vec.emplace(vec.end(), 30);
    vec.emplace(vec.end(), 40);
    vec.emplace(vec.end(), 50);

    CHECK(vec.size() == 5);
    CHECK(*vec.nth(0) == 10);
    CHECK(*vec.nth(1) == 20);
    CHECK(*vec.nth(2) == 30);
    CHECK(*vec.nth(3) == 40);
    CHECK(*vec.nth(4) == 50);

    std::initializer_list<xint> ilist{1, 2, 3, 4, 5};

    vec.assign(ilist);

    CHECK(vec.size() == 5);
    CHECK(*vec.nth(0) == 1);
    CHECK(*vec.nth(1) == 2);
    CHECK(*vec.nth(2) == 3);
    CHECK(*vec.nth(3) == 4);
    CHECK(*vec.nth(4) == 5);
}

PRINT("Test assign_range(Range&&)");
{
    // Input iterator (exactly)
    {
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        std::istringstream iss("10 20 30 40");

        #if SFL_CPP_VERSION >= SFL_CPP_20
        vec.assign_range(std::views::istream<int>(iss));
        #else
        vec.assign_range(sfl::test::istream_view<int>(iss));
        #endif

        CHECK(vec.size() == 4);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
    }

    // Forward iterator
    {
        std::vector<int> data({10, 20, 30, 40});

        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

        #if SFL_CPP_VERSION >= SFL_CPP_20
        vec.assign_range(std::views::all(data));
        #else
        vec.assign_range(data);
        #endif

        CHECK(vec.size() == 4);
        CHECK(*vec.nth(0) == 10);
        CHECK(*vec.nth(1) == 20);
        CHECK(*vec.nth(2) == 30);
        CHECK(*vec.nth(3) == 40);
    }
}

///////////////////////////////////////////////////////////////////////////////

PRINT("Test NON-MEMBER comparison operators");
{
    sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec1, vec2;

    vec1.emplace(vec1.end(), 10);
    vec1.emplace(vec1.end(), 20);
    vec1.emplace(vec1.end(), 30);

    vec2.emplace(vec2.end(), 10);
    vec2.emplace(vec2.end(), 20);
    vec2.emplace(vec2.end(), 30);
    vec2.emplace(vec2.end(), 40);
    vec2.emplace(vec2.end(), 50);

    CHECK((vec1 == vec1) == true);
    CHECK((vec1 == vec2) == false);
    CHECK((vec2 == vec1) == false);
    CHECK((vec2 == vec2) == true);

    CHECK((vec1 != vec1) == false);
    CHECK((vec1 != vec2) == true);
    CHECK((vec2 != vec1) == true);
    CHECK((vec2 != vec2) == false);

    CHECK((vec1 < vec1) == false);
    CHECK((vec1 < vec2) == true);
    CHECK((vec2 < vec1) == false);
    CHECK((vec2 < vec2) == false);

    CHECK((vec1 > vec1) == false);
    CHECK((vec1 > vec2) == false);
    CHECK((vec2 > vec1) == true);
    CHECK((vec2 > vec2) == false);

    CHECK((vec1 <= vec1) == true);
    CHECK((vec1 <= vec2) == true);
    CHECK((vec2 <= vec1) == false);
    CHECK((vec2 <= vec2) == true);

    CHECK((vec1 >= vec1) == true);
    CHECK((vec1 >= vec2) == false);
    CHECK((vec2 >= vec1) == true);
    CHECK((vec2 >= vec2) == true);
}

PRINT("Test NON-MEMBER swap(container&)");
{
    sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec1, vec2;

    vec1.emplace(vec1.end(), 10);
    vec1.emplace(vec1.end(), 20);
    vec1.emplace(vec1.end(), 30);

    vec2.emplace(vec2.end(), 40);
    vec2.emplace(vec2.end(), 50);
    vec2.emplace(vec2.end(), 60);
    vec2.emplace(vec2.end(), 70);
    vec2.emplace(vec2.end(), 80);

    CHECK(vec1.size() == 3);
    CHECK(*vec1.nth(0) == 10);
    CHECK(*vec1.nth(1) == 20);
    CHECK(*vec1.nth(2) == 30);

    CHECK(vec2.size() == 5);
    CHECK(*vec2.nth(0) == 40);
    CHECK(*vec2.nth(1) == 50);
    CHECK(*vec2.nth(2) == 60);
    CHECK(*vec2.nth(3) == 70);
    CHECK(*vec2.nth(4) == 80);

    ///////////////////////////////////////////////////////////////////////////

    swap(vec1, vec2);

    CHECK(vec1.size() == 5);
    CHECK(*vec1.nth(0) == 40);
    CHECK(*vec1.nth(1) == 50);
    CHECK(*vec1.nth(2) == 60);
    CHECK(*vec1.nth(3) == 70);
    CHECK(*vec1.nth(4) == 80);

    CHECK(vec2.size() == 3);
    CHECK(*vec2.nth(0) == 10);
    CHECK(*vec2.nth(1) == 20);
    CHECK(*vec2.nth(2) == 30);
}

PRINT("Test NON-MEMBER erase(container&, const U&)");
{
    sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>> vec;

    vec.emplace(vec.end(), 10);
    vec.emplace(vec.end(), 20);
    vec.emplace(vec.end(), 20);
    vec.emplace(vec.end(), 20);
    vec.emplace(vec.end(), 30);

    CHECK(vec.size() == 5);
    CHECK(*vec.nth(0) == 10);
    CHECK(*vec.nth(1) == 20);
    CHECK(*vec.nth(2) == 20);
    CHECK(*vec.nth(3) == 20);
    CHECK(*vec.nth(4) == 30);

    ///////////////////////////////////////////////////////////////////////////

    CHECK(erase(vec, 20) == 3);
    CHECK(vec.size() == 2);
    CHECK(*vec.nth(0) == 10);
    CHECK(*vec.nth(1) == 30);

    ///////////////////////////////////////////////////////////////////////////

    CHECK(erase(vec, 20) == 0);
    CHECK(vec.size() == 2);
    CHECK(*vec.nth(0) == 10);
    CHECK(*vec.nth(1) == 30);
}

PRINT("Test NON-MEMBER erase_if(container&, Predicate)");
{
    using container_type =
        sfl::segmented_devector<xint, TPARAM_N, TPARAM_ALLOCATOR<xint>>;

    using const_reference = typename container_type::const_reference;

    ///////////////////////////////////////////////////////////////////////////

    container_type vec;

    vec.emplace(vec.end(), 10);
    vec.emplace(vec.end(), 20);
    vec.emplace(vec.end(), 20);
    vec.emplace(vec.end(), 20);
    vec.emplace(vec.end(), 30);

    CHECK(vec.size() == 5);
    CHECK(*vec.nth(0) == 10);
    CHECK(*vec.nth(1) == 20);
    CHECK(*vec.nth(2) == 20);
    CHECK(*vec.nth(3) == 20);
    CHECK(*vec.nth(4) == 30);

    ///////////////////////////////////////////////////////////////////////////

    CHECK(erase_if(vec, [](const_reference& value){ return value == 20; }) == 3);
    CHECK(vec.size() == 2);
    CHECK(*vec.nth(0) == 10);
    CHECK(*vec.nth(1) == 30);

    ///////////////////////////////////////////////////////////////////////////

    CHECK(erase_if(vec, [](const_reference& value){ return value == 20; }) == 0);
    CHECK(vec.size() == 2);
    CHECK(*vec.nth(0) == 10);
    CHECK(*vec.nth(1) == 30);
}
