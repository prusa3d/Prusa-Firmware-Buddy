#ifndef TPARAM_ALLOCATOR
#error Please define macro TPARAM_ALLOCATOR
#endif

using sfl::test::xint;
using sfl::test::xobj;

PRINT("Test container()");
{
    sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map;

    CHECK(map.empty() == true);
    CHECK(map.size() == 0);
    CHECK(map.max_size() > 0);
}

PRINT("Test container(const Compare&)");
{
    std::less<xint> comp;

    sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map(comp);

    CHECK(map.empty() == true);
    CHECK(map.size() == 0);
    CHECK(map.max_size() > 0);
}

PRINT("Test container(const Allocator&)");
{
    TPARAM_ALLOCATOR<std::pair<const xint, xint>> alloc;

    sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map(alloc);

    CHECK(map.empty() == true);
    CHECK(map.size() == 0);
    CHECK(map.max_size() > 0);
}

PRINT("Test container(const Compare&, const Allocator&)");
{
    std::less<xint> comp;

    TPARAM_ALLOCATOR<std::pair<const xint, xint>> alloc;

    sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map(comp, alloc);

    CHECK(map.empty() == true);
    CHECK(map.size() == 0);
    CHECK(map.max_size() > 0);
}

PRINT("Test container(InputIt, InputIt)");
{
    std::vector<std::pair<int, int>> data
    (
        {
            {10, 1},
            {20, 1},
            {30, 1},
            {20, 2},
            {20, 3}
        }
    );

    sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map(data.begin(), data.end());

    CHECK(map.empty() == false);
    CHECK(map.size() == 5);
    CHECK(map.max_size() > 0);
    CHECK(NTH(map, 0)->first == 10); CHECK(NTH(map, 0)->second == 1);
    CHECK(NTH(map, 1)->first == 20); CHECK(NTH(map, 1)->second == 1);
    CHECK(NTH(map, 2)->first == 20); CHECK(NTH(map, 2)->second == 2);
    CHECK(NTH(map, 3)->first == 20); CHECK(NTH(map, 3)->second == 3);
    CHECK(NTH(map, 4)->first == 30); CHECK(NTH(map, 4)->second == 1);
}

PRINT("Test container(InputIt, InputIt, const Compare&)");
{
    std::vector<std::pair<int, int>> data
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

    sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map(data.begin(), data.end(), comp);

    CHECK(map.empty() == false);
    CHECK(map.size() == 5);
    CHECK(map.max_size() > 0);
    CHECK(NTH(map, 0)->first == 10); CHECK(NTH(map, 0)->second == 1);
    CHECK(NTH(map, 1)->first == 20); CHECK(NTH(map, 1)->second == 1);
    CHECK(NTH(map, 2)->first == 20); CHECK(NTH(map, 2)->second == 2);
    CHECK(NTH(map, 3)->first == 20); CHECK(NTH(map, 3)->second == 3);
    CHECK(NTH(map, 4)->first == 30); CHECK(NTH(map, 4)->second == 1);
}

PRINT("Test container(InputIt, InputIt, const Allocator&)");
{
    std::vector<std::pair<int, int>> data
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

    sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map(data.begin(), data.end(), alloc);

    CHECK(map.empty() == false);
    CHECK(map.size() == 5);
    CHECK(map.max_size() > 0);
    CHECK(NTH(map, 0)->first == 10); CHECK(NTH(map, 0)->second == 1);
    CHECK(NTH(map, 1)->first == 20); CHECK(NTH(map, 1)->second == 1);
    CHECK(NTH(map, 2)->first == 20); CHECK(NTH(map, 2)->second == 2);
    CHECK(NTH(map, 3)->first == 20); CHECK(NTH(map, 3)->second == 3);
    CHECK(NTH(map, 4)->first == 30); CHECK(NTH(map, 4)->second == 1);
}

PRINT("Test container(InputIt, InputIt, const Compare&, const Allocator&)");
{
    std::vector<std::pair<int, int>> data
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

    TPARAM_ALLOCATOR<std::pair<const xint, xint>> alloc;

    sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map(data.begin(), data.end(), comp, alloc);

    CHECK(map.empty() == false);
    CHECK(map.size() == 5);
    CHECK(map.max_size() > 0);
    CHECK(NTH(map, 0)->first == 10); CHECK(NTH(map, 0)->second == 1);
    CHECK(NTH(map, 1)->first == 20); CHECK(NTH(map, 1)->second == 1);
    CHECK(NTH(map, 2)->first == 20); CHECK(NTH(map, 2)->second == 2);
    CHECK(NTH(map, 3)->first == 20); CHECK(NTH(map, 3)->second == 3);
    CHECK(NTH(map, 4)->first == 30); CHECK(NTH(map, 4)->second == 1);
}

PRINT("Test container(std::initializer_list)");
{
    std::initializer_list<std::pair<const xint, xint>> ilist
    {
        {10, 1},
        {20, 1},
        {30, 1},
        {20, 2},
        {20, 3}
    };

    sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map(ilist);

    CHECK(map.empty() == false);
    CHECK(map.size() == 5);
    CHECK(map.max_size() > 0);
    CHECK(NTH(map, 0)->first == 10); CHECK(NTH(map, 0)->second == 1);
    CHECK(NTH(map, 1)->first == 20); CHECK(NTH(map, 1)->second == 1);
    CHECK(NTH(map, 2)->first == 20); CHECK(NTH(map, 2)->second == 2);
    CHECK(NTH(map, 3)->first == 20); CHECK(NTH(map, 3)->second == 3);
    CHECK(NTH(map, 4)->first == 30); CHECK(NTH(map, 4)->second == 1);
}

PRINT("Test container(std::initializer_list, const Compare&)");
{
    std::initializer_list<std::pair<const xint, xint>> ilist
    {
        {10, 1},
        {20, 1},
        {30, 1},
        {20, 2},
        {20, 3}
    };

    std::less<xint> comp;

    sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map(ilist, comp);

    CHECK(map.empty() == false);
    CHECK(map.size() == 5);
    CHECK(map.max_size() > 0);
    CHECK(NTH(map, 0)->first == 10); CHECK(NTH(map, 0)->second == 1);
    CHECK(NTH(map, 1)->first == 20); CHECK(NTH(map, 1)->second == 1);
    CHECK(NTH(map, 2)->first == 20); CHECK(NTH(map, 2)->second == 2);
    CHECK(NTH(map, 3)->first == 20); CHECK(NTH(map, 3)->second == 3);
    CHECK(NTH(map, 4)->first == 30); CHECK(NTH(map, 4)->second == 1);
}

PRINT("Test container(std::initializer_list, const Allocator&)");
{
    std::initializer_list<std::pair<const xint, xint>> ilist
    {
        {10, 1},
        {20, 1},
        {30, 1},
        {20, 2},
        {20, 3}
    };

    TPARAM_ALLOCATOR<std::pair<const xint, xint>> alloc;

    sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map(ilist, alloc);

    CHECK(map.empty() == false);
    CHECK(map.size() == 5);
    CHECK(map.max_size() > 0);
    CHECK(NTH(map, 0)->first == 10); CHECK(NTH(map, 0)->second == 1);
    CHECK(NTH(map, 1)->first == 20); CHECK(NTH(map, 1)->second == 1);
    CHECK(NTH(map, 2)->first == 20); CHECK(NTH(map, 2)->second == 2);
    CHECK(NTH(map, 3)->first == 20); CHECK(NTH(map, 3)->second == 3);
    CHECK(NTH(map, 4)->first == 30); CHECK(NTH(map, 4)->second == 1);
}

PRINT("Test container(std::initializer_list, const Compare&, const Allocator&)");
{
    std::initializer_list<std::pair<const xint, xint>> ilist
    {
        {10, 1},
        {20, 1},
        {30, 1},
        {20, 2},
        {20, 3}
    };

    std::less<xint> comp;

    TPARAM_ALLOCATOR<std::pair<const xint, xint>> alloc;

    sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map(ilist, comp, alloc);

    CHECK(map.empty() == false);
    CHECK(map.size() == 5);
    CHECK(map.max_size() > 0);
    CHECK(NTH(map, 0)->first == 10); CHECK(NTH(map, 0)->second == 1);
    CHECK(NTH(map, 1)->first == 20); CHECK(NTH(map, 1)->second == 1);
    CHECK(NTH(map, 2)->first == 20); CHECK(NTH(map, 2)->second == 2);
    CHECK(NTH(map, 3)->first == 20); CHECK(NTH(map, 3)->second == 3);
    CHECK(NTH(map, 4)->first == 30); CHECK(NTH(map, 4)->second == 1);
}

PRINT("Test container(const container&)");
{
    sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map1;

    map1.emplace(10, 1);
    map1.emplace(20, 1);
    map1.emplace(30, 1);

    CHECK(map1.size() == 3);
    CHECK(NTH(map1, 0)->first == 10); CHECK(NTH(map1, 0)->second == 1);
    CHECK(NTH(map1, 1)->first == 20); CHECK(NTH(map1, 1)->second == 1);
    CHECK(NTH(map1, 2)->first == 30); CHECK(NTH(map1, 2)->second == 1);

    ///////////////////////////////////////////////////////////////////////

    sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map2(map1);

    CHECK(map2.size() == 3);
    CHECK(NTH(map2, 0)->first == 10); CHECK(NTH(map2, 0)->second == 1);
    CHECK(NTH(map2, 1)->first == 20); CHECK(NTH(map2, 1)->second == 1);
    CHECK(NTH(map2, 2)->first == 30); CHECK(NTH(map2, 2)->second == 1);
}

PRINT("Test container(const container&, const Allocator&)");
{
    sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map1;

    map1.emplace(10, 1);
    map1.emplace(20, 1);
    map1.emplace(30, 1);

    CHECK(map1.size() == 3);
    CHECK(NTH(map1, 0)->first == 10); CHECK(NTH(map1, 0)->second == 1);
    CHECK(NTH(map1, 1)->first == 20); CHECK(NTH(map1, 1)->second == 1);
    CHECK(NTH(map1, 2)->first == 30); CHECK(NTH(map1, 2)->second == 1);

    ///////////////////////////////////////////////////////////////////////

    TPARAM_ALLOCATOR<std::pair<const xint, xint>> alloc;

    sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map2(map1, alloc);

    CHECK(map2.size() == 3);
    CHECK(NTH(map2, 0)->first == 10); CHECK(NTH(map2, 0)->second == 1);
    CHECK(NTH(map2, 1)->first == 20); CHECK(NTH(map2, 1)->second == 1);
    CHECK(NTH(map2, 2)->first == 30); CHECK(NTH(map2, 2)->second == 1);
}

PRINT("Test container(container&&)");
{
    sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map1;

    map1.emplace(10, 1);
    map1.emplace(20, 1);
    map1.emplace(30, 1);

    CHECK(map1.size() == 3);
    CHECK(NTH(map1, 0)->first == 10); CHECK(NTH(map1, 0)->second == 1);
    CHECK(NTH(map1, 1)->first == 20); CHECK(NTH(map1, 1)->second == 1);
    CHECK(NTH(map1, 2)->first == 30); CHECK(NTH(map1, 2)->second == 1);

    ///////////////////////////////////////////////////////////////////////

    sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map2(std::move(map1));

    CHECK(map1.size() == 0);
    CHECK(map2.size() == 3);
    CHECK(NTH(map2, 0)->first == 10); CHECK(NTH(map2, 0)->second == 1);
    CHECK(NTH(map2, 1)->first == 20); CHECK(NTH(map2, 1)->second == 1);
    CHECK(NTH(map2, 2)->first == 30); CHECK(NTH(map2, 2)->second == 1);
}

PRINT("Test container(container&&, const Allocator&)");
{
    sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map1;

    map1.emplace(10, 1);
    map1.emplace(20, 1);
    map1.emplace(30, 1);

    CHECK(map1.size() == 3);
    CHECK(NTH(map1, 0)->first == 10); CHECK(NTH(map1, 0)->second == 1);
    CHECK(NTH(map1, 1)->first == 20); CHECK(NTH(map1, 1)->second == 1);
    CHECK(NTH(map1, 2)->first == 30); CHECK(NTH(map1, 2)->second == 1);

    ///////////////////////////////////////////////////////////////////////

    TPARAM_ALLOCATOR<std::pair<const xint, xint>> alloc;

    sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map2(std::move(map1), alloc);

    if (map1.size() == 3)
    {
        // Elements are moved one-by-one
        CHECK(NTH(map1, 0)->first == +10); CHECK(NTH(map1, 0)->second == -1);
        CHECK(NTH(map1, 1)->first == +20); CHECK(NTH(map1, 1)->second == -1);
        CHECK(NTH(map1, 2)->first == +30); CHECK(NTH(map1, 2)->second == -1);
    }
    else
    {
        CHECK(map1.size() == 0);
    }

    CHECK(map2.size() == 3);
    CHECK(NTH(map2, 0)->first == 10); CHECK(NTH(map2, 0)->second == 1);
    CHECK(NTH(map2, 1)->first == 20); CHECK(NTH(map2, 1)->second == 1);
    CHECK(NTH(map2, 2)->first == 30); CHECK(NTH(map2, 2)->second == 1);
}

PRINT("Test container(sfl::from_range_t, Range&&)");
{
    // Input iterator (exactly)
    {
        std::istringstream iss("10 1 20 1 30 1 20 2 20 3");

        #if SFL_CPP_VERSION >= SFL_CPP_20
        sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map
        (
            (sfl::from_range_t()),
            (std::views::istream<std::pair<int, int>>(iss))
        );
        #else
        sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map
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
        CHECK(NTH(map, 2)->first == 20); CHECK(NTH(map, 2)->second == 2);
        CHECK(NTH(map, 3)->first == 20); CHECK(NTH(map, 3)->second == 3);
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
        sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map
        (
            sfl::from_range_t(),
            std::views::all(data)
        );
        #else
        sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map
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
        CHECK(NTH(map, 2)->first == 20); CHECK(NTH(map, 2)->second == 2);
        CHECK(NTH(map, 3)->first == 20); CHECK(NTH(map, 3)->second == 3);
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
        sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map
        (
            (sfl::from_range_t()),
            (std::views::istream<std::pair<int, int>>(iss)),
            comp
        );
        #else
        sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map
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
        CHECK(NTH(map, 1)->first == 20); CHECK(NTH(map, 1)->second == 1);
        CHECK(NTH(map, 2)->first == 20); CHECK(NTH(map, 2)->second == 2);
        CHECK(NTH(map, 3)->first == 20); CHECK(NTH(map, 3)->second == 3);
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
        sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map
        (
            sfl::from_range_t(),
            std::views::all(data),
            comp
        );
        #else
        sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map
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
        CHECK(NTH(map, 1)->first == 20); CHECK(NTH(map, 1)->second == 1);
        CHECK(NTH(map, 2)->first == 20); CHECK(NTH(map, 2)->second == 2);
        CHECK(NTH(map, 3)->first == 20); CHECK(NTH(map, 3)->second == 3);
        CHECK(NTH(map, 4)->first == 30); CHECK(NTH(map, 4)->second == 1);
    }
}

PRINT("Test container(sfl::from_range_t, Range&&, const Allocator&)");
{
    // Input iterator (exactly)
    {
        std::istringstream iss("10 1 20 1 30 1 20 2 20 3");

        TPARAM_ALLOCATOR<std::pair<const xint, xint>> alloc;

        #if SFL_CPP_VERSION >= SFL_CPP_20
        sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map
        (
            (sfl::from_range_t()),
            (std::views::istream<std::pair<int, int>>(iss)),
            alloc
        );
        #else
        sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map
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
        CHECK(NTH(map, 2)->first == 20); CHECK(NTH(map, 2)->second == 2);
        CHECK(NTH(map, 3)->first == 20); CHECK(NTH(map, 3)->second == 3);
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

        TPARAM_ALLOCATOR<std::pair<const xint, xint>> alloc;

        #if SFL_CPP_VERSION >= SFL_CPP_20
        sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map
        (
            sfl::from_range_t(),
            std::views::all(data),
            alloc
        );
        #else
        sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map
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
        CHECK(NTH(map, 2)->first == 20); CHECK(NTH(map, 2)->second == 2);
        CHECK(NTH(map, 3)->first == 20); CHECK(NTH(map, 3)->second == 3);
        CHECK(NTH(map, 4)->first == 30); CHECK(NTH(map, 4)->second == 1);
    }
}

PRINT("Test container(sfl::from_range_t, Range&&, const Compare&, const Allocator&)");
{
    // Input iterator (exactly)
    {
        std::istringstream iss("10 1 20 1 30 1 20 2 20 3");

        std::less<xint> comp;
        TPARAM_ALLOCATOR<std::pair<const xint, xint>> alloc;

        #if SFL_CPP_VERSION >= SFL_CPP_20
        sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map
        (
            (sfl::from_range_t()),
            (std::views::istream<std::pair<int, int>>(iss)),
            comp,
            alloc
        );
        #else
        sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map
        (
            (sfl::from_range_t()),
            (sfl::test::istream_view<std::pair<int, int>>(iss)),
            comp,
            alloc
        );
        #endif

        CHECK(map.empty() == false);
        CHECK(map.size() == 5);
        CHECK(map.max_size() > 0);
        CHECK(NTH(map, 0)->first == 10); CHECK(NTH(map, 0)->second == 1);
        CHECK(NTH(map, 1)->first == 20); CHECK(NTH(map, 1)->second == 1);
        CHECK(NTH(map, 2)->first == 20); CHECK(NTH(map, 2)->second == 2);
        CHECK(NTH(map, 3)->first == 20); CHECK(NTH(map, 3)->second == 3);
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
        TPARAM_ALLOCATOR<std::pair<const xint, xint>> alloc;

        #if SFL_CPP_VERSION >= SFL_CPP_20
        sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map
        (
            sfl::from_range_t(),
            std::views::all(data),
            comp,
            alloc
        );
        #else
        sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map
        (
            sfl::from_range_t(),
            data,
            comp,
            alloc
        );
        #endif

        CHECK(map.empty() == false);
        CHECK(map.size() == 5);
        CHECK(map.max_size() > 0);
        CHECK(NTH(map, 0)->first == 10); CHECK(NTH(map, 0)->second == 1);
        CHECK(NTH(map, 1)->first == 20); CHECK(NTH(map, 1)->second == 1);
        CHECK(NTH(map, 2)->first == 20); CHECK(NTH(map, 2)->second == 2);
        CHECK(NTH(map, 3)->first == 20); CHECK(NTH(map, 3)->second == 3);
        CHECK(NTH(map, 4)->first == 30); CHECK(NTH(map, 4)->second == 1);
    }
}

///////////////////////////////////////////////////////////////////////////////

PRINT("Test operator=(const container&)");
{
    sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map1, map2;

    map1.emplace(10, 1);
    map1.emplace(20, 1);
    map1.emplace(30, 1);

    CHECK(map1.size() == 3);
    CHECK(NTH(map1, 0)->first == 10); CHECK(NTH(map1, 0)->second == 1);
    CHECK(NTH(map1, 1)->first == 20); CHECK(NTH(map1, 1)->second == 1);
    CHECK(NTH(map1, 2)->first == 30); CHECK(NTH(map1, 2)->second == 1);

    map2.emplace(40, 2);
    map2.emplace(50, 2);
    map2.emplace(60, 2);

    CHECK(map2.size() == 3);
    CHECK(NTH(map2, 0)->first == 40); CHECK(NTH(map2, 0)->second == 2);
    CHECK(NTH(map2, 1)->first == 50); CHECK(NTH(map2, 1)->second == 2);
    CHECK(NTH(map2, 2)->first == 60); CHECK(NTH(map2, 2)->second == 2);

    ///////////////////////////////////////////////////////////////////////

    map2 = map1;

    CHECK(map1.size() == 3);
    CHECK(NTH(map1, 0)->first == 10); CHECK(NTH(map1, 0)->second == 1);
    CHECK(NTH(map1, 1)->first == 20); CHECK(NTH(map1, 1)->second == 1);
    CHECK(NTH(map1, 2)->first == 30); CHECK(NTH(map1, 2)->second == 1);

    CHECK(map2.size() == 3);
    CHECK(NTH(map2, 0)->first == 10); CHECK(NTH(map2, 0)->second == 1);
    CHECK(NTH(map2, 1)->first == 20); CHECK(NTH(map2, 1)->second == 1);
    CHECK(NTH(map2, 2)->first == 30); CHECK(NTH(map2, 2)->second == 1);
}

PRINT("Test operator=(container&&)");
{
    sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map1, map2;

    map1.emplace(10, 1);
    map1.emplace(20, 1);
    map1.emplace(30, 1);

    CHECK(map1.size() == 3);
    CHECK(NTH(map1, 0)->first == 10); CHECK(NTH(map1, 0)->second == 1);
    CHECK(NTH(map1, 1)->first == 20); CHECK(NTH(map1, 1)->second == 1);
    CHECK(NTH(map1, 2)->first == 30); CHECK(NTH(map1, 2)->second == 1);

    map2.emplace(40, 2);
    map2.emplace(50, 2);
    map2.emplace(60, 2);

    CHECK(map2.size() == 3);
    CHECK(NTH(map2, 0)->first == 40); CHECK(NTH(map2, 0)->second == 2);
    CHECK(NTH(map2, 1)->first == 50); CHECK(NTH(map2, 1)->second == 2);
    CHECK(NTH(map2, 2)->first == 60); CHECK(NTH(map2, 2)->second == 2);

    ///////////////////////////////////////////////////////////////////////

    map2 = std::move(map1);

    if (map1.size() == 3)
    {
        // Elements are move one-by-one
        CHECK(NTH(map1, 0)->first == +10); CHECK(NTH(map1, 0)->second == -1);
        CHECK(NTH(map1, 1)->first == +20); CHECK(NTH(map1, 1)->second == -1);
        CHECK(NTH(map1, 2)->first == +30); CHECK(NTH(map1, 2)->second == -1);
    }
    else
    {
        CHECK(map1.size() == 0);
    }

    CHECK(map2.size() == 3);
    CHECK(NTH(map2, 0)->first == 10); CHECK(NTH(map2, 0)->second == 1);
    CHECK(NTH(map2, 1)->first == 20); CHECK(NTH(map2, 1)->second == 1);
    CHECK(NTH(map2, 2)->first == 30); CHECK(NTH(map2, 2)->second == 1);
}

PRINT("Test operator=(std::initializer_list)");
{
    sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map;

    map.emplace(10, 1);
    map.emplace(20, 1);
    map.emplace(30, 1);

    CHECK(map.size() == 3);
    CHECK(NTH(map, 0)->first == 10); CHECK(NTH(map, 0)->second == 1);
    CHECK(NTH(map, 1)->first == 20); CHECK(NTH(map, 1)->second == 1);
    CHECK(NTH(map, 2)->first == 30); CHECK(NTH(map, 2)->second == 1);

    ///////////////////////////////////////////////////////////////////////

    std::initializer_list<std::pair<const xint, xint>> ilist
    {
        {40, 2},
        {50, 2},
        {60, 2},
        {50, 3},
        {50, 4}
    };

    map = ilist;

    CHECK(map.size() == 5);
    CHECK(NTH(map, 0)->first == 40); CHECK(NTH(map, 0)->second == 2);
    CHECK(NTH(map, 1)->first == 50); CHECK(NTH(map, 1)->second == 2);
    CHECK(NTH(map, 2)->first == 50); CHECK(NTH(map, 2)->second == 3);
    CHECK(NTH(map, 3)->first == 50); CHECK(NTH(map, 3)->second == 4);
    CHECK(NTH(map, 4)->first == 60); CHECK(NTH(map, 4)->second == 2);
}

///////////////////////////////////////////////////////////////////////////////

PRINT("Test get_allocator()");
{
    sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map;

    auto alloc = map.get_allocator();

    (void)alloc;
}

///////////////////////////////////////////////////////////////////////////////

PRINT("Test key_comp()");
{
    {
        sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map;

        auto key_comp = map.key_comp();

        CHECK(key_comp(10, 10) == false);
        CHECK(key_comp(10, 20) == true);
        CHECK(key_comp(20, 10) == false);
        CHECK(key_comp(20, 20) == false);
    }

    {
        sfl::multimap<xobj, xint, xobj::less, TPARAM_ALLOCATOR<std::pair<const xobj, xint>>> map;

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
        sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map;

        auto value_comp = map.value_comp();

        CHECK(value_comp({10, 1}, {10, 2}) == false);
        CHECK(value_comp({10, 1}, {20, 2}) == true);
        CHECK(value_comp({20, 1}, {10, 2}) == false);
        CHECK(value_comp({20, 1}, {20, 2}) == false);
    }

    {
        sfl::multimap<xobj, xint, xobj::less, TPARAM_ALLOCATOR<std::pair<const xobj, xint>>> map;

        auto value_comp = map.value_comp();

        CHECK(value_comp({xobj(10), 1}, {xobj(10), 2}) == false);
        CHECK(value_comp({xobj(10), 1}, {xobj(20), 2}) == true);
        CHECK(value_comp({xobj(20), 1}, {xobj(10), 2}) == false);
        CHECK(value_comp({xobj(20), 1}, {xobj(20), 2}) == false);
    }
}

///////////////////////////////////////////////////////////////////////////////

PRINT("Test begin, end, cbegin, cend, rbegin, rend, crbegin, crend");
{
    sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map;

    map.emplace(20, 1);
    map.emplace(40, 1);
    map.emplace(60, 1);

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
}

///////////////////////////////////////////////////////////////////////////////

PRINT("Test lower_bound, upper_bound, equal_range, find, count, contains");
{
    // xint, xint
    {
        sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>> > map;

        map.emplace(20, 1);
        map.emplace(40, 1);
        map.emplace(60, 1);

        ///////////////////////////////////////////////////////////////////////

        CHECK(map.lower_bound(10) == NTH(map, 0));
        CHECK(map.lower_bound(20) == NTH(map, 0));
        CHECK(map.lower_bound(30) == NTH(map, 1));
        CHECK(map.lower_bound(40) == NTH(map, 1));
        CHECK(map.lower_bound(50) == NTH(map, 2));
        CHECK(map.lower_bound(60) == NTH(map, 2));
        CHECK(map.lower_bound(70) == NTH(map, 3));

        ///////////////////////////////////////////////////////////////////////

        CHECK(map.upper_bound(10) == NTH(map, 0));
        CHECK(map.upper_bound(20) == NTH(map, 1));
        CHECK(map.upper_bound(30) == NTH(map, 1));
        CHECK(map.upper_bound(40) == NTH(map, 2));
        CHECK(map.upper_bound(50) == NTH(map, 2));
        CHECK(map.upper_bound(60) == NTH(map, 3));
        CHECK(map.upper_bound(70) == NTH(map, 3));

        ///////////////////////////////////////////////////////////////////////

        CHECK(map.equal_range(10) == std::make_pair(NTH(map, 0), NTH(map, 0)));
        CHECK(map.equal_range(20) == std::make_pair(NTH(map, 0), NTH(map, 1)));
        CHECK(map.equal_range(30) == std::make_pair(NTH(map, 1), NTH(map, 1)));
        CHECK(map.equal_range(40) == std::make_pair(NTH(map, 1), NTH(map, 2)));
        CHECK(map.equal_range(50) == std::make_pair(NTH(map, 2), NTH(map, 2)));
        CHECK(map.equal_range(60) == std::make_pair(NTH(map, 2), NTH(map, 3)));
        CHECK(map.equal_range(70) == std::make_pair(NTH(map, 3), NTH(map, 3)));

        ///////////////////////////////////////////////////////////////////////

        CHECK(map.find(10) == map.end());
        CHECK(map.find(20) == NTH(map, 0));
        CHECK(map.find(30) == map.end());
        CHECK(map.find(40) == NTH(map, 1));
        CHECK(map.find(50) == map.end());
        CHECK(map.find(60) == NTH(map, 2));
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
        sfl::multimap<xobj, xint, xobj::less, TPARAM_ALLOCATOR<std::pair<const xobj, xint>> > map;

        map.emplace(std::piecewise_construct, std::forward_as_tuple(20), std::forward_as_tuple(1));
        map.emplace(std::piecewise_construct, std::forward_as_tuple(40), std::forward_as_tuple(1));
        map.emplace(std::piecewise_construct, std::forward_as_tuple(60), std::forward_as_tuple(1));

        ///////////////////////////////////////////////////////////////////////

        CHECK(map.lower_bound(10) == NTH(map, 0));
        CHECK(map.lower_bound(20) == NTH(map, 0));
        CHECK(map.lower_bound(30) == NTH(map, 1));
        CHECK(map.lower_bound(40) == NTH(map, 1));
        CHECK(map.lower_bound(50) == NTH(map, 2));
        CHECK(map.lower_bound(60) == NTH(map, 2));
        CHECK(map.lower_bound(70) == NTH(map, 3));

        ///////////////////////////////////////////////////////////////////////

        CHECK(map.upper_bound(10) == NTH(map, 0));
        CHECK(map.upper_bound(20) == NTH(map, 1));
        CHECK(map.upper_bound(30) == NTH(map, 1));
        CHECK(map.upper_bound(40) == NTH(map, 2));
        CHECK(map.upper_bound(50) == NTH(map, 2));
        CHECK(map.upper_bound(60) == NTH(map, 3));
        CHECK(map.upper_bound(70) == NTH(map, 3));

        ///////////////////////////////////////////////////////////////////////

        CHECK(map.equal_range(10) == std::make_pair(NTH(map, 0), NTH(map, 0)));
        CHECK(map.equal_range(20) == std::make_pair(NTH(map, 0), NTH(map, 1)));
        CHECK(map.equal_range(30) == std::make_pair(NTH(map, 1), NTH(map, 1)));
        CHECK(map.equal_range(40) == std::make_pair(NTH(map, 1), NTH(map, 2)));
        CHECK(map.equal_range(50) == std::make_pair(NTH(map, 2), NTH(map, 2)));
        CHECK(map.equal_range(60) == std::make_pair(NTH(map, 2), NTH(map, 3)));
        CHECK(map.equal_range(70) == std::make_pair(NTH(map, 3), NTH(map, 3)));

        ///////////////////////////////////////////////////////////////////////

        CHECK(map.find(10) == map.end());
        CHECK(map.find(20) == NTH(map, 0));
        CHECK(map.find(30) == map.end());
        CHECK(map.find(40) == NTH(map, 1));
        CHECK(map.find(50) == map.end());
        CHECK(map.find(60) == NTH(map, 2));
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

PRINT("Test emplace(Args&&...)");
{
    sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map;

    CHECK(map.size() == 0);

    {
        const auto res = map.emplace(10, 1);

        CHECK(res->first == 10); CHECK(res->second == 1);

        CHECK(map.size() == 1);
    }

    {
        const auto res = map.emplace(10, 2);

        CHECK(res->first == 10); CHECK(res->second == 2);

        CHECK(map.size() == 2);
    }
}

PRINT("Test emplace_hint(const_iterator, Args&&...)");
{
    sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map;

    CHECK(map.size() == 0);

    {
        const auto res = map.emplace_hint(map.begin(), 10, 1);

        CHECK(res->first == 10); CHECK(res->second == 1);

        CHECK(map.size() == 1);
    }

    {
        const auto res = map.emplace_hint(map.begin(), 10, 2);

        CHECK(res->first == 10); CHECK(res->second == 2);

        CHECK(map.size() == 2);
    }
}

PRINT("Test insert(const value_type&)");
{
    sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map;

    CHECK(map.size() == 0);

    using value_type = std::pair<const xint, xint>;

    {
        const value_type value_10_1(10, 1);

        const auto res = map.insert(value_10_1);

        CHECK(res->first == 10); CHECK(res->second == 1);

        CHECK(map.size() == 1);
    }

    {
        const value_type value_10_2(10, 2);

        const auto res = map.insert(value_10_2);

        CHECK(res->first == 10); CHECK(res->second == 2);

        CHECK(map.size() == 2);
    }
}

PRINT("Test insert(value_type&&)");
{
    sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map;

    CHECK(map.size() == 0);

    using value_type = std::pair<const xint, xint>;

    {
        value_type value_10_1(10, 1);

        const auto res = map.insert(std::move(value_10_1));

        CHECK(res->first == 10); CHECK(res->second == 1);

        CHECK(map.size() == 1);

        CHECK(value_10_1.first  == +10);
        CHECK(value_10_1.second == -1);
    }

    {
        value_type value_10_2(10, 2);

        const auto res = map.insert(std::move(value_10_2));

        CHECK(res->first == 10); CHECK(res->second == 2);

        CHECK(map.size() == 2);

        CHECK(value_10_2.first  == +10);
        CHECK(value_10_2.second == -2);
    }
}

PRINT("Test insert(P&&)");
{
    sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map;

    CHECK(map.size() == 0);

    using value_type = std::pair<long, long>;

    {
        value_type value_10_1(10, 1);

        const auto res = map.insert(value_10_1);

        CHECK(res->first == 10); CHECK(res->second == 1);

        CHECK(map.size() == 1);
    }

    {
        value_type value_10_2(10, 2);

        const auto res = map.insert(value_10_2);

        CHECK(res->first == 10); CHECK(res->second == 2);

        CHECK(map.size() == 2);
    }
}

PRINT("Test insert(const_iterator, const value_type&)");
{
    sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map;

    CHECK(map.size() == 0);

    using value_type = std::pair<const xint, xint>;

    {
        const value_type value_10_1(10, 1);

        const auto res = map.insert(map.begin(), value_10_1);

        CHECK(res->first == 10); CHECK(res->second == 1);

        CHECK(map.size() == 1);
    }

    {
        const value_type value_10_2(10, 2);

        const auto res = map.insert(map.begin(), value_10_2);

        CHECK(res->first == 10); CHECK(res->second == 2);

        CHECK(map.size() == 2);
    }
}

PRINT("Test insert(const_iterator, value_type&&)");
{
    sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map;

    CHECK(map.size() == 0);

    using value_type = std::pair<const xint, xint>;

    {
        value_type value_10_1(10, 1);

        const auto res = map.insert(map.begin(), std::move(value_10_1));

        CHECK(res->first == 10); CHECK(res->second == 1);

        CHECK(map.size() == 1);

        CHECK(value_10_1.first  == +10);
        CHECK(value_10_1.second == -1);
    }

    {
        value_type value_10_2(10, 2);

        const auto res = map.insert(map.begin(), std::move(value_10_2));

        CHECK(res->first == 10); CHECK(res->second == 2);

        CHECK(map.size() == 2);

        CHECK(value_10_2.first  == +10);
        CHECK(value_10_2.second == -2);
    }
}

PRINT("Test insert(const_iterator, P&&)");
{
    sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map;

    CHECK(map.size() == 0);

    using value_type = std::pair<long, long>;

    {
        value_type value_10_1(10, 1);

        const auto res = map.insert(map.begin(), value_10_1);

        CHECK(res->first == 10); CHECK(res->second == 1);

        CHECK(map.size() == 1);
    }

    {
        value_type value_10_2(10, 2);

        const auto res = map.insert(map.begin(), value_10_2);

        CHECK(res->first == 10); CHECK(res->second == 2);

        CHECK(map.size() == 2);
    }
}

PRINT("Test insert(InputIt, InputIt");
{
    sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map;

    CHECK(map.size() == 0);

    std::vector<std::pair<const xint, xint>> data
    (
        {
            {10, 1},
            {20, 1},
            {30, 1},
            {20, 2},
            {20, 3}
        }
    );

    map.insert(data.begin(), data.end());

    CHECK(map.size() == 5);
    CHECK(NTH(map, 0)->first == 10); CHECK(NTH(map, 0)->second == 1);
    CHECK(NTH(map, 1)->first == 20); CHECK(NTH(map, 1)->second == 1);
    CHECK(NTH(map, 2)->first == 20); CHECK(NTH(map, 2)->second == 2);
    CHECK(NTH(map, 3)->first == 20); CHECK(NTH(map, 3)->second == 3);
    CHECK(NTH(map, 4)->first == 30); CHECK(NTH(map, 4)->second == 1);
}

PRINT("Test insert(std::initializer_list");
{
    sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map;

    CHECK(map.size() == 0);

    std::initializer_list<std::pair<const xint, xint>> ilist
    {
        {10, 1},
        {20, 1},
        {30, 1},
        {20, 2},
        {20, 3}
    };

    map.insert(ilist);

    CHECK(map.size() == 5);
    CHECK(NTH(map, 0)->first == 10); CHECK(NTH(map, 0)->second == 1);
    CHECK(NTH(map, 1)->first == 20); CHECK(NTH(map, 1)->second == 1);
    CHECK(NTH(map, 2)->first == 20); CHECK(NTH(map, 2)->second == 2);
    CHECK(NTH(map, 3)->first == 20); CHECK(NTH(map, 3)->second == 3);
    CHECK(NTH(map, 4)->first == 30); CHECK(NTH(map, 4)->second == 1);
}

PRINT("Test insert_range(Range&&");
{
    // Input iterator (exactly)
    {
        std::istringstream iss("10 1 20 1 30 1 20 2 20 3");

        sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map;

        #if SFL_CPP_VERSION >= SFL_CPP_20
        map.insert_range(std::views::istream<std::pair<int, int>>(iss));
        #else
        map.insert_range(sfl::test::istream_view<std::pair<int, int>>(iss));
        #endif

        CHECK(map.size() == 5);
        CHECK(NTH(map, 0)->first == 10); CHECK(NTH(map, 0)->second == 1);
        CHECK(NTH(map, 1)->first == 20); CHECK(NTH(map, 1)->second == 1);
        CHECK(NTH(map, 2)->first == 20); CHECK(NTH(map, 2)->second == 2);
        CHECK(NTH(map, 3)->first == 20); CHECK(NTH(map, 3)->second == 3);
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

        sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map;

        #if SFL_CPP_VERSION >= SFL_CPP_20
        map.insert_range(std::views::all(data));
        #else
        map.insert_range(data);
        #endif

        CHECK(map.size() == 5);
        CHECK(NTH(map, 0)->first == 10); CHECK(NTH(map, 0)->second == 1);
        CHECK(NTH(map, 1)->first == 20); CHECK(NTH(map, 1)->second == 1);
        CHECK(NTH(map, 2)->first == 20); CHECK(NTH(map, 2)->second == 2);
        CHECK(NTH(map, 3)->first == 20); CHECK(NTH(map, 3)->second == 3);
        CHECK(NTH(map, 4)->first == 30); CHECK(NTH(map, 4)->second == 1);
    }
}

PRINT("Test erase(const_iterator)");
{
    sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map;

    map.emplace(10, 1);
    map.emplace(20, 1);
    map.emplace(30, 1);

    CHECK(map.size() == 3);
    CHECK(NTH(map, 0)->first == 10); CHECK(NTH(map, 0)->second == 1);
    CHECK(NTH(map, 1)->first == 20); CHECK(NTH(map, 1)->second == 1);
    CHECK(NTH(map, 2)->first == 30); CHECK(NTH(map, 2)->second == 1);

    {
        const auto res = map.erase(map.begin());

        CHECK(res == map.begin());
        CHECK(map.size() == 2);
        CHECK(NTH(map, 0)->first == 20); CHECK(NTH(map, 0)->second == 1);
        CHECK(NTH(map, 1)->first == 30); CHECK(NTH(map, 1)->second == 1);
    }

    {
        const auto res = map.erase(map.begin());

        CHECK(res == map.begin());
        CHECK(map.size() == 1);
        CHECK(NTH(map, 0)->first == 30); CHECK(NTH(map, 0)->second == 1);
    }

    {
        const auto res = map.erase(map.begin());

        CHECK(res == map.end());
        CHECK(map.size() == 0);
    }
}

PRINT("Test erase(const_iterator, const_iterator)");
{
    sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map;

    map.emplace(10, 1);
    map.emplace(20, 1);
    map.emplace(30, 1);

    CHECK(map.size() == 3);
    CHECK(NTH(map, 0)->first == 10); CHECK(NTH(map, 0)->second == 1);
    CHECK(NTH(map, 1)->first == 20); CHECK(NTH(map, 1)->second == 1);
    CHECK(NTH(map, 2)->first == 30); CHECK(NTH(map, 2)->second == 1);

    const auto res = map.erase(NTH(map, 1), NTH(map, 2));

    CHECK(res == NTH(map, 1));
    CHECK(map.size() == 2);
    CHECK(NTH(map, 0)->first == 10); CHECK(NTH(map, 0)->second == 1);
    CHECK(NTH(map, 1)->first == 30); CHECK(NTH(map, 0)->second == 1);
}

PRINT("Test erase(const Key&)");
{
    sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map;

    map.emplace(10, 1);
    map.emplace(20, 1);
    map.emplace(30, 1);
    map.emplace(20, 2);
    map.emplace(20, 3);

    CHECK(map.size() == 5);
    CHECK(NTH(map, 0)->first == 10); CHECK(NTH(map, 0)->second == 1);
    CHECK(NTH(map, 1)->first == 20); CHECK(NTH(map, 1)->second == 1);
    CHECK(NTH(map, 2)->first == 20); CHECK(NTH(map, 2)->second == 2);
    CHECK(NTH(map, 3)->first == 20); CHECK(NTH(map, 3)->second == 3);
    CHECK(NTH(map, 4)->first == 30); CHECK(NTH(map, 4)->second == 1);

    CHECK(map.erase(30) == 1);
    CHECK(map.erase(30) == 0);
    CHECK(map.size() == 4);
    CHECK(NTH(map, 0)->first == 10); CHECK(NTH(map, 0)->second == 1);
    CHECK(NTH(map, 1)->first == 20); CHECK(NTH(map, 1)->second == 1);
    CHECK(NTH(map, 2)->first == 20); CHECK(NTH(map, 2)->second == 2);
    CHECK(NTH(map, 3)->first == 20); CHECK(NTH(map, 3)->second == 3);

    CHECK(map.erase(20) == 3);
    CHECK(map.erase(20) == 0);
    CHECK(map.size() == 1);
    CHECK(NTH(map, 0)->first == 10); CHECK(NTH(map, 0)->second == 1);

    CHECK(map.erase(10) == 1);
    CHECK(map.erase(10) == 0);
    CHECK(map.size() == 0);
}

PRINT("Test erase(K&&)");
{
    sfl::multimap<xobj, xint, xobj::less, TPARAM_ALLOCATOR<std::pair<const xobj, xint>>> map;

    map.emplace(std::piecewise_construct, std::forward_as_tuple(10), std::forward_as_tuple(1));
    map.emplace(std::piecewise_construct, std::forward_as_tuple(20), std::forward_as_tuple(1));
    map.emplace(std::piecewise_construct, std::forward_as_tuple(30), std::forward_as_tuple(1));
    map.emplace(std::piecewise_construct, std::forward_as_tuple(20), std::forward_as_tuple(2));
    map.emplace(std::piecewise_construct, std::forward_as_tuple(20), std::forward_as_tuple(3));

    CHECK(map.size() == 5);
    CHECK(NTH(map, 0)->first.value() == 10); CHECK(NTH(map, 0)->second == 1);
    CHECK(NTH(map, 1)->first.value() == 20); CHECK(NTH(map, 1)->second == 1);
    CHECK(NTH(map, 2)->first.value() == 20); CHECK(NTH(map, 2)->second == 2);
    CHECK(NTH(map, 3)->first.value() == 20); CHECK(NTH(map, 3)->second == 3);
    CHECK(NTH(map, 4)->first.value() == 30); CHECK(NTH(map, 4)->second == 1);

    CHECK(map.erase(30) == 1);
    CHECK(map.erase(30) == 0);
    CHECK(map.size() == 4);
    CHECK(NTH(map, 0)->first.value() == 10); CHECK(NTH(map, 0)->second == 1);
    CHECK(NTH(map, 1)->first.value() == 20); CHECK(NTH(map, 1)->second == 1);
    CHECK(NTH(map, 2)->first.value() == 20); CHECK(NTH(map, 2)->second == 2);
    CHECK(NTH(map, 3)->first.value() == 20); CHECK(NTH(map, 3)->second == 3);

    CHECK(map.erase(20) == 3);
    CHECK(map.erase(20) == 0);
    CHECK(map.size() == 1);
    CHECK(NTH(map, 0)->first.value() == 10); CHECK(NTH(map, 0)->second == 1);

    CHECK(map.erase(10) == 1);
    CHECK(map.erase(10) == 0);
    CHECK(map.size() == 0);
}

PRINT("Test swap(container&)");
{
    sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map1, map2;

    map1.emplace(10, 1);
    map1.emplace(20, 1);
    map1.emplace(30, 1);

    map2.emplace(40, 2);
    map2.emplace(50, 2);
    map2.emplace(60, 2);
    map2.emplace(70, 2);

    CHECK(map1.size() == 3);
    CHECK(NTH(map1, 0)->first == 10); CHECK(NTH(map1, 0)->second == 1);
    CHECK(NTH(map1, 1)->first == 20); CHECK(NTH(map1, 1)->second == 1);
    CHECK(NTH(map1, 2)->first == 30); CHECK(NTH(map1, 2)->second == 1);

    CHECK(map2.size() == 4);
    CHECK(NTH(map2, 0)->first == 40); CHECK(NTH(map2, 0)->second == 2);
    CHECK(NTH(map2, 1)->first == 50); CHECK(NTH(map2, 1)->second == 2);
    CHECK(NTH(map2, 2)->first == 60); CHECK(NTH(map2, 2)->second == 2);
    CHECK(NTH(map2, 3)->first == 70); CHECK(NTH(map2, 3)->second == 2);

    ///////////////////////////////////////////////////////////////////////

    map1.swap(map2);

    CHECK(map1.size() == 4);
    CHECK(NTH(map1, 0)->first == 40); CHECK(NTH(map1, 0)->second == 2);
    CHECK(NTH(map1, 1)->first == 50); CHECK(NTH(map1, 1)->second == 2);
    CHECK(NTH(map1, 2)->first == 60); CHECK(NTH(map1, 2)->second == 2);
    CHECK(NTH(map1, 3)->first == 70); CHECK(NTH(map1, 3)->second == 2);

    CHECK(map2.size() == 3);
    CHECK(NTH(map2, 0)->first == 10); CHECK(NTH(map2, 0)->second == 1);
    CHECK(NTH(map2, 1)->first == 20); CHECK(NTH(map2, 1)->second == 1);
    CHECK(NTH(map2, 2)->first == 30); CHECK(NTH(map2, 2)->second == 1);

    ///////////////////////////////////////////////////////////////////////

    map1.swap(map2);

    CHECK(map1.size() == 3);
    CHECK(NTH(map1, 0)->first == 10); CHECK(NTH(map1, 0)->second == 1);
    CHECK(NTH(map1, 1)->first == 20); CHECK(NTH(map1, 1)->second == 1);
    CHECK(NTH(map1, 2)->first == 30); CHECK(NTH(map1, 2)->second == 1);

    CHECK(map2.size() == 4);
    CHECK(NTH(map2, 0)->first == 40); CHECK(NTH(map2, 0)->second == 2);
    CHECK(NTH(map2, 1)->first == 50); CHECK(NTH(map2, 1)->second == 2);
    CHECK(NTH(map2, 2)->first == 60); CHECK(NTH(map2, 2)->second == 2);
    CHECK(NTH(map2, 3)->first == 70); CHECK(NTH(map2, 3)->second == 2);
}

///////////////////////////////////////////////////////////////////////////////

PRINT("Test NON-MEMBER comparison operators");
{
    sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map1, map2;

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
    sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>> map1, map2;

    map1.emplace(10, 1);
    map1.emplace(20, 1);
    map1.emplace(30, 1);

    map2.emplace(40, 2);
    map2.emplace(50, 2);
    map2.emplace(60, 2);
    map2.emplace(70, 2);
    map2.emplace(80, 2);

    CHECK(map1.size() == 3);
    CHECK(NTH(map1, 0)->first == 10); CHECK(NTH(map1, 0)->second == 1);
    CHECK(NTH(map1, 1)->first == 20); CHECK(NTH(map1, 1)->second == 1);
    CHECK(NTH(map1, 2)->first == 30); CHECK(NTH(map1, 2)->second == 1);

    CHECK(map2.size() == 5);
    CHECK(NTH(map2, 0)->first == 40); CHECK(NTH(map2, 0)->second == 2);
    CHECK(NTH(map2, 1)->first == 50); CHECK(NTH(map2, 1)->second == 2);
    CHECK(NTH(map2, 2)->first == 60); CHECK(NTH(map2, 2)->second == 2);
    CHECK(NTH(map2, 3)->first == 70); CHECK(NTH(map2, 3)->second == 2);
    CHECK(NTH(map2, 4)->first == 80); CHECK(NTH(map2, 4)->second == 2);

    ///////////////////////////////////////////////////////////////////////////

    swap(map1, map2);

    CHECK(map1.size() == 5);
    CHECK(NTH(map1, 0)->first == 40); CHECK(NTH(map1, 0)->second == 2);
    CHECK(NTH(map1, 1)->first == 50); CHECK(NTH(map1, 1)->second == 2);
    CHECK(NTH(map1, 2)->first == 60); CHECK(NTH(map1, 2)->second == 2);
    CHECK(NTH(map1, 3)->first == 70); CHECK(NTH(map1, 3)->second == 2);
    CHECK(NTH(map1, 4)->first == 80); CHECK(NTH(map1, 4)->second == 2);

    CHECK(map2.size() == 3);
    CHECK(NTH(map2, 0)->first == 10); CHECK(NTH(map2, 0)->second == 1);
    CHECK(NTH(map2, 1)->first == 20); CHECK(NTH(map2, 1)->second == 1);
    CHECK(NTH(map2, 2)->first == 30); CHECK(NTH(map2, 2)->second == 1);
}

PRINT("Test NON-MEMBER erase_if(container&, Predicate)");
{
    using container_type =
        sfl::multimap<xint, xint, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>>;

    using const_reference = typename container_type::const_reference;

    ///////////////////////////////////////////////////////////////////////////

    container_type map;

    map.emplace(10, 1);
    map.emplace(20, 1);
    map.emplace(20, 2);
    map.emplace(20, 3);
    map.emplace(30, 1);

    CHECK(map.size() == 5);
    CHECK(NTH(map, 0)->first == 10); CHECK(NTH(map, 0)->second == 1);
    CHECK(NTH(map, 1)->first == 20); CHECK(NTH(map, 1)->second == 1);
    CHECK(NTH(map, 2)->first == 20); CHECK(NTH(map, 2)->second == 2);
    CHECK(NTH(map, 3)->first == 20); CHECK(NTH(map, 3)->second == 3);
    CHECK(NTH(map, 4)->first == 30); CHECK(NTH(map, 4)->second == 1);

    ///////////////////////////////////////////////////////////////////////////

    CHECK(erase_if(map, [](const_reference& value){ return value.first == 20; }) == 3);
    CHECK(map.size() == 2);
    CHECK(NTH(map, 0)->first == 10); CHECK(NTH(map, 0)->second == 1);
    CHECK(NTH(map, 1)->first == 30); CHECK(NTH(map, 1)->second == 1);

    ///////////////////////////////////////////////////////////////////////////

    CHECK(erase_if(map, [](const_reference& value){ return value.first == 20; }) == 0);
    CHECK(map.size() == 2);
    CHECK(NTH(map, 0)->first == 10); CHECK(NTH(map, 0)->second == 1);
    CHECK(NTH(map, 1)->first == 30); CHECK(NTH(map, 1)->second == 1);
}
