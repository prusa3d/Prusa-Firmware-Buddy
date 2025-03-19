#ifndef TPARAM_ALLOCATOR
#error Please define macro TPARAM_ALLOCATOR
#endif

using sfl::test::xint;
using sfl::test::xobj;

PRINT("Test PRIVATE make_node and drop_node");
{
    using tree_type = sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void>;
    using node_pointer = typename tree_type::node_pointer;

    tree_type tree;

    tree_type::make_node_functor make_node(tree);

    node_pointer p1 = make_node(1);
    node_pointer p2 = make_node(2);
    node_pointer p3 = make_node(3);

    CHECK(p1->value_ == 1);
    CHECK(p2->value_ == 2);
    CHECK(p3->value_ == 3);

    tree.drop_node(p3);
    tree.drop_node(p2);
    tree.drop_node(p1);
}

PRINT("Test PRIVATE underlying implementation [empty tree]");
{
    using tree_type = sfl::dtl::rb_tree<int, int, sfl::dtl::identity, std::less<int>, TPARAM_ALLOCATOR<int>, void>;

    tree_type tree;

    CHECK(tree.minimum(tree.data_.header()) == tree.data_.header());

    CHECK(tree.maximum(tree.data_.header()) == tree.data_.header());

    (void)tree.next(tree.data_.header()); // Undefined behavior, but should compile without errors
                                          // and shouldn't be an infinite loop.

    (void)tree.prev(tree.data_.header()); // Undefined behavior, but should compile without errors
                                          // and shouldn't be an infinite loop.

    CHECK(tree.begin() == tree.end());
    CHECK(tree.begin() == tree.cbegin());
    CHECK(tree.begin() == tree.cend());
    CHECK(tree.cbegin() == tree.end());

    auto it1 = tree.begin();

    (void)++it1; // Undefined behavior, but should compile without errors
                 // and shouldn't be an infinite loop.

    auto it2 = tree.begin();

    (void)--it2; // Undefined behavior, but should compile without errors
                 // and shouldn't be an infinite loop.

    auto cit1 = tree.cbegin();

    (void)++cit1; // Undefined behavior, but should compile without errors
                  // and shouldn't be an infinite loop.

    auto cit2 = tree.cbegin();

    (void)--cit2; // Undefined behavior, but should compile without errors
                  // and shouldn't be an infinite loop.

    CHECK(tree.rbegin() == tree.rend());
    CHECK(tree.rbegin() == tree.crbegin());
    CHECK(tree.rbegin() == tree.crend());
    CHECK(tree.crbegin() == tree.rend());

    auto rit1 = tree.rbegin();

    (void)++rit1; // Undefined behavior, but should compile without errors
                  // and shouldn't be an infinite loop.

    auto rit2 = tree.rbegin();

    (void)--rit2; // Undefined behavior, but should compile without errors
                  // and shouldn't be an infinite loop.

    auto crit1 = tree.crbegin();

    (void)++crit1; // Undefined behavior, but should compile without errors
                   // and shouldn't be an infinite loop.

    auto crit2 = tree.crbegin();

    (void)--crit2; // Undefined behavior, but should compile without errors
                   // and shouldn't be an infinite loop.
}

PRINT("Test PRIVATE calculate_position_for_insert_equal(const K&) [empty tree]");
{
    using tree_type = sfl::dtl::rb_tree<int, int, sfl::dtl::identity, std::less<int>, TPARAM_ALLOCATOR<int>, void>;

    tree_type tree;

    auto res = tree.calculate_position_for_insert_equal(42);

    CHECK(res.pos == tree.data_.header());
    CHECK(res.left == true);
}

PRINT("Test PRIVATE calculate_position_for_insert_unique(const K&) [empty tree]");
{
    using tree_type = sfl::dtl::rb_tree<int, int, sfl::dtl::identity, std::less<int>, TPARAM_ALLOCATOR<int>, void>;

    tree_type tree;

    auto res = tree.calculate_position_for_insert_unique(42);

    CHECK(res.pos == tree.data_.header());
    CHECK(res.left == true);
    CHECK(res.status == true);
}

PRINT("Test PRIVATE calculate_position_for_insert_hint_equal(const_iterator, const K&) [empty tree]");
{
    using tree_type = sfl::dtl::rb_tree<int, int, sfl::dtl::identity, std::less<int>, TPARAM_ALLOCATOR<int>, void>;

    tree_type tree;

    auto res = tree.calculate_position_for_insert_hint_equal(tree.end(), 42);

    CHECK(res.pos == tree.data_.header());
    CHECK(res.left == true);
}

PRINT("Test PRIVATE calculate_position_for_insert_hint_unique(const_iterator, const K&) [empty tree]");
{
    using tree_type = sfl::dtl::rb_tree<int, int, sfl::dtl::identity, std::less<int>, TPARAM_ALLOCATOR<int>, void>;

    tree_type tree;

    auto res = tree.calculate_position_for_insert_hint_unique(tree.end(), 42);

    CHECK(res.pos == tree.data_.header());
    CHECK(res.left == true);
    CHECK(res.status == true);
}

PRINT("Test PRIVATE underlying implementation [non-empty tree]");
{
    /*
     *                   (pointer to minumum value, i.e. 1)
     *                   |
     *                   H  (header)
     *                  /
     *          ______ 8 ______
     *         /               \
     *      _ 4 _            __ 12 __
     *     /     \          /        \
     *    2       5        11       _ 15 _
     *   / \       \      /        /      \
     *  1   3       7    9        13       17
     *             /      \        \      /
     *            6        10       14   16
     */

    using tree_type = sfl::dtl::rb_tree<int, int, sfl::dtl::identity, std::less<int>, TPARAM_ALLOCATOR<int>, void>;
    using node_type = typename tree_type::node_type;

    tree_type tree;
    node_type n1, n2, n3, n4, n5, n6, n7, n8, n9, n10, n11, n12, n13, n14, n15, n16, n17;

    tree.data_.root() = &n8;
    tree.data_.minimum() = &n1;
    tree.data_.size_ = 17;

    n1.left_  = nullptr;    n1.right_  = nullptr;   n1.parent_  = &n2;
    n2.left_  = &n1;        n2.right_  = &n3;       n2.parent_  = &n4;
    n3.left_  = nullptr;    n3.right_  = nullptr;   n3.parent_  = &n2;
    n4.left_  = &n2;        n4.right_  = &n5;       n4.parent_  = &n8;
    n5.left_  = nullptr;    n5.right_  = &n7;       n5.parent_  = &n4;
    n6.left_  = nullptr;    n6.right_  = nullptr;   n6.parent_  = &n7;
    n7.left_  = &n6;        n7.right_  = nullptr;   n7.parent_  = &n5;
    n8.left_  = &n4;        n8.right_  = &n12;      n8.parent_  = tree.data_.header();
    n9.left_  = nullptr;    n9.right_  = &n10;      n9.parent_  = &n11;
    n10.left_ = nullptr;    n10.right_ = nullptr;   n10.parent_ = &n9;
    n11.left_ = &n9;        n11.right_ = nullptr;   n11.parent_ = &n12;
    n12.left_ = &n11;       n12.right_ = &n15;      n12.parent_ = &n8;
    n13.left_ = nullptr;    n13.right_ = &n14;      n13.parent_ = &n15;
    n14.left_ = nullptr;    n14.right_ = nullptr;   n14.parent_ = &n13;
    n15.left_ = &n13;       n15.right_ = &n17;      n15.parent_ = &n12;
    n16.left_ = nullptr;    n16.right_ = nullptr;   n16.parent_ = &n17;
    n17.left_ = &n16;       n17.right_ = nullptr;   n17.parent_ = &n15;

    n1.value_  = 1;
    n2.value_  = 2;
    n3.value_  = 3;
    n4.value_  = 4;
    n5.value_  = 5;
    n6.value_  = 6;
    n7.value_  = 7;
    n8.value_  = 8;
    n9.value_  = 9;
    n10.value_ = 10;
    n11.value_ = 11;
    n12.value_ = 12;
    n13.value_ = 13;
    n14.value_ = 14;
    n15.value_ = 15;
    n16.value_ = 16;
    n17.value_ = 17;

    CHECK(tree.minimum(tree.data_.header()) == &n1);
    CHECK(tree.minimum(&n1) == &n1);
    CHECK(tree.minimum(&n2) == &n1);
    CHECK(tree.minimum(&n3) == &n3);
    CHECK(tree.minimum(&n4) == &n1);
    CHECK(tree.minimum(&n5) == &n5);
    CHECK(tree.minimum(&n6) == &n6);
    CHECK(tree.minimum(&n7) == &n6);
    CHECK(tree.minimum(&n8) == &n1);
    CHECK(tree.minimum(&n9) == &n9);
    CHECK(tree.minimum(&n10) == &n10);
    CHECK(tree.minimum(&n11) == &n9);
    CHECK(tree.minimum(&n12) == &n9);
    CHECK(tree.minimum(&n13) == &n13);
    CHECK(tree.minimum(&n14) == &n14);
    CHECK(tree.minimum(&n15) == &n13);
    CHECK(tree.minimum(&n16) == &n16);
    CHECK(tree.minimum(&n17) == &n16);

    (void)tree.maximum(tree.data_.header()); // Undefined behavior, but should compile without errors
                                             // and shouldn't be an infinite loop.
    CHECK(tree.maximum(&n1) == &n1);
    CHECK(tree.maximum(&n2) == &n3);
    CHECK(tree.maximum(&n3) == &n3);
    CHECK(tree.maximum(&n4) == &n7);
    CHECK(tree.maximum(&n5) == &n7);
    CHECK(tree.maximum(&n6) == &n6);
    CHECK(tree.maximum(&n7) == &n7);
    CHECK(tree.maximum(&n8) == &n17);
    CHECK(tree.maximum(&n9) == &n10);
    CHECK(tree.maximum(&n10) == &n10);
    CHECK(tree.maximum(&n11) == &n11);
    CHECK(tree.maximum(&n12) == &n17);
    CHECK(tree.maximum(&n13) == &n14);
    CHECK(tree.maximum(&n14) == &n14);
    CHECK(tree.maximum(&n15) == &n17);
    CHECK(tree.maximum(&n16) == &n16);
    CHECK(tree.maximum(&n17) == &n17);

    (void)tree.next(tree.data_.header()); // Undefined behavior, but should compile without errors
                                          // and shouldn't be an infinite loop.
    CHECK(tree.next(&n1) == &n2);
    CHECK(tree.next(&n2) == &n3);
    CHECK(tree.next(&n3) == &n4);
    CHECK(tree.next(&n4) == &n5);
    CHECK(tree.next(&n5) == &n6);
    CHECK(tree.next(&n6) == &n7);
    CHECK(tree.next(&n7) == &n8);
    CHECK(tree.next(&n8) == &n9);
    CHECK(tree.next(&n9) == &n10);
    CHECK(tree.next(&n10) == &n11);
    CHECK(tree.next(&n11) == &n12);
    CHECK(tree.next(&n12) == &n13);
    CHECK(tree.next(&n13) == &n14);
    CHECK(tree.next(&n14) == &n15);
    CHECK(tree.next(&n15) == &n16);
    CHECK(tree.next(&n16) == &n17);
    CHECK(tree.next(&n17) == tree.data_.header());

    (void)tree.prev(tree.data_.header()); // Undefined behavior, but should compile without errors
                                          // and shouldn't be an infinite loop.
    (void)tree.prev(&n1);                 // Undefined behavior, but should compile without errors
                                          // and shouldn't be an infinite loop.
    CHECK(tree.prev(&n2) == &n1);
    CHECK(tree.prev(&n3) == &n2);
    CHECK(tree.prev(&n4) == &n3);
    CHECK(tree.prev(&n5) == &n4);
    CHECK(tree.prev(&n6) == &n5);
    CHECK(tree.prev(&n7) == &n6);
    CHECK(tree.prev(&n8) == &n7);
    CHECK(tree.prev(&n9) == &n8);
    CHECK(tree.prev(&n10) == &n9);
    CHECK(tree.prev(&n11) == &n10);
    CHECK(tree.prev(&n12) == &n11);
    CHECK(tree.prev(&n13) == &n12);
    CHECK(tree.prev(&n14) == &n13);
    CHECK(tree.prev(&n15) == &n14);
    CHECK(tree.prev(&n16) == &n15);
    CHECK(tree.prev(&n17) == &n16);

    auto it1 = tree.begin();
    CHECK(*it1 == 1); ++it1;
    CHECK(*it1 == 2); ++it1;
    CHECK(*it1 == 3); ++it1;
    CHECK(*it1 == 4); ++it1;
    CHECK(*it1 == 5); ++it1;
    CHECK(*it1 == 6); ++it1;
    CHECK(*it1 == 7); ++it1;
    CHECK(*it1 == 8); ++it1;
    CHECK(*it1 == 9); ++it1;
    CHECK(*it1 == 10); ++it1;
    CHECK(*it1 == 11); ++it1;
    CHECK(*it1 == 12); ++it1;
    CHECK(*it1 == 13); ++it1;
    CHECK(*it1 == 14); ++it1;
    CHECK(*it1 == 15); ++it1;
    CHECK(*it1 == 16); ++it1;
    CHECK(*it1 == 17); ++it1;
    CHECK(it1 == tree.end());
    (void)++it1; // Undefined behavior, but should compile without errors
                 // and shouldn't be an infinite loop.

    auto it2 = tree.end();
    --it2; CHECK(*it2 == 17);
    --it2; CHECK(*it2 == 16);
    --it2; CHECK(*it2 == 15);
    --it2; CHECK(*it2 == 14);
    --it2; CHECK(*it2 == 13);
    --it2; CHECK(*it2 == 12);
    --it2; CHECK(*it2 == 11);
    --it2; CHECK(*it2 == 10);
    --it2; CHECK(*it2 == 9);
    --it2; CHECK(*it2 == 8);
    --it2; CHECK(*it2 == 7);
    --it2; CHECK(*it2 == 6);
    --it2; CHECK(*it2 == 5);
    --it2; CHECK(*it2 == 4);
    --it2; CHECK(*it2 == 3);
    --it2; CHECK(*it2 == 2);
    --it2; CHECK(*it2 == 1);
    CHECK(it2 == tree.begin());
    (void)--it2; // Undefined behavior, but should compile without errors
                 // and shouldn't be an infinite loop.

    auto cit1 = tree.cbegin();
    CHECK(*cit1 == 1); ++cit1;
    CHECK(*cit1 == 2); ++cit1;
    CHECK(*cit1 == 3); ++cit1;
    CHECK(*cit1 == 4); ++cit1;
    CHECK(*cit1 == 5); ++cit1;
    CHECK(*cit1 == 6); ++cit1;
    CHECK(*cit1 == 7); ++cit1;
    CHECK(*cit1 == 8); ++cit1;
    CHECK(*cit1 == 9); ++cit1;
    CHECK(*cit1 == 10); ++cit1;
    CHECK(*cit1 == 11); ++cit1;
    CHECK(*cit1 == 12); ++cit1;
    CHECK(*cit1 == 13); ++cit1;
    CHECK(*cit1 == 14); ++cit1;
    CHECK(*cit1 == 15); ++cit1;
    CHECK(*cit1 == 16); ++cit1;
    CHECK(*cit1 == 17); ++cit1;
    CHECK(cit1 == tree.cend());
    (void)++cit1; // Undefined behavior, but should compile without errors
                  // and shouldn't be an infinite loop.

    auto cit2 = tree.cend();
    --cit2; CHECK(*cit2 == 17);
    --cit2; CHECK(*cit2 == 16);
    --cit2; CHECK(*cit2 == 15);
    --cit2; CHECK(*cit2 == 14);
    --cit2; CHECK(*cit2 == 13);
    --cit2; CHECK(*cit2 == 12);
    --cit2; CHECK(*cit2 == 11);
    --cit2; CHECK(*cit2 == 10);
    --cit2; CHECK(*cit2 == 9);
    --cit2; CHECK(*cit2 == 8);
    --cit2; CHECK(*cit2 == 7);
    --cit2; CHECK(*cit2 == 6);
    --cit2; CHECK(*cit2 == 5);
    --cit2; CHECK(*cit2 == 4);
    --cit2; CHECK(*cit2 == 3);
    --cit2; CHECK(*cit2 == 2);
    --cit2; CHECK(*cit2 == 1);
    CHECK(cit2 == tree.cbegin());
    (void)--cit2; // Undefined behavior, but should compile without errors
                  // and shouldn't be an infinite loop.

    auto rit1 = tree.rbegin();
    CHECK(*rit1 == 17); ++rit1;
    CHECK(*rit1 == 16); ++rit1;
    CHECK(*rit1 == 15); ++rit1;
    CHECK(*rit1 == 14); ++rit1;
    CHECK(*rit1 == 13); ++rit1;
    CHECK(*rit1 == 12); ++rit1;
    CHECK(*rit1 == 11); ++rit1;
    CHECK(*rit1 == 10); ++rit1;
    CHECK(*rit1 == 9); ++rit1;
    CHECK(*rit1 == 8); ++rit1;
    CHECK(*rit1 == 7); ++rit1;
    CHECK(*rit1 == 6); ++rit1;
    CHECK(*rit1 == 5); ++rit1;
    CHECK(*rit1 == 4); ++rit1;
    CHECK(*rit1 == 3); ++rit1;
    CHECK(*rit1 == 2); ++rit1;
    CHECK(*rit1 == 1); ++rit1;
    CHECK(rit1 == tree.rend());
    (void)++rit1; // Undefined behavior, but should compile without errors
                  // and shouldn't be an infinite loop.

    auto rit2 = tree.rend();
    --rit2; CHECK(*rit2 == 1);
    --rit2; CHECK(*rit2 == 2);
    --rit2; CHECK(*rit2 == 3);
    --rit2; CHECK(*rit2 == 4);
    --rit2; CHECK(*rit2 == 5);
    --rit2; CHECK(*rit2 == 6);
    --rit2; CHECK(*rit2 == 7);
    --rit2; CHECK(*rit2 == 8);
    --rit2; CHECK(*rit2 == 9);
    --rit2; CHECK(*rit2 == 10);
    --rit2; CHECK(*rit2 == 11);
    --rit2; CHECK(*rit2 == 12);
    --rit2; CHECK(*rit2 == 13);
    --rit2; CHECK(*rit2 == 14);
    --rit2; CHECK(*rit2 == 15);
    --rit2; CHECK(*rit2 == 16);
    --rit2; CHECK(*rit2 == 17);
    CHECK(rit2 == tree.rbegin());
    (void)--rit2; // Undefined behavior, but should compile without errors
                  // and shouldn't be an infinite loop.

    auto crit1 = tree.crbegin();
    CHECK(*crit1 == 17); ++crit1;
    CHECK(*crit1 == 16); ++crit1;
    CHECK(*crit1 == 15); ++crit1;
    CHECK(*crit1 == 14); ++crit1;
    CHECK(*crit1 == 13); ++crit1;
    CHECK(*crit1 == 12); ++crit1;
    CHECK(*crit1 == 11); ++crit1;
    CHECK(*crit1 == 10); ++crit1;
    CHECK(*crit1 == 9); ++crit1;
    CHECK(*crit1 == 8); ++crit1;
    CHECK(*crit1 == 7); ++crit1;
    CHECK(*crit1 == 6); ++crit1;
    CHECK(*crit1 == 5); ++crit1;
    CHECK(*crit1 == 4); ++crit1;
    CHECK(*crit1 == 3); ++crit1;
    CHECK(*crit1 == 2); ++crit1;
    CHECK(*crit1 == 1); ++crit1;
    CHECK(crit1 == tree.crend());
    (void)++crit1; // Undefined behavior, but should compile without errors
                   // and shouldn't be an infinite loop.

    auto crit2 = tree.crend();
    --crit2; CHECK(*crit2 == 1);
    --crit2; CHECK(*crit2 == 2);
    --crit2; CHECK(*crit2 == 3);
    --crit2; CHECK(*crit2 == 4);
    --crit2; CHECK(*crit2 == 5);
    --crit2; CHECK(*crit2 == 6);
    --crit2; CHECK(*crit2 == 7);
    --crit2; CHECK(*crit2 == 8);
    --crit2; CHECK(*crit2 == 9);
    --crit2; CHECK(*crit2 == 10);
    --crit2; CHECK(*crit2 == 11);
    --crit2; CHECK(*crit2 == 12);
    --crit2; CHECK(*crit2 == 13);
    --crit2; CHECK(*crit2 == 14);
    --crit2; CHECK(*crit2 == 15);
    --crit2; CHECK(*crit2 == 16);
    --crit2; CHECK(*crit2 == 17);
    CHECK(crit2 == tree.crbegin());
    (void)--crit2; // Undefined behavior, but should compile without errors
                   // and shouldn't be an infinite loop.

    tree.data_.reset();
}

PRINT("Test PRIVATE calculate_position_for_insert_equal(const K&) [non-empty tree]");
{
    /*
     *                 (pointer to minumum value, i.e. 10)
     *                 |
     *                 H (header)
     *                /
     *           __ 40 __
     *          /        \
     *        20          50
     *       /  \
     *     10    30
     */

    using tree_type = sfl::dtl::rb_tree<int, int, sfl::dtl::identity, std::less<int>, TPARAM_ALLOCATOR<int>, void>;
    using node_type = typename tree_type::node_type;

    tree_type tree;
    node_type n10, n20, n30, n40, n50;

    tree.data_.root() = &n40;
    tree.data_.minimum() = &n10;
    tree.data_.size_ = 5;

    n10.left_  = nullptr;    n10.right_  = nullptr;   n10.parent_  = &n20;
    n20.left_  = &n10;       n20.right_  = &n30;      n20.parent_  = &n40;
    n30.left_  = nullptr;    n30.right_  = nullptr;   n30.parent_  = &n20;
    n40.left_  = &n20;       n40.right_  = &n50;      n40.parent_  = tree.data_.header();
    n50.left_  = nullptr;    n50.right_  = nullptr;   n50.parent_  = &n40;

    n10.value_ = 10;
    n20.value_ = 20;
    n30.value_ = 30;
    n40.value_ = 40;
    n50.value_ = 50;

    {
        auto res = tree.calculate_position_for_insert_equal(5);
        CHECK(res.pos == &n10);
        CHECK(res.left == true);
    }

    {
        auto res = tree.calculate_position_for_insert_equal(10);
        CHECK(res.pos == &n10);
        CHECK(res.left == false);
    }

    {
        auto res = tree.calculate_position_for_insert_equal(15);
        CHECK(res.pos == &n10);
        CHECK(res.left == false);
    }

    {
        auto res = tree.calculate_position_for_insert_equal(20);
        CHECK(res.pos == &n30);
        CHECK(res.left == true);
    }

    {
        auto res = tree.calculate_position_for_insert_equal(25);
        CHECK(res.pos == &n30);
        CHECK(res.left == true);
    }

    {
        auto res = tree.calculate_position_for_insert_equal(30);
        CHECK(res.pos == &n30);
        CHECK(res.left == false);
    }

    {
        auto res = tree.calculate_position_for_insert_equal(35);
        CHECK(res.pos == &n30);
        CHECK(res.left == false);
    }

    {
        auto res = tree.calculate_position_for_insert_equal(40);
        CHECK(res.pos == &n50);
        CHECK(res.left == true);
    }

    {
        auto res = tree.calculate_position_for_insert_equal(45);
        CHECK(res.pos == &n50);
        CHECK(res.left == true);
    }

    {
        auto res = tree.calculate_position_for_insert_equal(50);
        CHECK(res.pos == &n50);
        CHECK(res.left == false);
    }

    {
        auto res = tree.calculate_position_for_insert_equal(55);
        CHECK(res.pos == &n50);
        CHECK(res.left == false);
    }

    tree.data_.reset();
}

PRINT("Test PRIVATE calculate_position_for_insert_unique(const K&) [non-empty tree]");
{
    /*
     *                 (pointer to minumum value, i.e. 10)
     *                 |
     *                 H (header)
     *                /
     *           __ 40 __
     *          /        \
     *        20          50
     *       /  \
     *     10    30
     */

    using tree_type = sfl::dtl::rb_tree<int, int, sfl::dtl::identity, std::less<int>, TPARAM_ALLOCATOR<int>, void>;
    using node_type = typename tree_type::node_type;

    tree_type tree;
    node_type n10, n20, n30, n40, n50;

    tree.data_.root() = &n40;
    tree.data_.minimum() = &n10;
    tree.data_.size_ = 5;

    n10.left_  = nullptr;    n10.right_  = nullptr;   n10.parent_  = &n20;
    n20.left_  = &n10;       n20.right_  = &n30;      n20.parent_  = &n40;
    n30.left_  = nullptr;    n30.right_  = nullptr;   n30.parent_  = &n20;
    n40.left_  = &n20;       n40.right_  = &n50;      n40.parent_  = tree.data_.header();
    n50.left_  = nullptr;    n50.right_  = nullptr;   n50.parent_  = &n40;

    n10.value_ = 10;
    n20.value_ = 20;
    n30.value_ = 30;
    n40.value_ = 40;
    n50.value_ = 50;

    {
        auto res = tree.calculate_position_for_insert_unique(5);
        CHECK(res.status == true);
        CHECK(res.pos == &n10);
        CHECK(res.left == true);
    }

    {
        auto res = tree.calculate_position_for_insert_unique(10);
        CHECK(res.status == false);
        CHECK(res.pos == &n10);
    }

    {
        auto res = tree.calculate_position_for_insert_unique(15);
        CHECK(res.status == true);
        CHECK(res.pos == &n10);
        CHECK(res.left == false);
    }

    {
        auto res = tree.calculate_position_for_insert_unique(20);
        CHECK(res.status == false);
        CHECK(res.pos == &n20);
    }

    {
        auto res = tree.calculate_position_for_insert_unique(25);
        CHECK(res.status == true);
        CHECK(res.pos == &n30);
        CHECK(res.left == true);
    }

    {
        auto res = tree.calculate_position_for_insert_unique(30);
        CHECK(res.status == false);
        CHECK(res.pos == &n30);
    }

    {
        auto res = tree.calculate_position_for_insert_unique(35);
        CHECK(res.status == true);
        CHECK(res.pos == &n30);
        CHECK(res.left == false);
    }

    {
        auto res = tree.calculate_position_for_insert_unique(40);
        CHECK(res.status == false);
        CHECK(res.pos == &n40);
    }

    {
        auto res = tree.calculate_position_for_insert_unique(45);
        CHECK(res.status == true);
        CHECK(res.pos == &n50);
        CHECK(res.left == true);
    }

    {
        auto res = tree.calculate_position_for_insert_unique(50);
        CHECK(res.status == false);
        CHECK(res.pos == &n50);
    }

    {
        auto res = tree.calculate_position_for_insert_unique(55);
        CHECK(res.status == true);
        CHECK(res.pos == &n50);
        CHECK(res.left == false);
    }

    tree.data_.reset();
}

PRINT("Test PRIVATE calculate_position_for_insert_hint_equal(const_iterator, const K&) [non-empty tree]");
{
    {
        /*
         *              (pointer to minumum value, i.e. 10)
         *              |
         *              H (header)
         *             /
         *        __ 40 __
         *       /        \
         *     20          60
         *    /  \        /  \
         *  10    30    50    70
         */

        using tree_type = sfl::dtl::rb_tree<int, int, sfl::dtl::identity, std::less<int>, TPARAM_ALLOCATOR<int>, void>;
        using node_type = typename tree_type::node_type;

        tree_type tree;
        node_type n10, n20, n30, n40, n50, n60, n70;

        tree.data_.root() = &n40;
        tree.data_.minimum() = &n10;
        tree.data_.size_ = 7;

        n10.left_ = nullptr;    n10.right_ = nullptr;   n10.parent_ = &n20;
        n20.left_ = &n10;       n20.right_ = &n30;      n20.parent_ = &n40;
        n30.left_ = nullptr;    n30.right_ = nullptr;   n30.parent_ = &n20;
        n40.left_ = &n20;       n40.right_ = &n60;      n40.parent_ = tree.data_.header();
        n50.left_ = nullptr;    n50.right_ = nullptr;   n50.parent_ = &n60;
        n60.left_ = &n50;       n60.right_ = &n70;      n60.parent_ = &n40;
        n70.left_ = nullptr;    n70.right_ = nullptr;   n70.parent_ = &n60;

        n10.value_ = 10;
        n20.value_ = 20;
        n30.value_ = 30;
        n40.value_ = 40;
        n50.value_ = 50;
        n60.value_ = 60;
        n70.value_ = 70;

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 5);
            CHECK(res.pos == &n10);
            CHECK(res.left == true);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 10);
            CHECK((res.pos == &n10 && res.left == true) || (res.pos == &n10 && res.left == false));
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 15);
            CHECK(res.pos == &n10);
            CHECK(res.left == false);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 20);
            CHECK((res.pos == &n10 && res.left == false) || (res.pos == &n30 && res.left == true));
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 25);
            CHECK(res.pos == &n30);
            CHECK(res.left == true);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 30);
            CHECK((res.pos == &n30 && res.left == true) || (res.pos == &n30 && res.left == false));
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 35);
            CHECK(res.pos == &n30);
            CHECK(res.left == false);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 40);
            CHECK((res.pos == &n30 && res.left == false) || (res.pos == &n50 && res.left == true));
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 45);
            CHECK(res.pos == &n50);
            CHECK(res.left == true);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 50);
            CHECK((res.pos == &n50 && res.left == true) || (res.pos == &n50 && res.left == false));
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 55);
            CHECK(res.pos == &n50);
            CHECK(res.left == false);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 60);
            CHECK((res.pos == &n50 && res.left == false) || (res.pos == &n70 && res.left == true));
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 65);
            CHECK(res.pos == &n70);
            CHECK(res.left == true);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 70);
            CHECK((res.pos == &n70 && res.left == true) || (res.pos == &n70 && res.left == false));
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 75);
            CHECK(res.pos == &n70);
            CHECK(res.left == false);
        }

        tree.data_.reset();
    }

    {
        /*
         *              (pointer to minumum value, i.e. 10)
         *              |
         *              H (header)
         *             /
         *        __ 40 __
         *       /        \
         *     20          60
         *    /           /
         *  10          50
         */

        using tree_type = sfl::dtl::rb_tree<int, int, sfl::dtl::identity, std::less<int>, TPARAM_ALLOCATOR<int>, void>;
        using node_type = typename tree_type::node_type;

        tree_type tree;
        node_type n10, n20, n40, n50, n60;

        tree.data_.root() = &n40;
        tree.data_.minimum() = &n10;
        tree.data_.size_ = 5;

        n10.left_ = nullptr;    n10.right_ = nullptr;   n10.parent_ = &n20;
        n20.left_ = &n10;       n20.right_ = nullptr;   n20.parent_ = &n40;
        n40.left_ = &n20;       n40.right_ = &n60;      n40.parent_ = tree.data_.header();
        n50.left_ = nullptr;    n50.right_ = nullptr;   n50.parent_ = &n60;
        n60.left_ = &n50;       n60.right_ = nullptr;   n60.parent_ = &n40;

        n10.value_ = 10;
        n20.value_ = 20;
        n40.value_ = 40;
        n50.value_ = 50;
        n60.value_ = 60;

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 5);
            CHECK(res.pos == &n10);
            CHECK(res.left == true);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 10);
            CHECK((res.pos == &n10 && res.left == true) || (res.pos == &n10 && res.left == false));
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 15);
            CHECK(res.pos == &n10);
            CHECK(res.left == false);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 20);
            CHECK((res.pos == &n10 && res.left == false) || (res.pos == &n20 && res.left == false));
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 25);
            CHECK(res.pos == &n20);
            CHECK(res.left == false);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 30);
            CHECK(res.pos == &n20);
            CHECK(res.left == false);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 35);
            CHECK(res.pos == &n20);
            CHECK(res.left == false);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 40);
            CHECK((res.pos == &n20 && res.left == false) || (res.pos == &n50 && res.left == true));
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 45);
            CHECK(res.pos == &n50);
            CHECK(res.left == true);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 50);
            CHECK((res.pos == &n50 && res.left == true) || (res.pos == &n50 && res.left == false));
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 55);
            CHECK(res.pos == &n50);
            CHECK(res.left == false);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 60);
            CHECK((res.pos == &n50 && res.left == false) || (res.pos == &n60 && res.left == false));
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 65);
            CHECK(res.pos == &n60);
            CHECK(res.left == false);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 70);
            CHECK(res.pos == &n60);
            CHECK(res.left == false);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 75);
            CHECK(res.pos == &n60);
            CHECK(res.left == false);
        }

        tree.data_.reset();
    }

    {
        /*
         *              (pointer to minumum value, i.e. 20)
         *              |
         *              H (header)
         *             /
         *        __ 40 __
         *       /        \
         *     20          60
         *       \           \
         *        30          70
         */

        using tree_type = sfl::dtl::rb_tree<int, int, sfl::dtl::identity, std::less<int>, TPARAM_ALLOCATOR<int>, void>;
        using node_type = typename tree_type::node_type;

        tree_type tree;
        node_type n20, n30, n40, n60, n70;

        tree.data_.root() = &n40;
        tree.data_.minimum() = &n20;
        tree.data_.size_ = 5;

        n20.left_ = nullptr;    n20.right_ = &n30;      n20.parent_ = &n40;
        n30.left_ = nullptr;    n30.right_ = nullptr;   n30.parent_ = &n20;
        n40.left_ = &n20;       n40.right_ = &n60;      n40.parent_ = tree.data_.header();
        n60.left_ = nullptr;    n60.right_ = &n70;      n60.parent_ = &n40;
        n70.left_ = nullptr;    n70.right_ = nullptr;   n70.parent_ = &n60;

        n20.value_ = 20;
        n30.value_ = 30;
        n40.value_ = 40;
        n60.value_ = 60;
        n70.value_ = 70;

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 5);
            CHECK(res.pos == &n20);
            CHECK(res.left == true);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 10);
            CHECK(res.pos == &n20);
            CHECK(res.left == true);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 15);
            CHECK(res.pos == &n20);
            CHECK(res.left == true);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 20);
            CHECK((res.pos == &n20 && res.left == true) || (res.pos == &n30 && res.left == true));
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 25);
            CHECK(res.pos == &n30);
            CHECK(res.left == true);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 30);
            CHECK((res.pos == &n30 && res.left == true) || (res.pos == &n30 && res.left == false));
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 35);
            CHECK(res.pos == &n30);
            CHECK(res.left == false);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 40);
            CHECK((res.pos == &n30 && res.left == false) || (res.pos == &n60 && res.left == true));
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 45);
            CHECK(res.pos == &n60);
            CHECK(res.left == true);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 50);
            CHECK(res.pos == &n60);
            CHECK(res.left == true);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 55);
            CHECK(res.pos == &n60);
            CHECK(res.left == true);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 60);
            CHECK((res.pos == &n60 && res.left == true) || (res.pos == &n70 && res.left == true));
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 65);
            CHECK(res.pos == &n70);
            CHECK(res.left == true);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 70);
            CHECK((res.pos == &n70 && res.left == true) || (res.pos == &n70 && res.left == false));
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 75);
            CHECK(res.pos == &n70);
            CHECK(res.left == false);
        }

        tree.data_.reset();
    }

    {
        /*
         *           (pointer to minumum value, i.e. 10)
         *           |
         *           H (header)
         *          /
         *        40
         *       /  \
         *     20    50
         *    /  \
         *  10    30b
         *       /  \
         *     30a   30c
         */

        using tree_type = sfl::dtl::rb_tree<int, int, sfl::dtl::identity, std::less<int>, TPARAM_ALLOCATOR<int>, void>;
        using node_type = typename tree_type::node_type;

        tree_type tree;
        node_type n10, n20, n30a, n30b, n30c, n40, n50;

        tree.data_.root() = &n40;
        tree.data_.minimum() = &n10;
        tree.data_.size_ = 7;

        n10.left_  = nullptr;   n10.right_  = nullptr;  n10.parent_  = &n20;
        n20.left_  = &n10;      n20.right_  = &n30b;    n20.parent_  = &n40;
        n30a.left_ = nullptr;   n30a.right_ = nullptr;  n30a.parent_ = &n30b;
        n30b.left_ = &n30a;     n30b.right_ = &n30c;    n30b.parent_ = &n20;
        n30c.left_ = nullptr;   n30c.right_ = nullptr;  n30c.parent_ = &n30b;
        n40.left_  = &n20;      n40.right_  = &n50;     n40.parent_  = tree.data_.header();
        n50.left_  = nullptr;   n50.right_  = nullptr;  n50.parent_  = &n40;

        n10.value_  = 10;
        n20.value_  = 20;
        n30a.value_ = 30;
        n30b.value_ = 30;
        n30c.value_ = 30;
        n40.value_  = 40;
        n50.value_  = 50;

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 5);
            CHECK(res.pos == &n10);
            CHECK(res.left == true);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 10);
            CHECK((res.pos == &n10 && res.left == true) || (res.pos == &n10 && res.left == false));
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 15);
            CHECK(res.pos == &n10);
            CHECK(res.left == false);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 20);
            CHECK((res.pos == &n10 && res.left == false) || (res.pos == &n30a && res.left == true));
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 25);
            CHECK(res.pos == &n30a);
            CHECK(res.left == true);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 30);
            CHECK((res.pos == &n30a && res.left == true) || (res.pos == &n30a && res.left == false) ||
                  (res.pos == &n30c && res.left == true) || (res.pos == &n30c && res.left == false));
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 35);
            CHECK(res.pos == &n30c);
            CHECK(res.left == false);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 40);
            CHECK((res.pos == &n30c && res.left == false) || (res.pos == &n50 && res.left == true));
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 45);
            CHECK(res.pos == &n50);
            CHECK(res.left == true);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 50);
            CHECK((res.pos == &n50 && res.left == true) || (res.pos == &n50 && res.left == false));
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 55);
            CHECK(res.pos == &n50);
            CHECK(res.left == false);
        }

        tree.data_.reset();
    }

    {
        /*
         *           (pointer to minumum value, i.e. 10)
         *           |
         *           H (header)
         *          /
         *        40
         *       /  \
         *     20    50
         *    /  \
         *  10    30b
         *       /
         *     30a
         */

        using tree_type = sfl::dtl::rb_tree<int, int, sfl::dtl::identity, std::less<int>, TPARAM_ALLOCATOR<int>, void>;
        using node_type = typename tree_type::node_type;

        tree_type tree;
        node_type n10, n20, n30a, n30b, n40, n50;

        tree.data_.root() = &n40;
        tree.data_.minimum() = &n10;
        tree.data_.size_ = 7;

        n10.left_  = nullptr;   n10.right_  = nullptr;  n10.parent_  = &n20;
        n20.left_  = &n10;      n20.right_  = &n30b;    n20.parent_  = &n40;
        n30a.left_ = nullptr;   n30a.right_ = nullptr;  n30a.parent_ = &n30b;
        n30b.left_ = &n30a;     n30b.right_ = nullptr;  n30b.parent_ = &n20;
        n40.left_  = &n20;      n40.right_  = &n50;     n40.parent_  = tree.data_.header();
        n50.left_  = nullptr;   n50.right_  = nullptr;  n50.parent_  = &n40;

        n10.value_  = 10;
        n20.value_  = 20;
        n30a.value_ = 30;
        n30b.value_ = 30;
        n40.value_  = 40;
        n50.value_  = 50;

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 5);
            CHECK(res.pos == &n10);
            CHECK(res.left == true);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 10);
            CHECK((res.pos == &n10 && res.left == true) || (res.pos == &n10 && res.left == false));
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 15);
            CHECK(res.pos == &n10);
            CHECK(res.left == false);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 20);
            CHECK((res.pos == &n10 && res.left == false) || (res.pos == &n30a && res.left == true));
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 25);
            CHECK(res.pos == &n30a);
            CHECK(res.left == true);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 30);
            CHECK((res.pos == &n30a && res.left == true) || (res.pos == &n30a && res.left == false) ||
                  (res.pos == &n30b && res.left == false));
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 35);
            CHECK(res.pos == &n30b);
            CHECK(res.left == false);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 40);
            CHECK((res.pos == &n30b && res.left == false) || (res.pos == &n50 && res.left == true));
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 45);
            CHECK(res.pos == &n50);
            CHECK(res.left == true);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 50);
            CHECK((res.pos == &n50 && res.left == true) || (res.pos == &n50 && res.left == false));
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 55);
            CHECK(res.pos == &n50);
            CHECK(res.left == false);
        }

        tree.data_.reset();
    }

    {
        /*
         *           (pointer to minumum value, i.e. 10)
         *           |
         *           H (header)
         *          /
         *        40
         *       /  \
         *     20    50
         *    /  \
         *  10    30b
         *          \
         *           30c
         */

        using tree_type = sfl::dtl::rb_tree<int, int, sfl::dtl::identity, std::less<int>, TPARAM_ALLOCATOR<int>, void>;
        using node_type = typename tree_type::node_type;

        tree_type tree;
        node_type n10, n20, n30b, n30c, n40, n50;

        tree.data_.root() = &n40;
        tree.data_.minimum() = &n10;
        tree.data_.size_ = 6;

        n10.left_  = nullptr;   n10.right_  = nullptr;  n10.parent_  = &n20;
        n20.left_  = &n10;      n20.right_  = &n30b;    n20.parent_  = &n40;
        n30b.left_ = nullptr;   n30b.right_ = &n30c;    n30b.parent_ = &n20;
        n30c.left_ = nullptr;   n30c.right_ = nullptr;  n30c.parent_ = &n30b;
        n40.left_  = &n20;      n40.right_  = &n50;     n40.parent_  = tree.data_.header();
        n50.left_  = nullptr;   n50.right_  = nullptr;  n50.parent_  = &n40;

        n10.value_  = 10;
        n20.value_  = 20;
        n30b.value_ = 30;
        n30c.value_ = 30;
        n40.value_  = 40;
        n50.value_  = 50;

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 5);
            CHECK(res.pos == &n10);
            CHECK(res.left == true);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 10);
            CHECK((res.pos == &n10 && res.left == true) || (res.pos == &n10 && res.left == false));
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 15);
            CHECK(res.pos == &n10);
            CHECK(res.left == false);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 20);
            CHECK((res.pos == &n10 && res.left == false) || (res.pos == &n30b && res.left == true));
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 25);
            CHECK(res.pos == &n30b);
            CHECK(res.left == true);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 30);
            CHECK((res.pos == &n30b && res.left == true) ||
                  (res.pos == &n30c && res.left == true) || (res.pos == &n30c && res.left == false));
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 35);
            CHECK(res.pos == &n30c);
            CHECK(res.left == false);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 40);
            CHECK((res.pos == &n30c && res.left == false) || (res.pos == &n50 && res.left == true));
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 45);
            CHECK(res.pos == &n50);
            CHECK(res.left == true);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 50);
            CHECK((res.pos == &n50 && res.left == true) || (res.pos == &n50 && res.left == false));
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_equal(hint, 55);
            CHECK(res.pos == &n50);
            CHECK(res.left == false);
        }

        tree.data_.reset();
    }
}

PRINT("Test PRIVATE calculate_position_for_insert_hint_unique(const_iterator, const K&) [non-empty tree]");
{
    {
        /*
         *              (pointer to minumum value, i.e. 10)
         *              |
         *              H (header)
         *             /
         *        __ 40 __
         *       /        \
         *     20          60
         *    /  \        /  \
         *  10    30    50    70
         */

        using tree_type = sfl::dtl::rb_tree<int, int, sfl::dtl::identity, std::less<int>, TPARAM_ALLOCATOR<int>, void>;
        using node_type = typename tree_type::node_type;

        tree_type tree;
        node_type n10, n20, n30, n40, n50, n60, n70;

        tree.data_.root() = &n40;
        tree.data_.minimum() = &n10;
        tree.data_.size_ = 7;

        n10.left_ = nullptr;    n10.right_ = nullptr;   n10.parent_ = &n20;
        n20.left_ = &n10;       n20.right_ = &n30;      n20.parent_ = &n40;
        n30.left_ = nullptr;    n30.right_ = nullptr;   n30.parent_ = &n20;
        n40.left_ = &n20;       n40.right_ = &n60;      n40.parent_ = tree.data_.header();
        n50.left_ = nullptr;    n50.right_ = nullptr;   n50.parent_ = &n60;
        n60.left_ = &n50;       n60.right_ = &n70;      n60.parent_ = &n40;
        n70.left_ = nullptr;    n70.right_ = nullptr;   n70.parent_ = &n60;

        n10.value_ = 10;
        n20.value_ = 20;
        n30.value_ = 30;
        n40.value_ = 40;
        n50.value_ = 50;
        n60.value_ = 60;
        n70.value_ = 70;

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_unique(hint, 5);
            CHECK(res.status == true);
            CHECK(res.pos == &n10);
            CHECK(res.left == true);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_unique(hint, 10);
            CHECK(res.status == false);
            CHECK(res.pos == &n10);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_unique(hint, 15);
            CHECK(res.status == true);
            CHECK(res.pos == &n10);
            CHECK(res.left == false);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_unique(hint, 20);
            CHECK(res.status == false);
            CHECK(res.pos == &n20);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_unique(hint, 25);
            CHECK(res.status == true);
            CHECK(res.pos == &n30);
            CHECK(res.left == true);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_unique(hint, 30);
            CHECK(res.status == false);
            CHECK(res.pos == &n30);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_unique(hint, 35);
            CHECK(res.status == true);
            CHECK(res.pos == &n30);
            CHECK(res.left == false);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_unique(hint, 40);
            CHECK(res.status == false);
            CHECK(res.pos == &n40);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_unique(hint, 45);
            CHECK(res.status == true);
            CHECK(res.pos == &n50);
            CHECK(res.left == true);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_unique(hint, 50);
            CHECK(res.status == false);
            CHECK(res.pos == &n50);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_unique(hint, 55);
            CHECK(res.status == true);
            CHECK(res.pos == &n50);
            CHECK(res.left == false);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_unique(hint, 60);
            CHECK(res.status == false);
            CHECK(res.pos == &n60);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_unique(hint, 65);
            CHECK(res.status == true);
            CHECK(res.pos == &n70);
            CHECK(res.left == true);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_unique(hint, 70);
            CHECK(res.status == false);
            CHECK(res.pos == &n70);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_unique(hint, 75);
            CHECK(res.status == true);
            CHECK(res.pos == &n70);
            CHECK(res.left == false);
        }

        tree.data_.reset();
    }

    {
        /*
         *              (pointer to minumum value, i.e. 10)
         *              |
         *              H (header)
         *             /
         *        __ 40 __
         *       /        \
         *     20          60
         *    /           /
         *  10          50
         */

        using tree_type = sfl::dtl::rb_tree<int, int, sfl::dtl::identity, std::less<int>, TPARAM_ALLOCATOR<int>, void>;
        using node_type = typename tree_type::node_type;

        tree_type tree;
        node_type n10, n20, n40, n50, n60;

        tree.data_.root() = &n40;
        tree.data_.minimum() = &n10;
        tree.data_.size_ = 5;

        n10.left_ = nullptr;    n10.right_ = nullptr;   n10.parent_ = &n20;
        n20.left_ = &n10;       n20.right_ = nullptr;   n20.parent_ = &n40;
        n40.left_ = &n20;       n40.right_ = &n60;      n40.parent_ = tree.data_.header();
        n50.left_ = nullptr;    n50.right_ = nullptr;   n50.parent_ = &n60;
        n60.left_ = &n50;       n60.right_ = nullptr;   n60.parent_ = &n40;

        n10.value_ = 10;
        n20.value_ = 20;
        n40.value_ = 40;
        n50.value_ = 50;
        n60.value_ = 60;

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_unique(hint, 5);
            CHECK(res.status == true);
            CHECK(res.pos == &n10);
            CHECK(res.left == true);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_unique(hint, 10);
            CHECK(res.status == false);
            CHECK(res.pos == &n10);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_unique(hint, 15);
            CHECK(res.status == true);
            CHECK(res.pos == &n10);
            CHECK(res.left == false);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_unique(hint, 20);
            CHECK(res.status == false);
            CHECK(res.pos == &n20);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_unique(hint, 25);
            CHECK(res.status == true);
            CHECK(res.pos == &n20);
            CHECK(res.left == false);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_unique(hint, 30);
            CHECK(res.status == true);
            CHECK(res.pos == &n20);
            CHECK(res.left == false);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_unique(hint, 35);
            CHECK(res.status == true);
            CHECK(res.pos == &n20);
            CHECK(res.left == false);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_unique(hint, 40);
            CHECK(res.status == false);
            CHECK(res.pos == &n40);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_unique(hint, 45);
            CHECK(res.status == true);
            CHECK(res.pos == &n50);
            CHECK(res.left == true);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_unique(hint, 50);
            CHECK(res.status == false);
            CHECK(res.pos == &n50);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_unique(hint, 55);
            CHECK(res.status == true);
            CHECK(res.pos == &n50);
            CHECK(res.left == false);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_unique(hint, 60);
            CHECK(res.status == false);
            CHECK(res.pos == &n60);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_unique(hint, 65);
            CHECK(res.status == true);
            CHECK(res.pos == &n60);
            CHECK(res.left == false);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_unique(hint, 70);
            CHECK(res.status == true);
            CHECK(res.pos == &n60);
            CHECK(res.left == false);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_unique(hint, 75);
            CHECK(res.status == true);
            CHECK(res.pos == &n60);
            CHECK(res.left == false);
        }

        tree.data_.reset();
    }

    {
        /*
         *              (pointer to minumum value, i.e. 20)
         *              |
         *              H (header)
         *             /
         *        __ 40 __
         *       /        \
         *     20          60
         *       \           \
         *        30          70
         */

        using tree_type = sfl::dtl::rb_tree<int, int, sfl::dtl::identity, std::less<int>, TPARAM_ALLOCATOR<int>, void>;
        using node_type = typename tree_type::node_type;

        tree_type tree;
        node_type n20, n30, n40, n60, n70;

        tree.data_.root() = &n40;
        tree.data_.minimum() = &n20;
        tree.data_.size_ = 5;

        n20.left_ = nullptr;    n20.right_ = &n30;      n20.parent_ = &n40;
        n30.left_ = nullptr;    n30.right_ = nullptr;   n30.parent_ = &n20;
        n40.left_ = &n20;       n40.right_ = &n60;      n40.parent_ = tree.data_.header();
        n60.left_ = nullptr;    n60.right_ = &n70;      n60.parent_ = &n40;
        n70.left_ = nullptr;    n70.right_ = nullptr;   n70.parent_ = &n60;

        n20.value_ = 20;
        n30.value_ = 30;
        n40.value_ = 40;
        n60.value_ = 60;
        n70.value_ = 70;

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_unique(hint, 5);
            CHECK(res.status == true);
            CHECK(res.pos == &n20);
            CHECK(res.left == true);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_unique(hint, 10);
            CHECK(res.status == true);
            CHECK(res.pos == &n20);
            CHECK(res.left == true);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_unique(hint, 15);
            CHECK(res.status == true);
            CHECK(res.pos == &n20);
            CHECK(res.left == true);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_unique(hint, 20);
            CHECK(res.status == false);
            CHECK(res.pos == &n20);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_unique(hint, 25);
            CHECK(res.status == true);
            CHECK(res.pos == &n30);
            CHECK(res.left == true);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_unique(hint, 30);
            CHECK(res.status == false);
            CHECK(res.pos == &n30);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_unique(hint, 35);
            CHECK(res.status == true);
            CHECK(res.pos == &n30);
            CHECK(res.left == false);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_unique(hint, 40);
            CHECK(res.status == false);
            CHECK(res.pos == &n40);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_unique(hint, 45);
            CHECK(res.status == true);
            CHECK(res.pos == &n60);
            CHECK(res.left == true);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_unique(hint, 50);
            CHECK(res.status == true);
            CHECK(res.pos == &n60);
            CHECK(res.left == true);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_unique(hint, 55);
            CHECK(res.status == true);
            CHECK(res.pos == &n60);
            CHECK(res.left == true);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_unique(hint, 60);
            CHECK(res.status == false);
            CHECK(res.pos == &n60);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_unique(hint, 65);
            CHECK(res.status == true);
            CHECK(res.pos == &n70);
            CHECK(res.left == true);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_unique(hint, 70);
            CHECK(res.status == false);
            CHECK(res.pos == &n70);
        }

        for (int i = 0; i < int(tree.size() + 1); ++i)
        {
            auto hint = NTH(tree, i);
            auto res = tree.calculate_position_for_insert_hint_unique(hint, 75);
            CHECK(res.status == true);
            CHECK(res.pos == &n70);
            CHECK(res.left == false);
        }

        tree.data_.reset();
    }
}

PRINT("Test PRIVATE rotate_left(base_node_pointer)");
{
    {
        /*
         *          (pointer to node 1)          (pointer to node 1)
         *          |                            |
         *          H (header)                   H (header)
         *         /                            /
         *        2           left             4
         *       / \         rotate           / \
         *      1   4     ------------>      2   5
         *         / \      @ node 2        / \
         *        3   5                    1   3
         */

        using tree_type = sfl::dtl::rb_tree<int, int, sfl::dtl::identity, std::less<int>, TPARAM_ALLOCATOR<int>, void>;
        using node_type = typename tree_type::node_type;

        tree_type tree;
        node_type n1, n2, n3, n4, n5;

        tree.data_.root() = &n2;
        tree.data_.minimum() = &n1;

        n1.left_ = nullptr; n1.right_ = nullptr; n1.parent_ = &n2;
        n2.left_ = &n1;     n2.right_ = &n4;     n2.parent_ = tree.data_.header();
        n3.left_ = nullptr; n3.right_ = nullptr; n3.parent_ = &n4;
        n4.left_ = &n3;     n4.right_ = &n5;     n4.parent_ = &n2;
        n5.left_ = nullptr; n5.right_ = nullptr; n5.parent_ = &n4;

        tree.rotate_left(&n2);

        CHECK(tree.data_.root() == &n4);
        CHECK(tree.data_.minimum() == &n1);

        CHECK(n1.left_ == nullptr); CHECK(n1.right_ == nullptr); CHECK(n1.parent_ == &n2);
        CHECK(n2.left_ == &n1);     CHECK(n2.right_ == &n3);     CHECK(n2.parent_ == &n4);
        CHECK(n3.left_ == nullptr); CHECK(n3.right_ == nullptr); CHECK(n3.parent_ == &n2);
        CHECK(n4.left_ == &n2);     CHECK(n4.right_ == &n5);     CHECK(n4.parent_ == tree.data_.header());
        CHECK(n5.left_ == nullptr); CHECK(n5.right_ == nullptr); CHECK(n5.parent_ == &n4);

        tree.data_.reset();
    }

    {
        /*
         *            (pointer to node 1)          (pointer to node 1)
         *            |                            |
         *            H (header)                   H (header)
         *           /                            /
         *          6           left             6
         *         / \         rotate           / \
         *        2   7     ------------>      4   7
         *       / \          @ node 2        / \
         *      1   4                        2   5
         *         / \                      / \
         *        3   5                    1   3
         */

        using tree_type = sfl::dtl::rb_tree<int, int, sfl::dtl::identity, std::less<int>, TPARAM_ALLOCATOR<int>, void>;
        using node_type = typename tree_type::node_type;

        tree_type tree;
        node_type n1, n2, n3, n4, n5, n6, n7;

        tree.data_.root() = &n6;
        tree.data_.minimum() = &n1;

        n1.left_ = nullptr; n1.right_ = nullptr; n1.parent_ = &n2;
        n2.left_ = &n1;     n2.right_ = &n4;     n2.parent_ = &n6;
        n3.left_ = nullptr; n3.right_ = nullptr; n3.parent_ = &n4;
        n4.left_ = &n3;     n4.right_ = &n5;     n4.parent_ = &n2;
        n5.left_ = nullptr; n5.right_ = nullptr; n5.parent_ = &n4;
        n6.left_ = &n2;     n6.right_ = &n7;     n6.parent_ = tree.data_.header();
        n7.left_ = nullptr; n7.right_ = nullptr; n7.parent_ = &n6;

        tree.rotate_left(&n2);

        CHECK(tree.data_.root() == &n6);
        CHECK(tree.data_.minimum() == &n1);

        CHECK(n1.left_ == nullptr); CHECK(n1.right_ == nullptr); CHECK(n1.parent_ == &n2);
        CHECK(n2.left_ == &n1);     CHECK(n2.right_ == &n3);     CHECK(n2.parent_ == &n4);
        CHECK(n3.left_ == nullptr); CHECK(n3.right_ == nullptr); CHECK(n3.parent_ == &n2);
        CHECK(n4.left_ == &n2);     CHECK(n4.right_ == &n5);     CHECK(n4.parent_ == &n6);
        CHECK(n5.left_ == nullptr); CHECK(n5.right_ == nullptr); CHECK(n5.parent_ == &n4);
        CHECK(n6.left_ == &n4);     CHECK(n6.right_ == &n7);     CHECK(n6.parent_ == tree.data_.header());
        CHECK(n7.left_ == nullptr); CHECK(n7.right_ == nullptr); CHECK(n7.parent_ == &n6);

        tree.data_.reset();
    }

    {
        /*
         *          (pointer to node 1)          (pointer to node 1)
         *          |                            |
         *          H (header)                   H (header)
         *         /                            /
         *        2              left          2
         *       / \            rotate        / \
         *      1   4        ------------>   1   6
         *         / \         @ node 4         / \
         *        3   6                        4   7
         *           / \                      / \
         *          5   7                    3   5
         */

        using tree_type = sfl::dtl::rb_tree<int, int, sfl::dtl::identity, std::less<int>, TPARAM_ALLOCATOR<int>, void>;
        using node_type = typename tree_type::node_type;

        tree_type tree;
        node_type n1, n2, n3, n4, n5, n6, n7;

        tree.data_.root() = &n2;
        tree.data_.minimum() = &n1;

        n1.left_ = nullptr; n1.right_ = nullptr; n1.parent_ = &n2;
        n2.left_ = &n1;     n2.right_ = &n4;     n2.parent_ = tree.data_.header();
        n3.left_ = nullptr; n3.right_ = nullptr; n3.parent_ = &n4;
        n4.left_ = &n3;     n4.right_ = &n6;     n4.parent_ = &n2;
        n5.left_ = nullptr; n5.right_ = nullptr; n5.parent_ = &n6;
        n6.left_ = &n5;     n6.right_ = &n7;     n6.parent_ = &n4;
        n7.left_ = nullptr; n7.right_ = nullptr; n7.parent_ = &n6;

        tree.rotate_left(&n4);

        CHECK(tree.data_.root() == &n2);
        CHECK(tree.data_.minimum() == &n1);

        CHECK(n1.left_ == nullptr); CHECK(n1.right_ == nullptr); CHECK(n1.parent_ == &n2);
        CHECK(n2.left_ == &n1);     CHECK(n2.right_ == &n6);     CHECK(n2.parent_ == tree.data_.header());
        CHECK(n3.left_ == nullptr); CHECK(n3.right_ == nullptr); CHECK(n3.parent_ == &n4);
        CHECK(n4.left_ == &n3);     CHECK(n4.right_ == &n5);     CHECK(n4.parent_ == &n6);
        CHECK(n5.left_ == nullptr); CHECK(n5.right_ == nullptr); CHECK(n5.parent_ == &n4);
        CHECK(n6.left_ == &n4);     CHECK(n6.right_ == &n7);     CHECK(n6.parent_ == &n2);
        CHECK(n7.left_ == nullptr); CHECK(n7.right_ == nullptr); CHECK(n7.parent_ == &n6);

        tree.data_.reset();
    }
}

PRINT("Test PRIVATE rotate_right(base_node_pointer)");
{
    {
        /*
         *            (pointer to node 1)            (pointer to node 1)
         *            |                              |
         *            H (header)                     H (header)
         *           /                              /
         *          4             right            2
         *         / \            rotate          / \
         *        2   5        ------------>     1   4
         *       / \             @ node 4           / \
         *      1   3                              3   5
         */

        using tree_type = sfl::dtl::rb_tree<int, int, sfl::dtl::identity, std::less<int>, TPARAM_ALLOCATOR<int>, void>;
        using node_type = typename tree_type::node_type;

        tree_type tree;
        node_type n1, n2, n3, n4, n5;

        tree.data_.root() = &n4;
        tree.data_.minimum() = &n1;

        n1.left_ = nullptr; n1.right_ = nullptr; n1.parent_ = &n2;
        n2.left_ = &n1;     n2.right_ = &n3;     n2.parent_ = &n4;
        n3.left_ = nullptr; n3.right_ = nullptr; n3.parent_ = &n2;
        n4.left_ = &n2;     n4.right_ = &n5;     n4.parent_ = tree.data_.header();
        n5.left_ = nullptr; n5.right_ = nullptr; n5.parent_ = &n4;

        tree.rotate_right(&n4);

        CHECK(tree.data_.root() == &n2);
        CHECK(tree.data_.minimum() == &n1);

        CHECK(n1.left_ == nullptr); CHECK(n1.right_ == nullptr); CHECK(n1.parent_ == &n2);
        CHECK(n2.left_ == &n1);     CHECK(n2.right_ == &n4);     CHECK(n2.parent_ == tree.data_.header());
        CHECK(n3.left_ == nullptr); CHECK(n3.right_ == nullptr); CHECK(n3.parent_ == &n4);
        CHECK(n4.left_ == &n3);     CHECK(n4.right_ == &n5);     CHECK(n4.parent_ == &n2);
        CHECK(n5.left_ == nullptr); CHECK(n5.right_ == nullptr); CHECK(n5.parent_ == &n4);

        tree.data_.reset();
    }

    {
        /*
         *              (pointer to node 1)          (pointer to node 1)
         *              |                            |
         *              H (header)                   H (header)
         *             /                            /
         *            6           right            6
         *           / \          rotate          / \
         *          4   7      ------------>     2   7
         *         / \           @ node 4       / \
         *        2   5                        1   4
         *       / \                              / \
         *      1   3                            3   5
         */

        using tree_type = sfl::dtl::rb_tree<int, int, sfl::dtl::identity, std::less<int>, TPARAM_ALLOCATOR<int>, void>;
        using node_type = typename tree_type::node_type;

        tree_type tree;
        node_type n1, n2, n3, n4, n5, n6, n7;

        tree.data_.root() = &n6;
        tree.data_.minimum() = &n1;

        n1.left_ = nullptr; n1.right_ = nullptr; n1.parent_ = &n2;
        n2.left_ = &n1;     n2.right_ = &n3;     n2.parent_ = &n4;
        n3.left_ = nullptr; n3.right_ = nullptr; n3.parent_ = &n2;
        n4.left_ = &n2;     n4.right_ = &n5;     n4.parent_ = &n6;
        n5.left_ = nullptr; n5.right_ = nullptr; n5.parent_ = &n4;
        n6.left_ = &n4;     n6.right_ = &n7;     n6.parent_ = tree.data_.header();
        n7.left_ = nullptr; n7.right_ = nullptr; n7.parent_ = &n6;

        tree.rotate_right(&n4);

        CHECK(tree.data_.root() == &n6);
        CHECK(tree.data_.minimum() == &n1);

        CHECK(n1.left_ == nullptr); CHECK(n1.right_ == nullptr); CHECK(n1.parent_ == &n2);
        CHECK(n2.left_ == &n1);     CHECK(n2.right_ == &n4);     CHECK(n2.parent_ == &n6);
        CHECK(n3.left_ == nullptr); CHECK(n3.right_ == nullptr); CHECK(n3.parent_ == &n4);
        CHECK(n4.left_ == &n3);     CHECK(n4.right_ == &n5);     CHECK(n4.parent_ == &n2);
        CHECK(n5.left_ == nullptr); CHECK(n5.right_ == nullptr); CHECK(n5.parent_ == &n4);
        CHECK(n6.left_ == &n2);     CHECK(n6.right_ == &n7);     CHECK(n6.parent_ == tree.data_.header());
        CHECK(n7.left_ == nullptr); CHECK(n7.right_ == nullptr); CHECK(n7.parent_ == &n6);

        tree.data_.reset();
    }

    {
        /*
         *          (pointer to node 1)        (pointer to node 1)
         *          |                          |
         *          H (header)                 H (header)
         *         /                          /
         *        2            right         2
         *       / \           rotate       / \
         *      1   6       ------------>  1   4
         *         / \        @ node 6        / \
         *        4   7                      3   6
         *       / \                            / \
         *      3   5                          5   7
         */

        using tree_type = sfl::dtl::rb_tree<int, int, sfl::dtl::identity, std::less<int>, TPARAM_ALLOCATOR<int>, void>;
        using node_type = typename tree_type::node_type;

        tree_type tree;
        node_type n1, n2, n3, n4, n5, n6, n7;

        tree.data_.root() = &n2;
        tree.data_.minimum() = &n1;

        n1.left_ = nullptr; n1.right_ = nullptr; n1.parent_ = &n2;
        n2.left_ = &n1;     n2.right_ = &n6;     n2.parent_ = tree.data_.header();
        n3.left_ = nullptr; n3.right_ = nullptr; n3.parent_ = &n4;
        n4.left_ = &n3;     n4.right_ = &n5;     n4.parent_ = &n6;
        n5.left_ = nullptr; n5.right_ = nullptr; n5.parent_ = &n4;
        n6.left_ = &n4;     n6.right_ = &n7;     n6.parent_ = &n2;
        n7.left_ = nullptr; n7.right_ = nullptr; n7.parent_ = &n6;

        tree.rotate_right(&n6);

        CHECK(tree.data_.root() == &n2);
        CHECK(tree.data_.minimum() == &n1);

        CHECK(n1.left_ == nullptr); CHECK(n1.right_ == nullptr); CHECK(n1.parent_ == &n2);
        CHECK(n2.left_ == &n1);     CHECK(n2.right_ == &n4);     CHECK(n2.parent_ == tree.data_.header());
        CHECK(n3.left_ == nullptr); CHECK(n3.right_ == nullptr); CHECK(n3.parent_ == &n4);
        CHECK(n4.left_ == &n3);     CHECK(n4.right_ == &n6);     CHECK(n4.parent_ == &n2);
        CHECK(n5.left_ == nullptr); CHECK(n5.right_ == nullptr); CHECK(n5.parent_ == &n6);
        CHECK(n6.left_ == &n5);     CHECK(n6.right_ == &n7);     CHECK(n6.parent_ == &n4);
        CHECK(n7.left_ == nullptr); CHECK(n7.right_ == nullptr); CHECK(n7.parent_ == &n6);

        tree.data_.reset();
    }
}

PRINT("Test PRIVATE insert(base_node_pointer, base_node_pointer, bool, base_node_pointer&, base_node_pointer&)");
{
    static const std::vector<std::vector<int>> insert_patterns
    {
        {  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16 },
        {  1,  2,  3,  4,  5,  6,  7,  8, 16, 15, 14, 13, 12, 11, 10,  9 },
        {  8,  7,  6,  5,  4,  3,  2,  1,  9, 10, 11, 12, 13, 14, 15, 16 },
        {  8,  7,  6,  5,  4,  3,  2,  1, 16, 15, 14, 13, 12, 11, 10,  9 },

        {  9, 10, 11, 12, 13, 14, 15, 16,  1,  2,  3,  4,  5,  6,  7,  8 },
        {  9, 10, 11, 12, 13, 14, 15, 16,  8,  7,  6,  5,  4,  3,  2,  1 },
        { 16, 15, 14, 13, 12, 11, 10,  9,  1,  2,  3,  4,  5,  6,  7,  8 },
        { 16, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1 },

        {  1,  3,  5,  7,  9, 11, 13, 15,  2,  4,  6,  8, 10, 12, 14, 16 },
        {  1,  3,  5,  7,  9, 11, 13, 15, 16, 14, 12, 10,  8,  6,  4,  2 },
        { 15, 13, 11,  9,  7,  5,  3,  1,  2,  4,  6,  8, 10, 12, 14, 16 },
        { 15, 13, 11,  9,  7,  5,  3,  1, 16, 14, 12, 10,  8,  6,  4,  2 },

        {  2,  4,  6,  8, 10, 12, 14, 16,  1,  3,  5,  7,  9, 11, 13, 15 },
        {  2,  4,  6,  8, 10, 12, 14, 16, 15, 13, 11,  9,  7,  5,  3,  1 },
        { 16, 14, 12, 10,  8,  6,  4,  2,  1,  3,  5,  7,  9, 11, 13, 15 },
        { 16, 14, 12, 10,  8,  6,  4,  2, 15, 13, 11,  9,  7,  5,  3,  1 },
    };

    for (auto& row : insert_patterns)
    {
        using tree_type = sfl::dtl::rb_tree<int, int, sfl::dtl::identity, std::less<int>, TPARAM_ALLOCATOR<int>, void>;
        using node_type = typename tree_type::node_type;
        using node_pointer = typename tree_type::node_pointer;

        tree_type tree;

        ///////////////////////////////////////////////////////////////////////

        std::vector<node_type> nodes1(row.size());
        std::vector<node_type> nodes2(row.size());
        std::vector<node_type> nodes3(row.size());

        for (int i = 0; i < int(row.size()); ++i)
        {
            nodes1[i].value_ = row[i];
            nodes2[i].value_ = row[i];
            nodes3[i].value_ = row[i];
        }

        CHECK(tree.begin() == tree.end());
        CHECK(tree.data_.root() == nullptr);
        CHECK(tree.data_.minimum() == tree.data_.header());

        ///////////////////////////////////////////////////////////////////////

        for (int i = 0; i < int(nodes1.size()); ++i)
        {
            const auto res = tree.calculate_position_for_insert_equal(nodes1[i].value_);
            tree.insert(&nodes1[i], res.pos, res.left, tree.data_.root(), tree.data_.minimum());
        }

        CHECK(tree.begin() != tree.end());
        CHECK(tree.data_.root() != nullptr);
        CHECK(static_cast<node_pointer>(tree.data_.minimum())->value_ == 1);

        {
            int i = 1;

            for (auto it = tree.begin(), end = tree.end(); it != end; ++it, ++i)
            {
                CHECK(*it == i);
            }
        }

        ///////////////////////////////////////////////////////////////////////

        for (int i = 0; i < int(nodes2.size()); ++i)
        {
            const auto res = tree.calculate_position_for_insert_equal(nodes2[i].value_);
            tree.insert(&nodes2[i], res.pos, res.left, tree.data_.root(), tree.data_.minimum());
        }

        CHECK(tree.begin() != tree.end());
        CHECK(tree.data_.root() != nullptr);
        CHECK(static_cast<node_pointer>(tree.data_.minimum())->value_ == 1);

        {
            int i = 1;

            for (auto it = tree.begin(), end = tree.end(); it != end; ++it, ++i)
            {
                CHECK(*it == i);
                ++it;
                CHECK(*it == i);
            }
        }

        ///////////////////////////////////////////////////////////////////////

        for (int i = 0; i < int(nodes3.size()); ++i)
        {
            const auto res = tree.calculate_position_for_insert_equal(nodes3[i].value_);
            tree.insert(&nodes3[i], res.pos, res.left, tree.data_.root(), tree.data_.minimum());
        }

        CHECK(tree.begin() != tree.end());
        CHECK(tree.data_.root() != nullptr);
        CHECK(static_cast<node_pointer>(tree.data_.minimum())->value_ == 1);

        {
            int i = 1;

            for (auto it = tree.begin(), end = tree.end(); it != end; ++it, ++i)
            {
                CHECK(*it == i);
                ++it;
                CHECK(*it == i);
                ++it;
                CHECK(*it == i);
            }
        }

        ///////////////////////////////////////////////////////////////////////

        tree.data_.reset();
    }
}

PRINT("Test PRIVATE remove(base_node_pointer, base_node_pointer&)");
{
    static const std::vector<std::vector<int>> remove_patterns
    {
        {  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16 },
        {  1,  2,  3,  4,  5,  6,  7,  8, 16, 15, 14, 13, 12, 11, 10,  9 },
        {  8,  7,  6,  5,  4,  3,  2,  1,  9, 10, 11, 12, 13, 14, 15, 16 },
        {  8,  7,  6,  5,  4,  3,  2,  1, 16, 15, 14, 13, 12, 11, 10,  9 },

        {  9, 10, 11, 12, 13, 14, 15, 16,  1,  2,  3,  4,  5,  6,  7,  8 },
        {  9, 10, 11, 12, 13, 14, 15, 16,  8,  7,  6,  5,  4,  3,  2,  1 },
        { 16, 15, 14, 13, 12, 11, 10,  9,  1,  2,  3,  4,  5,  6,  7,  8 },
        { 16, 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1 },

        {  1,  3,  5,  7,  9, 11, 13, 15,  2,  4,  6,  8, 10, 12, 14, 16 },
        {  1,  3,  5,  7,  9, 11, 13, 15, 16, 14, 12, 10,  8,  6,  4,  2 },
        { 15, 13, 11,  9,  7,  5,  3,  1,  2,  4,  6,  8, 10, 12, 14, 16 },
        { 15, 13, 11,  9,  7,  5,  3,  1, 16, 14, 12, 10,  8,  6,  4,  2 },

        {  2,  4,  6,  8, 10, 12, 14, 16,  1,  3,  5,  7,  9, 11, 13, 15 },
        {  2,  4,  6,  8, 10, 12, 14, 16, 15, 13, 11,  9,  7,  5,  3,  1 },
        { 16, 14, 12, 10,  8,  6,  4,  2,  1,  3,  5,  7,  9, 11, 13, 15 },
        { 16, 14, 12, 10,  8,  6,  4,  2, 15, 13, 11,  9,  7,  5,  3,  1 },
    };

    for (auto& row : remove_patterns)
    {
        using tree_type = sfl::dtl::rb_tree<int, int, sfl::dtl::identity, std::less<int>, TPARAM_ALLOCATOR<int>, void>;
        using node_type = typename tree_type::node_type;
        using node_pointer = typename tree_type::node_pointer;

        tree_type tree;

        ///////////////////////////////////////////////////////////////////////

        std::vector<node_type> nodes(row.size());

        for (int i = 0; i < int(nodes.size()); ++i)
        {
            nodes[i].value_ = i+1;
        }

        CHECK(tree.begin() == tree.end());
        CHECK(tree.data_.root() == nullptr);
        CHECK(tree.data_.minimum() == tree.data_.header());

        ///////////////////////////////////////////////////////////////////////

        for (int i = 0; i < int(nodes.size()); ++i)
        {
            const auto res = tree.calculate_position_for_insert_equal(nodes[i].value_);
            tree.insert(&nodes[i], res.pos, res.left, tree.data_.root(), tree.data_.minimum());
        }

        CHECK(tree.begin() != tree.end());
        CHECK(tree.data_.root() != nullptr);
        CHECK(static_cast<node_pointer>(tree.data_.minimum())->value_ == 1);

        {
            int i = 1;
            for (auto it = tree.begin(), end = tree.end(); it != end; ++it, ++i)
            {
                CHECK(*it == i);
            }
        }

        ///////////////////////////////////////////////////////////////////////////

        {
            std::cout << "Content: [";
            bool first = true;
            for (auto& node : nodes)
            {
                if (!first) std::cout << ", ";
                first = false;
                std::cout << node.value_;
            }
            std::cout << "]" << std::endl;
        }
        {
            std::cout << "Remove pattern: [";
            bool first = true;
            for (auto& elem : row)
            {
                if (!first) std::cout << ", ";
                first = false;
                std::cout << elem;
            }
            std::cout << "]" << std::endl;
        }

        for (int i = 0; i < int(row.size()); ++i)
        {
            std::cout << "  - Removing " << row[i] << "..." << std::endl;

            std::vector<int> remaining_elements(row.begin() + i + 1, row.end());

            std::sort(remaining_elements.begin(), remaining_elements.end());

            {
                auto it = tree.find(row[i]);
                CHECK(it != tree.end());
                tree.remove(it.node_, tree.data_.root(), tree.data_.minimum());
            }

            if (i < int(row.size() - 1))
            {
                CHECK(tree.begin() != tree.end());
                CHECK(tree.data_.root() != nullptr);
                CHECK(static_cast<node_pointer>(tree.data_.minimum())->value_ == remaining_elements[0]);
            }
            else
            {
                CHECK(tree.begin() == tree.end());
                CHECK(tree.data_.root() == nullptr);
            }

            {
                auto it1 = tree.begin();
                auto it2 = remaining_elements.begin();
                while (it1 != tree.end())
                {
                    CHECK(*it1 == *it2);
                    ++it1;
                    ++it2;
                }
            }
        }

        std::cout << "  - DONE." << std::endl;

        ///////////////////////////////////////////////////////////////////////////

        tree.data_.reset();
    }
}

///////////////////////////////////////////////////////////////////////////////

PRINT("Test get_allocator()");
{
    sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void> tree;

    auto alloc = tree.get_allocator();

    (void)alloc;
}

///////////////////////////////////////////////////////////////////////////////

PRINT("Test key_comp()");
{
    {
        sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void> tree;

        auto key_comp = tree.key_comp();

        CHECK(key_comp(10, 10) == false);
        CHECK(key_comp(10, 20) == true);
        CHECK(key_comp(20, 10) == false);
        CHECK(key_comp(20, 20) == false);
    }

    {
        sfl::dtl::rb_tree<xobj, xobj, sfl::dtl::identity, xobj::less, TPARAM_ALLOCATOR<xobj>, void> tree;

        auto key_comp = tree.key_comp();

        CHECK(key_comp(xobj(10), 10) == false);
        CHECK(key_comp(xobj(10), 20) == true);
        CHECK(key_comp(xobj(20), 10) == false);
        CHECK(key_comp(xobj(20), 20) == false);
    }
}

///////////////////////////////////////////////////////////////////////////////

PRINT("Test lower_bound, upper_bound, equal_range, find, count, contains");
{
    /*
     *           (pointer to minumum value, i.e. 10)
     *           |
     *           H (header)
     *          /
     *        40
     *       /  \
     *     20    50
     *    /  \
     *  10    30b
     *       /  \
     *     30a   30c
     */

    using tree_type = sfl::dtl::rb_tree<int, int, sfl::dtl::identity, std::less<int>, TPARAM_ALLOCATOR<int>, void>;
    using node_type = typename tree_type::node_type;

    tree_type tree;
    node_type n10, n20, n30a, n30b, n30c, n40, n50;

    tree.data_.root() = &n40;
    tree.data_.minimum() = &n10;
    tree.data_.size_ = 7;

    n10.left_  = nullptr;   n10.right_  = nullptr;  n10.parent_  = &n20;
    n20.left_  = &n10;      n20.right_  = &n30b;    n20.parent_  = &n40;
    n30a.left_ = nullptr;   n30a.right_ = nullptr;  n30a.parent_ = &n30b;
    n30b.left_ = &n30a;     n30b.right_ = &n30c;    n30b.parent_ = &n20;
    n30c.left_ = nullptr;   n30c.right_ = nullptr;  n30c.parent_ = &n30b;
    n40.left_  = &n20;      n40.right_  = &n50;     n40.parent_  = tree.data_.header();
    n50.left_  = nullptr;   n50.right_  = nullptr;  n50.parent_  = &n40;

    n10.value_  = 10;
    n20.value_  = 20;
    n30a.value_ = 30;
    n30b.value_ = 30;
    n30c.value_ = 30;
    n40.value_  = 40;
    n50.value_  = 50;

    ///////////////////////////////////////////////////////////////////////////

    CHECK(tree.size() == 7);
    CHECK(*NTH(tree, 0) == 10);
    CHECK(*NTH(tree, 1) == 20);
    CHECK(*NTH(tree, 2) == 30);
    CHECK(*NTH(tree, 3) == 30);
    CHECK(*NTH(tree, 4) == 30);
    CHECK(*NTH(tree, 5) == 40);
    CHECK(*NTH(tree, 6) == 50);

    const auto& cref = tree;

    ///////////////////////////////////////////////////////////////////////////

    CHECK(tree.lower_bound( 5) == NTH(tree, 0));
    CHECK(tree.lower_bound(10) == NTH(tree, 0));
    CHECK(tree.lower_bound(15) == NTH(tree, 1));
    CHECK(tree.lower_bound(20) == NTH(tree, 1));
    CHECK(tree.lower_bound(25) == NTH(tree, 2));
    CHECK(tree.lower_bound(30) == NTH(tree, 2));
    CHECK(tree.lower_bound(35) == NTH(tree, 5));
    CHECK(tree.lower_bound(40) == NTH(tree, 5));
    CHECK(tree.lower_bound(45) == NTH(tree, 6));
    CHECK(tree.lower_bound(50) == NTH(tree, 6));
    CHECK(tree.lower_bound(55) == tree.end());

    CHECK(cref.lower_bound( 5) == NTH(cref, 0));
    CHECK(cref.lower_bound(10) == NTH(cref, 0));
    CHECK(cref.lower_bound(15) == NTH(cref, 1));
    CHECK(cref.lower_bound(20) == NTH(cref, 1));
    CHECK(cref.lower_bound(25) == NTH(cref, 2));
    CHECK(cref.lower_bound(30) == NTH(cref, 2));
    CHECK(cref.lower_bound(35) == NTH(cref, 5));
    CHECK(cref.lower_bound(40) == NTH(cref, 5));
    CHECK(cref.lower_bound(45) == NTH(cref, 6));
    CHECK(cref.lower_bound(50) == NTH(cref, 6));
    CHECK(cref.lower_bound(55) == cref.end());

    ///////////////////////////////////////////////////////////////////////////

    CHECK(tree.upper_bound( 5) == NTH(tree, 0));
    CHECK(tree.upper_bound(10) == NTH(tree, 1));
    CHECK(tree.upper_bound(15) == NTH(tree, 1));
    CHECK(tree.upper_bound(20) == NTH(tree, 2));
    CHECK(tree.upper_bound(25) == NTH(tree, 2));
    CHECK(tree.upper_bound(30) == NTH(tree, 5));
    CHECK(tree.upper_bound(35) == NTH(tree, 5));
    CHECK(tree.upper_bound(40) == NTH(tree, 6));
    CHECK(tree.upper_bound(45) == NTH(tree, 6));
    CHECK(tree.upper_bound(50) == tree.end());
    CHECK(tree.upper_bound(55) == tree.end());

    CHECK(cref.upper_bound( 5) == NTH(cref, 0));
    CHECK(cref.upper_bound(10) == NTH(cref, 1));
    CHECK(cref.upper_bound(15) == NTH(cref, 1));
    CHECK(cref.upper_bound(20) == NTH(cref, 2));
    CHECK(cref.upper_bound(25) == NTH(cref, 2));
    CHECK(cref.upper_bound(30) == NTH(cref, 5));
    CHECK(cref.upper_bound(35) == NTH(cref, 5));
    CHECK(cref.upper_bound(40) == NTH(cref, 6));
    CHECK(cref.upper_bound(45) == NTH(cref, 6));
    CHECK(cref.upper_bound(50) == cref.end());
    CHECK(cref.upper_bound(55) == cref.end());

    ///////////////////////////////////////////////////////////////////////////

    CHECK(tree.equal_range( 5) == std::make_pair(NTH(tree, 0), NTH(tree, 0)));
    CHECK(tree.equal_range(10) == std::make_pair(NTH(tree, 0), NTH(tree, 1)));
    CHECK(tree.equal_range(15) == std::make_pair(NTH(tree, 1), NTH(tree, 1)));
    CHECK(tree.equal_range(20) == std::make_pair(NTH(tree, 1), NTH(tree, 2)));
    CHECK(tree.equal_range(25) == std::make_pair(NTH(tree, 2), NTH(tree, 2)));
    CHECK(tree.equal_range(30) == std::make_pair(NTH(tree, 2), NTH(tree, 5)));
    CHECK(tree.equal_range(35) == std::make_pair(NTH(tree, 5), NTH(tree, 5)));
    CHECK(tree.equal_range(40) == std::make_pair(NTH(tree, 5), NTH(tree, 6)));
    CHECK(tree.equal_range(45) == std::make_pair(NTH(tree, 6), NTH(tree, 6)));
    CHECK(tree.equal_range(50) == std::make_pair(NTH(tree, 6), tree.end()));
    CHECK(tree.equal_range(55) == std::make_pair(tree.end(), tree.end()));

    CHECK(cref.equal_range( 5) == std::make_pair(NTH(cref, 0), NTH(cref, 0)));
    CHECK(cref.equal_range(10) == std::make_pair(NTH(cref, 0), NTH(cref, 1)));
    CHECK(cref.equal_range(15) == std::make_pair(NTH(cref, 1), NTH(cref, 1)));
    CHECK(cref.equal_range(20) == std::make_pair(NTH(cref, 1), NTH(cref, 2)));
    CHECK(cref.equal_range(25) == std::make_pair(NTH(cref, 2), NTH(cref, 2)));
    CHECK(cref.equal_range(30) == std::make_pair(NTH(cref, 2), NTH(cref, 5)));
    CHECK(cref.equal_range(35) == std::make_pair(NTH(cref, 5), NTH(cref, 5)));
    CHECK(cref.equal_range(40) == std::make_pair(NTH(cref, 5), NTH(cref, 6)));
    CHECK(cref.equal_range(45) == std::make_pair(NTH(cref, 6), NTH(cref, 6)));
    CHECK(cref.equal_range(50) == std::make_pair(NTH(cref, 6), cref.end()));
    CHECK(cref.equal_range(55) == std::make_pair(cref.end(), cref.end()));

    ///////////////////////////////////////////////////////////////////////////

    CHECK(tree.find( 5) == tree.end());
    CHECK(tree.find(10) == tree.lower_bound(10));
    CHECK(tree.find(15) == tree.end());
    CHECK(tree.find(20) == tree.lower_bound(20));
    CHECK(tree.find(25) == tree.end());
    CHECK(tree.find(30) == tree.lower_bound(30));
    CHECK(tree.find(35) == tree.end());
    CHECK(tree.find(40) == tree.lower_bound(40));
    CHECK(tree.find(45) == tree.end());
    CHECK(tree.find(50) == tree.lower_bound(50));
    CHECK(tree.find(55) == tree.end());

    CHECK(cref.find( 5) == cref.end());
    CHECK(cref.find(10) == cref.lower_bound(10));
    CHECK(cref.find(15) == cref.end());
    CHECK(cref.find(20) == cref.lower_bound(20));
    CHECK(cref.find(25) == cref.end());
    CHECK(cref.find(30) == cref.lower_bound(30));
    CHECK(cref.find(35) == cref.end());
    CHECK(cref.find(40) == cref.lower_bound(40));
    CHECK(cref.find(45) == cref.end());
    CHECK(cref.find(50) == cref.lower_bound(50));
    CHECK(cref.find(55) == cref.end());

    ///////////////////////////////////////////////////////////////////////////

    CHECK(tree.count( 5) == 0);
    CHECK(tree.count(10) == 1);
    CHECK(tree.count(15) == 0);
    CHECK(tree.count(20) == 1);
    CHECK(tree.count(25) == 0);
    CHECK(tree.count(30) == 3);
    CHECK(tree.count(35) == 0);
    CHECK(tree.count(40) == 1);
    CHECK(tree.count(45) == 0);
    CHECK(tree.count(50) == 1);
    CHECK(tree.count(55) == 0);

    CHECK(cref.count( 5) == 0);
    CHECK(cref.count(10) == 1);
    CHECK(cref.count(15) == 0);
    CHECK(cref.count(20) == 1);
    CHECK(cref.count(25) == 0);
    CHECK(cref.count(30) == 3);
    CHECK(cref.count(35) == 0);
    CHECK(cref.count(40) == 1);
    CHECK(cref.count(45) == 0);
    CHECK(cref.count(50) == 1);
    CHECK(cref.count(55) == 0);

    ///////////////////////////////////////////////////////////////////////////

    CHECK(tree.contains( 5) == false);
    CHECK(tree.contains(10) == true);
    CHECK(tree.contains(15) == false);
    CHECK(tree.contains(20) == true);
    CHECK(tree.contains(25) == false);
    CHECK(tree.contains(30) == true);
    CHECK(tree.contains(35) == false);
    CHECK(tree.contains(40) == true);
    CHECK(tree.contains(45) == false);
    CHECK(tree.contains(50) == true);
    CHECK(tree.contains(55) == false);

    CHECK(cref.contains( 5) == false);
    CHECK(cref.contains(10) == true);
    CHECK(cref.contains(15) == false);
    CHECK(cref.contains(20) == true);
    CHECK(cref.contains(25) == false);
    CHECK(cref.contains(30) == true);
    CHECK(cref.contains(35) == false);
    CHECK(cref.contains(40) == true);
    CHECK(cref.contains(45) == false);
    CHECK(cref.contains(50) == true);
    CHECK(cref.contains(55) == false);

    ///////////////////////////////////////////////////////////////////////////

    tree.data_.reset();
}

///////////////////////////////////////////////////////////////////////////////

PRINT("Test clear()");
{
    using tree_type = sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void>;
    using node_pointer = typename tree_type::node_pointer;

    tree_type tree;

    tree_type::make_node_functor make_node(tree);

    node_pointer n10 = make_node(10);
    node_pointer n20 = make_node(20);
    node_pointer n30 = make_node(30);
    node_pointer n40 = make_node(40);
    node_pointer n50 = make_node(50);
    node_pointer n60 = make_node(60);
    node_pointer n70 = make_node(70);

    tree.data_.root() = n40;
    tree.data_.minimum() = n10;
    tree.data_.size_ = 7;

    n10->left_ = nullptr;   n10->right_ = nullptr;  n10->parent_ = n20;
    n20->left_ = n10;       n20->right_ = n30;      n20->parent_ = n40;
    n30->left_ = nullptr;   n30->right_ = nullptr;  n30->parent_ = n20;
    n40->left_ = n20;       n40->right_ = n60;      n40->parent_ = tree.data_.header();
    n50->left_ = nullptr;   n50->right_ = nullptr;  n50->parent_ = n60;
    n60->left_ = n50;       n60->right_ = n70;      n60->parent_ = n40;
    n70->left_ = nullptr;   n70->right_ = nullptr;  n70->parent_ = n60;

    auto it = tree.begin();
    CHECK(*it == 10); ++it;
    CHECK(*it == 20); ++it;
    CHECK(*it == 30); ++it;
    CHECK(*it == 40); ++it;
    CHECK(*it == 50); ++it;
    CHECK(*it == 60); ++it;
    CHECK(*it == 70); ++it;
    CHECK(it == tree.end());

    ///////////////////////////////////////////////////////////////////////////

    tree.clear();

    CHECK(tree.begin() == tree.end());
    CHECK(tree.data_.root() == nullptr);
    CHECK(tree.data_.minimum() == tree.data_.header());
    CHECK(tree.data_.size_ == 0);
}

PRINT("Test emplace_equal(Args&&...)");
{
    {
        sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void> tree;

        CHECK(tree.empty() == true);
        CHECK(tree.size() == 0);
        CHECK(tree.max_size() > 0);

        {
            const auto res = tree.emplace_equal(10);

            CHECK(res != tree.end());
            CHECK(*res == 10);

            CHECK(tree.empty() == false);
            CHECK(tree.size() == 1);
        }

        {
            const auto res = tree.emplace_equal(10);

            CHECK(res != tree.end());
            CHECK(*res == 10);

            CHECK(tree.empty() == false);
            CHECK(tree.size() == 2);
        }
    }

    {
        sfl::dtl::rb_tree<xint, std::pair<const xint, xint>, sfl::dtl::first, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>, void> tree;

        CHECK(tree.empty() == true);
        CHECK(tree.size() == 0);
        CHECK(tree.max_size() > 0);

        {
            const auto res = tree.emplace_equal(10, 1000);

            CHECK(res != tree.end());
            CHECK(res->first == 10);
            CHECK(res->second == 1000);

            CHECK(tree.empty() == false);
            CHECK(tree.size() == 1);
        }

        {
            const auto res = tree.emplace_equal(10, 2000);

            CHECK(res != tree.end());
            CHECK(res->first == 10);
            CHECK(res->second == 2000);

            CHECK(tree.empty() == false);
            CHECK(tree.size() == 2);
        }
    }
}

PRINT("Test emplace_unique(Args&&...)");
{
    {
        sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void> tree;

        CHECK(tree.empty() == true);
        CHECK(tree.size() == 0);
        CHECK(tree.max_size() > 0);

        {
            const auto res = tree.emplace_unique(10);

            CHECK(res.second == true);
            CHECK(*res.first == 10);

            CHECK(tree.empty() == false);
            CHECK(tree.size() == 1);
        }

        {
            const auto res = tree.emplace_unique(10);

            CHECK(res.second == false);
            CHECK(*res.first == 10);

            CHECK(tree.empty() == false);
            CHECK(tree.size() == 1);
        }
    }

    {
        sfl::dtl::rb_tree<xint, std::pair<const xint, xint>, sfl::dtl::first, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>, void> tree;

        CHECK(tree.empty() == true);
        CHECK(tree.size() == 0);
        CHECK(tree.max_size() > 0);

        {
            const auto res = tree.emplace_unique(10, 1000);

            CHECK(res.second == true);
            CHECK(res.first->first == 10);
            CHECK(res.first->second == 1000);

            CHECK(tree.empty() == false);
            CHECK(tree.size() == 1);
        }

        {
            const auto res = tree.emplace_unique(10, 2000);

            CHECK(res.second == false);
            CHECK(res.first->first == 10);
            CHECK(res.first->second == 1000);

            CHECK(tree.empty() == false);
            CHECK(tree.size() == 1);
        }
    }
}

PRINT("Test emplace_hint_equal(const_iterator, Args&&...)");
{
    {
        sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void> tree;

        CHECK(tree.empty() == true);
        CHECK(tree.size() == 0);
        CHECK(tree.max_size() > 0);

        {
            const auto hint = tree.begin();
            const auto res = tree.emplace_hint_equal(hint, 10);

            CHECK(res != tree.end());
            CHECK(*res == 10);

            CHECK(tree.empty() == false);
            CHECK(tree.size() == 1);
        }

        {
            const auto hint = tree.begin();
            const auto res = tree.emplace_hint_equal(hint, 10);

            CHECK(res != tree.end());
            CHECK(*res == 10);

            CHECK(tree.empty() == false);
            CHECK(tree.size() == 2);
        }
    }

    {
        sfl::dtl::rb_tree<xint, std::pair<const xint, xint>, sfl::dtl::first, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>, void> tree;

        CHECK(tree.empty() == true);
        CHECK(tree.size() == 0);
        CHECK(tree.max_size() > 0);

        {
            const auto hint = tree.begin();
            const auto res = tree.emplace_hint_equal(hint, 10, 1000);

            CHECK(res != tree.end());
            CHECK(res->first == 10);
            CHECK(res->second == 1000);

            CHECK(tree.empty() == false);
            CHECK(tree.size() == 1);
        }

        {
            const auto hint = tree.begin();
            const auto res = tree.emplace_hint_equal(hint, 10, 2000);

            CHECK(res != tree.end());
            CHECK(res->first == 10);
            CHECK(res->second == 2000);

            CHECK(tree.empty() == false);
            CHECK(tree.size() == 2);
        }
    }
}

PRINT("Test emplace_hint_unique(const_iterator, Args&&...)");
{
    {
        sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void> tree;

        CHECK(tree.empty() == true);
        CHECK(tree.size() == 0);
        CHECK(tree.max_size() > 0);

        {
            const auto hint = tree.begin();
            const auto res = tree.emplace_hint_unique(hint, 10);

            CHECK(*res == 10);

            CHECK(tree.empty() == false);
            CHECK(tree.size() == 1);
        }

        {
            const auto hint = tree.begin();
            const auto res = tree.emplace_hint_unique(hint, 10);

            CHECK(*res == 10);

            CHECK(tree.empty() == false);
            CHECK(tree.size() == 1);
        }
    }

    {
        sfl::dtl::rb_tree<xint, std::pair<const xint, xint>, sfl::dtl::first, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>, void> tree;

        CHECK(tree.empty() == true);
        CHECK(tree.size() == 0);
        CHECK(tree.max_size() > 0);

        {
            const auto hint = tree.begin();
            const auto res = tree.emplace_hint_unique(hint, 10, 1000);

            CHECK(res->first == 10);
            CHECK(res->second == 1000);

            CHECK(tree.empty() == false);
            CHECK(tree.size() == 1);
        }

        {
            const auto hint = tree.begin();
            const auto res = tree.emplace_hint_unique(hint, 10, 2000);

            CHECK(res->first == 10);
            CHECK(res->second == 1000);

            CHECK(tree.empty() == false);
            CHECK(tree.size() == 1);
        }
    }
}

PRINT("Test insert_equal(V&&)");
{
    {
        sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void> tree;

        CHECK(tree.empty() == true);
        CHECK(tree.size() == 0);
        CHECK(tree.max_size() > 0);

        {
            const auto res = tree.insert_equal(10);

            CHECK(res != tree.end());
            CHECK(*res == 10);

            CHECK(tree.empty() == false);
            CHECK(tree.size() == 1);
        }

        {
            const auto res = tree.insert_equal(10);

            CHECK(res != tree.end());
            CHECK(*res == 10);

            CHECK(tree.empty() == false);
            CHECK(tree.size() == 2);
        }
    }

    {
        sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void> tree;

        CHECK(tree.empty() == true);
        CHECK(tree.size() == 0);
        CHECK(tree.max_size() > 0);

        {
            xint value_10(10);

            const auto res = tree.insert_equal(std::move(value_10));

            CHECK(res != tree.end());
            CHECK(*res == 10);

            CHECK(tree.empty() == false);
            CHECK(tree.size() == 1);

            CHECK(value_10 == -10);
        }

        {
            xint value_10(10);

            const auto res = tree.insert_equal(std::move(value_10));

            CHECK(res != tree.end());
            CHECK(*res == 10);

            CHECK(tree.empty() == false);
            CHECK(tree.size() == 2);

            CHECK(value_10 == -10);
        }
    }

    {
        sfl::dtl::rb_tree<xint, std::pair<const xint, xint>, sfl::dtl::first, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>, void> tree;

        CHECK(tree.empty() == true);
        CHECK(tree.size() == 0);
        CHECK(tree.max_size() > 0);

        {
            const auto res = tree.insert_equal(std::make_pair(10, 1000));

            CHECK(res != tree.end());
            CHECK(res->first == 10);
            CHECK(res->second == 1000);

            CHECK(tree.empty() == false);
            CHECK(tree.size() == 1);
        }

        {
            const auto res = tree.insert_equal(std::make_pair(10, 2000));

            CHECK(res != tree.end());
            CHECK(res->first == 10);
            CHECK(res->second == 2000);

            CHECK(tree.empty() == false);
            CHECK(tree.size() == 2);
        }
    }
}

PRINT("Test insert_unique(V&&)");
{
    {
        sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void> tree;

        CHECK(tree.empty() == true);
        CHECK(tree.size() == 0);
        CHECK(tree.max_size() > 0);

        {
            const auto res = tree.insert_unique(10);

            CHECK(res.second == true);
            CHECK(*res.first == 10);

            CHECK(tree.empty() == false);
            CHECK(tree.size() == 1);
        }

        {
            const auto res = tree.insert_unique(10);

            CHECK(res.second == false);
            CHECK(*res.first == 10);

            CHECK(tree.empty() == false);
            CHECK(tree.size() == 1);
        }
    }

    {
        sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void> tree;

        CHECK(tree.empty() == true);
        CHECK(tree.size() == 0);
        CHECK(tree.max_size() > 0);

        {
            xint value_10(10);

            const auto res = tree.insert_unique(std::move(value_10));

            CHECK(res.second == true);
            CHECK(*res.first == 10);

            CHECK(tree.empty() == false);
            CHECK(tree.size() == 1);

            CHECK(value_10 == -10);
        }

        {
            xint value_10(10);

            const auto res = tree.insert_unique(std::move(value_10));

            CHECK(res.second == false);
            CHECK(*res.first == 10);

            CHECK(tree.empty() == false);
            CHECK(tree.size() == 1);

            CHECK(value_10 == 10);
        }
    }

    {
        sfl::dtl::rb_tree<xint, std::pair<const xint, xint>, sfl::dtl::first, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>, void> tree;

        CHECK(tree.empty() == true);
        CHECK(tree.size() == 0);
        CHECK(tree.max_size() > 0);

        {
            const auto res = tree.insert_unique(std::make_pair(10, 1000));

            CHECK(res.second == true);
            CHECK(res.first->first == 10);
            CHECK(res.first->second == 1000);

            CHECK(tree.empty() == false);
            CHECK(tree.size() == 1);
        }

        {
            const auto res = tree.insert_unique(std::make_pair(10, 2000));

            CHECK(res.second == false);
            CHECK(res.first->first == 10);
            CHECK(res.first->second == 1000);

            CHECK(tree.empty() == false);
            CHECK(tree.size() == 1);
        }
    }
}

PRINT("Test insert_hint_equal(const_iterator, V&&)");
{
    {
        sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void> tree;

        CHECK(tree.empty() == true);
        CHECK(tree.size() == 0);
        CHECK(tree.max_size() > 0);

        {
            const auto hint = tree.begin();
            const auto res = tree.insert_hint_equal(hint, 10);

            CHECK(res != tree.end());
            CHECK(*res == 10);

            CHECK(tree.empty() == false);
            CHECK(tree.size() == 1);
        }

        {
            const auto hint = tree.begin();
            const auto res = tree.insert_hint_equal(hint, 10);

            CHECK(res != tree.end());
            CHECK(*res == 10);

            CHECK(tree.empty() == false);
            CHECK(tree.size() == 2);
        }
    }

    {
        sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void> tree;

        CHECK(tree.empty() == true);
        CHECK(tree.size() == 0);
        CHECK(tree.max_size() > 0);

        {
            xint value_10(10);

            const auto hint = tree.begin();
            const auto res = tree.insert_hint_equal(hint, std::move(value_10));

            CHECK(res != tree.end());
            CHECK(*res == 10);

            CHECK(tree.empty() == false);
            CHECK(tree.size() == 1);

            CHECK(value_10 == -10);
        }

        {
            xint value_10(10);

            const auto hint = tree.begin();
            const auto res = tree.insert_hint_equal(hint, std::move(value_10));

            CHECK(res != tree.end());
            CHECK(*res == 10);

            CHECK(tree.empty() == false);
            CHECK(tree.size() == 2);

            CHECK(value_10 == -10);
        }
    }

    {
        sfl::dtl::rb_tree<xint, std::pair<const xint, xint>, sfl::dtl::first, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>, void> tree;

        CHECK(tree.empty() == true);
        CHECK(tree.size() == 0);
        CHECK(tree.max_size() > 0);

        {
            const auto hint = tree.begin();
            const auto res = tree.insert_hint_equal(hint, std::make_pair(10, 1000));

            CHECK(res != tree.end());
            CHECK(res->first == 10);
            CHECK(res->second == 1000);

            CHECK(tree.empty() == false);
            CHECK(tree.size() == 1);
        }

        {
            const auto hint = tree.begin();
            const auto res = tree.insert_hint_equal(hint, std::make_pair(10, 2000));

            CHECK(res != tree.end());
            CHECK(res->first == 10);
            CHECK(res->second == 2000);

            CHECK(tree.empty() == false);
            CHECK(tree.size() == 2);
        }
    }
}

PRINT("Test insert_hint_unique(const_iterator, V&&)");
{
    {
        sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void> tree;

        CHECK(tree.empty() == true);
        CHECK(tree.size() == 0);
        CHECK(tree.max_size() > 0);

        {
            const auto hint = tree.begin();
            const auto res = tree.insert_hint_unique(hint, 10);

            CHECK(*res == 10);

            CHECK(tree.empty() == false);
            CHECK(tree.size() == 1);
        }

        {
            const auto hint = tree.begin();
            const auto res = tree.insert_hint_unique(hint, 10);

            CHECK(*res == 10);

            CHECK(tree.empty() == false);
            CHECK(tree.size() == 1);
        }
    }

    {
        sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void> tree;

        CHECK(tree.empty() == true);
        CHECK(tree.size() == 0);
        CHECK(tree.max_size() > 0);

        {
            xint value_10(10);

            const auto hint = tree.begin();
            const auto res = tree.insert_hint_unique(hint, std::move(value_10));

            CHECK(*res == 10);

            CHECK(tree.empty() == false);
            CHECK(tree.size() == 1);

            CHECK(value_10 == -10);
        }

        {
            xint value_10(10);

            const auto hint = tree.begin();
            const auto res = tree.insert_hint_unique(hint, std::move(value_10));

            CHECK(*res == 10);

            CHECK(tree.empty() == false);
            CHECK(tree.size() == 1);

            CHECK(value_10 == 10);
        }
    }

    {
        sfl::dtl::rb_tree<xint, std::pair<const xint, xint>, sfl::dtl::first, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>, void> tree;

        CHECK(tree.empty() == true);
        CHECK(tree.size() == 0);
        CHECK(tree.max_size() > 0);

        {
            const auto hint = tree.begin();
            const auto res = tree.insert_hint_unique(hint, std::make_pair(10, 1000));

            CHECK(res->first == 10);
            CHECK(res->second == 1000);

            CHECK(tree.empty() == false);
            CHECK(tree.size() == 1);
        }

        {
            const auto hint = tree.begin();
            const auto res = tree.insert_hint_unique(hint, std::make_pair(10, 2000));

            CHECK(res->first == 10);
            CHECK(res->second == 1000);

            CHECK(tree.empty() == false);
            CHECK(tree.size() == 1);
        }
    }
}

PRINT("Test erase(const_iterator)");
{
    {
        sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void> tree;

        CHECK(tree.empty() == true);
        CHECK(tree.size() == 0);
        CHECK(tree.max_size() > 0);

        tree.emplace_equal(10);
        tree.emplace_equal(20);
        tree.emplace_equal(30);
        tree.emplace_equal(40);
        tree.emplace_equal(50);

        CHECK(tree.size() == 5);
        CHECK(*NTH(tree, 0) == 10);
        CHECK(*NTH(tree, 1) == 20);
        CHECK(*NTH(tree, 2) == 30);
        CHECK(*NTH(tree, 3) == 40);
        CHECK(*NTH(tree, 4) == 50);

        {
            const auto res = tree.erase(NTH(tree, 2));

            CHECK(*res == 40);
            CHECK(tree.size() == 4);
            CHECK(*NTH(tree, 0) == 10);
            CHECK(*NTH(tree, 1) == 20);
            CHECK(*NTH(tree, 2) == 40);
            CHECK(*NTH(tree, 3) == 50);
        }

        {
            const auto res = tree.erase(NTH(tree, 3));

            CHECK(res == tree.end());
            CHECK(tree.size() == 3);
            CHECK(*NTH(tree, 0) == 10);
            CHECK(*NTH(tree, 1) == 20);
            CHECK(*NTH(tree, 2) == 40);
        }
    }

    {
        sfl::dtl::rb_tree<xint, std::pair<const xint, xint>, sfl::dtl::first, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>, void> tree;

        CHECK(tree.empty() == true);
        CHECK(tree.size() == 0);
        CHECK(tree.max_size() > 0);

        tree.emplace_equal(10, 1);
        tree.emplace_equal(20, 1);
        tree.emplace_equal(30, 1);
        tree.emplace_equal(40, 1);
        tree.emplace_equal(50, 1);

        CHECK(tree.size() == 5);
        CHECK(NTH(tree, 0)->first == 10); CHECK(NTH(tree, 0)->second == 1);
        CHECK(NTH(tree, 1)->first == 20); CHECK(NTH(tree, 1)->second == 1);
        CHECK(NTH(tree, 2)->first == 30); CHECK(NTH(tree, 2)->second == 1);
        CHECK(NTH(tree, 3)->first == 40); CHECK(NTH(tree, 3)->second == 1);
        CHECK(NTH(tree, 4)->first == 50); CHECK(NTH(tree, 4)->second == 1);

        {
            const auto res = tree.erase(NTH(tree, 2));

            CHECK(res->first == 40 && res->second == 1);
            CHECK(tree.size() == 4);
            CHECK(NTH(tree, 0)->first == 10); CHECK(NTH(tree, 0)->second == 1);
            CHECK(NTH(tree, 1)->first == 20); CHECK(NTH(tree, 1)->second == 1);
            CHECK(NTH(tree, 2)->first == 40); CHECK(NTH(tree, 2)->second == 1);
            CHECK(NTH(tree, 3)->first == 50); CHECK(NTH(tree, 3)->second == 1);
        }

        {
            const auto res = tree.erase(NTH(tree, 3));

            CHECK(res == tree.end());
            CHECK(tree.size() == 3);
            CHECK(NTH(tree, 0)->first == 10); CHECK(NTH(tree, 0)->second == 1);
            CHECK(NTH(tree, 1)->first == 20); CHECK(NTH(tree, 1)->second == 1);
            CHECK(NTH(tree, 2)->first == 40); CHECK(NTH(tree, 2)->second == 1);
        }
    }
}

PRINT("Test erase(const_iterator, const_iterator)");
{
    {
        sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void> tree;

        CHECK(tree.empty() == true);
        CHECK(tree.size() == 0);
        CHECK(tree.max_size() > 0);

        tree.emplace_equal(10);
        tree.emplace_equal(20);
        tree.emplace_equal(30);
        tree.emplace_equal(40);
        tree.emplace_equal(50);

        CHECK(tree.size() == 5);
        CHECK(*NTH(tree, 0) == 10);
        CHECK(*NTH(tree, 1) == 20);
        CHECK(*NTH(tree, 2) == 30);
        CHECK(*NTH(tree, 3) == 40);
        CHECK(*NTH(tree, 4) == 50);

        {
            const auto res = tree.erase(NTH(tree, 1), NTH(tree, 3));

            CHECK(*res == 40);
            CHECK(tree.size() == 3);
            CHECK(*NTH(tree, 0) == 10);
            CHECK(*NTH(tree, 1) == 40);
            CHECK(*NTH(tree, 2) == 50);
        }

        {
            const auto res = tree.erase(NTH(tree, 1), NTH(tree, 3));

            CHECK(res == tree.end());
            CHECK(tree.size() == 1);
            CHECK(*NTH(tree, 0) == 10);
        }
    }

    {
        sfl::dtl::rb_tree<xint, std::pair<const xint, xint>, sfl::dtl::first, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>, void> tree;

        CHECK(tree.empty() == true);
        CHECK(tree.size() == 0);
        CHECK(tree.max_size() > 0);

        tree.emplace_equal(10, 1);
        tree.emplace_equal(20, 1);
        tree.emplace_equal(30, 1);
        tree.emplace_equal(40, 1);
        tree.emplace_equal(50, 1);

        CHECK(tree.size() == 5);
        CHECK(NTH(tree, 0)->first == 10); CHECK(NTH(tree, 0)->second == 1);
        CHECK(NTH(tree, 1)->first == 20); CHECK(NTH(tree, 1)->second == 1);
        CHECK(NTH(tree, 2)->first == 30); CHECK(NTH(tree, 2)->second == 1);
        CHECK(NTH(tree, 3)->first == 40); CHECK(NTH(tree, 3)->second == 1);
        CHECK(NTH(tree, 4)->first == 50); CHECK(NTH(tree, 4)->second == 1);

        {
            const auto res = tree.erase(NTH(tree, 1), NTH(tree, 3));

            CHECK(res->first == 40 && res->second == 1);
            CHECK(tree.size() == 3);
            CHECK(NTH(tree, 0)->first == 10); CHECK(NTH(tree, 0)->second == 1);
            CHECK(NTH(tree, 1)->first == 40); CHECK(NTH(tree, 1)->second == 1);
            CHECK(NTH(tree, 2)->first == 50); CHECK(NTH(tree, 2)->second == 1);
        }

        {
            const auto res = tree.erase(NTH(tree, 1), NTH(tree, 3));

            CHECK(res == tree.end());
            CHECK(tree.size() == 1);
            CHECK(NTH(tree, 0)->first == 10); CHECK(NTH(tree, 0)->second == 1);
        }
    }
}

PRINT("Test erase(const K&)");
{
    {
        sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void> tree;

        CHECK(tree.empty() == true);
        CHECK(tree.size() == 0);
        CHECK(tree.max_size() > 0);

        tree.emplace_equal(10);
        tree.emplace_equal(20);
        tree.emplace_equal(20);
        tree.emplace_equal(30);
        tree.emplace_equal(30);
        tree.emplace_equal(30);

        CHECK(tree.size() == 6);
        CHECK(*NTH(tree, 0) == 10);
        CHECK(*NTH(tree, 1) == 20);
        CHECK(*NTH(tree, 2) == 20);
        CHECK(*NTH(tree, 3) == 30);
        CHECK(*NTH(tree, 4) == 30);
        CHECK(*NTH(tree, 5) == 30);

        {
            CHECK(tree.erase(10) == 1);
            CHECK(tree.erase(10) == 0);
            CHECK(tree.size() == 5);
            CHECK(*NTH(tree, 0) == 20);
            CHECK(*NTH(tree, 1) == 20);
            CHECK(*NTH(tree, 2) == 30);
            CHECK(*NTH(tree, 3) == 30);
            CHECK(*NTH(tree, 4) == 30);
        }

        {
            CHECK(tree.erase(30) == 3);
            CHECK(tree.erase(30) == 0);
            CHECK(tree.size() == 2);
            CHECK(*NTH(tree, 0) == 20);
            CHECK(*NTH(tree, 1) == 20);
        }

        {
            CHECK(tree.erase(20) == 2);
            CHECK(tree.erase(20) == 0);
            CHECK(tree.size() == 0);
        }
    }

    {
        sfl::dtl::rb_tree<xint, std::pair<const xint, xint>, sfl::dtl::first, std::less<xint>, TPARAM_ALLOCATOR<std::pair<const xint, xint>>, void> tree;

        CHECK(tree.empty() == true);
        CHECK(tree.size() == 0);
        CHECK(tree.max_size() > 0);

        tree.emplace_equal(10, 1);
        tree.emplace_equal(20, 1);
        tree.emplace_equal(20, 1);
        tree.emplace_equal(30, 1);
        tree.emplace_equal(30, 1);
        tree.emplace_equal(30, 1);

        CHECK(tree.size() == 6);
        CHECK(NTH(tree, 0)->first == 10); CHECK(NTH(tree, 0)->second == 1);
        CHECK(NTH(tree, 1)->first == 20); CHECK(NTH(tree, 1)->second == 1);
        CHECK(NTH(tree, 2)->first == 20); CHECK(NTH(tree, 2)->second == 1);
        CHECK(NTH(tree, 3)->first == 30); CHECK(NTH(tree, 3)->second == 1);
        CHECK(NTH(tree, 4)->first == 30); CHECK(NTH(tree, 4)->second == 1);
        CHECK(NTH(tree, 5)->first == 30); CHECK(NTH(tree, 5)->second == 1);

        {
            CHECK(tree.erase(10) == 1);
            CHECK(tree.erase(10) == 0);
            CHECK(tree.size() == 5);
            CHECK(NTH(tree, 0)->first == 20); CHECK(NTH(tree, 0)->second == 1);
            CHECK(NTH(tree, 1)->first == 20); CHECK(NTH(tree, 1)->second == 1);
            CHECK(NTH(tree, 2)->first == 30); CHECK(NTH(tree, 2)->second == 1);
            CHECK(NTH(tree, 3)->first == 30); CHECK(NTH(tree, 3)->second == 1);
            CHECK(NTH(tree, 4)->first == 30); CHECK(NTH(tree, 4)->second == 1);
        }

        {
            CHECK(tree.erase(30) == 3);
            CHECK(tree.erase(30) == 0);
            CHECK(tree.size() == 2);
            CHECK(NTH(tree, 0)->first == 20); CHECK(NTH(tree, 0)->second == 1);
            CHECK(NTH(tree, 1)->first == 20); CHECK(NTH(tree, 1)->second == 1);
        }

        {
            CHECK(tree.erase(20) == 2);
            CHECK(tree.erase(20) == 0);
            CHECK(tree.size() == 0);
        }
    }
}

PRINT("Test swap(container&)");
{
    #define CONDITION tree1.size() == 0 && tree2.size() == 0
    {
        sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void> tree1, tree2;

        CHECK(tree1.size() == 0);
        CHECK(tree2.size() == 0);

        ///////////////////////////////////////////////////////////////////////

        CHECK(CONDITION);

        tree1.swap(tree2);

        CHECK(tree1.size() == 0);
        CHECK(tree2.size() == 0);
    }
    #undef CONDITION

    #define CONDITION tree1.size() == 0 && tree2.size() != 0
    {
        sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void> tree1, tree2;

        tree2.emplace_unique(10);
        tree2.emplace_unique(20);
        tree2.emplace_unique(30);
        tree2.emplace_unique(40);
        tree2.emplace_unique(50);
        tree2.emplace_unique(60);
        tree2.emplace_unique(70);
        tree2.emplace_unique(80);
        tree2.emplace_unique(90);

        CHECK(tree1.size() == 0);

        CHECK(tree2.size() == 9);
        CHECK(*NTH(tree2, 0) == 10);
        CHECK(*NTH(tree2, 1) == 20);
        CHECK(*NTH(tree2, 2) == 30);
        CHECK(*NTH(tree2, 3) == 40);
        CHECK(*NTH(tree2, 4) == 50);
        CHECK(*NTH(tree2, 5) == 60);
        CHECK(*NTH(tree2, 6) == 70);
        CHECK(*NTH(tree2, 7) == 80);
        CHECK(*NTH(tree2, 8) == 90);

        ///////////////////////////////////////////////////////////////////////

        CHECK(CONDITION);

        tree1.swap(tree2);

        CHECK(tree1.size() == 9);
        CHECK(*NTH(tree1, 0) == 10);
        CHECK(*NTH(tree1, 1) == 20);
        CHECK(*NTH(tree1, 2) == 30);
        CHECK(*NTH(tree1, 3) == 40);
        CHECK(*NTH(tree1, 4) == 50);
        CHECK(*NTH(tree1, 5) == 60);
        CHECK(*NTH(tree1, 6) == 70);
        CHECK(*NTH(tree1, 7) == 80);
        CHECK(*NTH(tree1, 8) == 90);
        CHECK(tree2.size() == 0);
    }
    #undef CONDITION

    #define CONDITION tree1.size() != 0 && tree2.size() == 0
    {
        sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void> tree1, tree2;

        tree1.emplace_unique(10);
        tree1.emplace_unique(20);
        tree1.emplace_unique(30);
        tree1.emplace_unique(40);
        tree1.emplace_unique(50);
        tree1.emplace_unique(60);
        tree1.emplace_unique(70);
        tree1.emplace_unique(80);
        tree1.emplace_unique(90);

        CHECK(tree1.size() == 9);
        CHECK(*NTH(tree1, 0) == 10);
        CHECK(*NTH(tree1, 1) == 20);
        CHECK(*NTH(tree1, 2) == 30);
        CHECK(*NTH(tree1, 3) == 40);
        CHECK(*NTH(tree1, 4) == 50);
        CHECK(*NTH(tree1, 5) == 60);
        CHECK(*NTH(tree1, 6) == 70);
        CHECK(*NTH(tree1, 7) == 80);
        CHECK(*NTH(tree1, 8) == 90);

        CHECK(tree2.size() == 0);

        ///////////////////////////////////////////////////////////////////////

        CHECK(CONDITION);

        tree1.swap(tree2);

        CHECK(tree1.size() == 0);
        CHECK(tree2.size() == 9);
        CHECK(*NTH(tree2, 0) == 10);
        CHECK(*NTH(tree2, 1) == 20);
        CHECK(*NTH(tree2, 2) == 30);
        CHECK(*NTH(tree2, 3) == 40);
        CHECK(*NTH(tree2, 4) == 50);
        CHECK(*NTH(tree2, 5) == 60);
        CHECK(*NTH(tree2, 6) == 70);
        CHECK(*NTH(tree2, 7) == 80);
        CHECK(*NTH(tree2, 8) == 90);
    }
    #undef CONDITION

    #define CONDITION tree1.size() != 0 && tree2.size() != 0 && tree1.size() < tree2.size()
    {
        sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void> tree1, tree2;

        tree1.emplace_unique(10);
        tree1.emplace_unique(20);
        tree1.emplace_unique(30);

        tree2.emplace_unique(40);
        tree2.emplace_unique(50);
        tree2.emplace_unique(60);
        tree2.emplace_unique(70);
        tree2.emplace_unique(80);
        tree2.emplace_unique(90);

        CHECK(tree1.size() == 3);
        CHECK(*NTH(tree1, 0) == 10);
        CHECK(*NTH(tree1, 1) == 20);
        CHECK(*NTH(tree1, 2) == 30);

        CHECK(tree2.size() == 6);
        CHECK(*NTH(tree2, 0) == 40);
        CHECK(*NTH(tree2, 1) == 50);
        CHECK(*NTH(tree2, 2) == 60);
        CHECK(*NTH(tree2, 3) == 70);
        CHECK(*NTH(tree2, 4) == 80);
        CHECK(*NTH(tree2, 5) == 90);

        ///////////////////////////////////////////////////////////////////////

        CHECK(CONDITION);

        tree1.swap(tree2);

        CHECK(tree1.size() == 6);
        CHECK(*NTH(tree1, 0) == 40);
        CHECK(*NTH(tree1, 1) == 50);
        CHECK(*NTH(tree1, 2) == 60);
        CHECK(*NTH(tree1, 3) == 70);
        CHECK(*NTH(tree1, 4) == 80);
        CHECK(*NTH(tree1, 5) == 90);

        CHECK(tree2.size() == 3);
        CHECK(*NTH(tree2, 0) == 10);
        CHECK(*NTH(tree2, 1) == 20);
        CHECK(*NTH(tree2, 2) == 30);
    }
    #undef CONDITION

    #define CONDITION tree1.size() != 0 && tree2.size() != 0 && tree1.size() == tree2.size()
    {
        sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void> tree1, tree2;

        tree1.emplace_unique(10);
        tree1.emplace_unique(20);
        tree1.emplace_unique(30);
        tree1.emplace_unique(40);

        tree2.emplace_unique(50);
        tree2.emplace_unique(60);
        tree2.emplace_unique(70);
        tree2.emplace_unique(80);

        CHECK(tree1.size() == 4);
        CHECK(*NTH(tree1, 0) == 10);
        CHECK(*NTH(tree1, 1) == 20);
        CHECK(*NTH(tree1, 2) == 30);
        CHECK(*NTH(tree1, 3) == 40);

        CHECK(tree2.size() == 4);
        CHECK(*NTH(tree2, 0) == 50);
        CHECK(*NTH(tree2, 1) == 60);
        CHECK(*NTH(tree2, 2) == 70);
        CHECK(*NTH(tree2, 3) == 80);

        ///////////////////////////////////////////////////////////////////////

        CHECK(CONDITION);

        PRINT("SWAP BEGIN");
        tree1.swap(tree2);
        PRINT("SWAP END");

        CHECK(tree1.size() == 4);
        CHECK(*NTH(tree1, 0) == 50);
        CHECK(*NTH(tree1, 1) == 60);
        CHECK(*NTH(tree1, 2) == 70);
        CHECK(*NTH(tree1, 3) == 80);

        CHECK(tree2.size() == 4);
        CHECK(*NTH(tree2, 0) == 10);
        CHECK(*NTH(tree2, 1) == 20);
        CHECK(*NTH(tree2, 2) == 30);
        CHECK(*NTH(tree2, 3) == 40);
    }
    #undef CONDITION

    #define CONDITION tree1.size() != 0 && tree2.size() != 0 && tree1.size() > tree2.size()
    {
        sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void> tree1, tree2;

        tree1.emplace_unique(10);
        tree1.emplace_unique(20);
        tree1.emplace_unique(30);
        tree1.emplace_unique(40);
        tree1.emplace_unique(50);
        tree1.emplace_unique(60);

        tree2.emplace_unique(70);
        tree2.emplace_unique(80);
        tree2.emplace_unique(90);

        CHECK(tree1.size() == 6);
        CHECK(*NTH(tree1, 0) == 10);
        CHECK(*NTH(tree1, 1) == 20);
        CHECK(*NTH(tree1, 2) == 30);
        CHECK(*NTH(tree1, 3) == 40);
        CHECK(*NTH(tree1, 4) == 50);
        CHECK(*NTH(tree1, 5) == 60);

        CHECK(tree2.size() == 3);
        CHECK(*NTH(tree2, 0) == 70);
        CHECK(*NTH(tree2, 1) == 80);
        CHECK(*NTH(tree2, 2) == 90);

        ///////////////////////////////////////////////////////////////////////

        CHECK(CONDITION);

        tree1.swap(tree2);

        CHECK(tree1.size() == 3);
        CHECK(*NTH(tree1, 0) == 70);
        CHECK(*NTH(tree1, 1) == 80);
        CHECK(*NTH(tree1, 2) == 90);

        CHECK(tree2.size() == 6);
        CHECK(*NTH(tree2, 0) == 10);
        CHECK(*NTH(tree2, 1) == 20);
        CHECK(*NTH(tree2, 2) == 30);
        CHECK(*NTH(tree2, 3) == 40);
        CHECK(*NTH(tree2, 4) == 50);
        CHECK(*NTH(tree2, 5) == 60);
    }
    #undef CONDITION
}

///////////////////////////////////////////////////////////////////////////////

PRINT("Test container()");
{
    sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void> tree;

    CHECK(tree.size() == 0);
}

PRINT("Test container(const KeyCompare&)");
{
    std::less<xint> comp;

    sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void> tree(comp);

    CHECK(tree.size() == 0);
}

PRINT("Test container(const Allocator&)");
{
    TPARAM_ALLOCATOR<std::pair<xint, xint>> alloc;

    sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void> tree(alloc);

    CHECK(tree.size() == 0);
}

PRINT("Test container(const KeyCompare&, const Allocator&)");
{
    std::less<xint> comp;

    TPARAM_ALLOCATOR<std::pair<xint, xint>> alloc;

    sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void> tree(comp, alloc);

    CHECK(tree.size() == 0);
}

PRINT("Test container(const container&)");
{
    {
        sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void> tree1;

        CHECK(tree1.size() == 0);

        ///////////////////////////////////////////////////////////////////////

        sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void> tree2(tree1);

        CHECK(tree2.size() == 0);
    }

    {
        sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void> tree1;

        tree1.emplace_unique(10);
        tree1.emplace_unique(20);
        tree1.emplace_unique(30);
        tree1.emplace_unique(40);
        tree1.emplace_unique(50);
        tree1.emplace_unique(60);
        tree1.emplace_unique(70);
        tree1.emplace_unique(80);
        tree1.emplace_unique(90);

        CHECK(tree1.size() == 9);
        CHECK(*NTH(tree1, 0) == 10);
        CHECK(*NTH(tree1, 1) == 20);
        CHECK(*NTH(tree1, 2) == 30);
        CHECK(*NTH(tree1, 3) == 40);
        CHECK(*NTH(tree1, 4) == 50);
        CHECK(*NTH(tree1, 5) == 60);
        CHECK(*NTH(tree1, 6) == 70);
        CHECK(*NTH(tree1, 7) == 80);
        CHECK(*NTH(tree1, 8) == 90);

        ///////////////////////////////////////////////////////////////////////

        sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void> tree2(tree1);

        CHECK(tree2.size() == 9);
        CHECK(*NTH(tree2, 0) == 10);
        CHECK(*NTH(tree2, 1) == 20);
        CHECK(*NTH(tree2, 2) == 30);
        CHECK(*NTH(tree2, 3) == 40);
        CHECK(*NTH(tree2, 4) == 50);
        CHECK(*NTH(tree2, 5) == 60);
        CHECK(*NTH(tree2, 6) == 70);
        CHECK(*NTH(tree2, 7) == 80);
        CHECK(*NTH(tree2, 8) == 90);
    }
}

PRINT("Test container(const container&, const Allocator&)");
{
    {
        sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void> tree1;

        CHECK(tree1.size() == 0);

        ///////////////////////////////////////////////////////////////////////

        TPARAM_ALLOCATOR<xint> alloc;

        sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void> tree2(tree1, alloc);

        CHECK(tree2.size() == 0);
    }

    {
        sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void> tree1;

        tree1.emplace_unique(10);
        tree1.emplace_unique(20);
        tree1.emplace_unique(30);
        tree1.emplace_unique(40);
        tree1.emplace_unique(50);
        tree1.emplace_unique(60);
        tree1.emplace_unique(70);
        tree1.emplace_unique(80);
        tree1.emplace_unique(90);

        CHECK(tree1.size() == 9);
        CHECK(*NTH(tree1, 0) == 10);
        CHECK(*NTH(tree1, 1) == 20);
        CHECK(*NTH(tree1, 2) == 30);
        CHECK(*NTH(tree1, 3) == 40);
        CHECK(*NTH(tree1, 4) == 50);
        CHECK(*NTH(tree1, 5) == 60);
        CHECK(*NTH(tree1, 6) == 70);
        CHECK(*NTH(tree1, 7) == 80);
        CHECK(*NTH(tree1, 8) == 90);

        ///////////////////////////////////////////////////////////////////////

        TPARAM_ALLOCATOR<xint> alloc;

        sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void> tree2(tree1, alloc);

        CHECK(tree2.size() == 9);
        CHECK(*NTH(tree2, 0) == 10);
        CHECK(*NTH(tree2, 1) == 20);
        CHECK(*NTH(tree2, 2) == 30);
        CHECK(*NTH(tree2, 3) == 40);
        CHECK(*NTH(tree2, 4) == 50);
        CHECK(*NTH(tree2, 5) == 60);
        CHECK(*NTH(tree2, 6) == 70);
        CHECK(*NTH(tree2, 7) == 80);
        CHECK(*NTH(tree2, 8) == 90);
    }
}

PRINT("Test container(const container&&)");
{
    {
        sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void> tree1;

        CHECK(tree1.size() == 0);

        ///////////////////////////////////////////////////////////////////////

        sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void> tree2(std::move(tree1));

        CHECK(tree1.size() == 0);
        CHECK(tree2.size() == 0);
    }

    {
        sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void> tree1;

        tree1.emplace_unique(10);
        tree1.emplace_unique(20);
        tree1.emplace_unique(30);
        tree1.emplace_unique(40);
        tree1.emplace_unique(50);
        tree1.emplace_unique(60);
        tree1.emplace_unique(70);
        tree1.emplace_unique(80);
        tree1.emplace_unique(90);

        CHECK(tree1.size() == 9);
        CHECK(*NTH(tree1, 0) == 10);
        CHECK(*NTH(tree1, 1) == 20);
        CHECK(*NTH(tree1, 2) == 30);
        CHECK(*NTH(tree1, 3) == 40);
        CHECK(*NTH(tree1, 4) == 50);
        CHECK(*NTH(tree1, 5) == 60);
        CHECK(*NTH(tree1, 6) == 70);
        CHECK(*NTH(tree1, 7) == 80);
        CHECK(*NTH(tree1, 8) == 90);

        ///////////////////////////////////////////////////////////////////////

        sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void> tree2(std::move(tree1));

        if (tree1.size() == 9)
        {
            // Elements are moved one-by-one
            CHECK(*NTH(tree1, 0) == -10);
            CHECK(*NTH(tree1, 1) == -20);
            CHECK(*NTH(tree1, 2) == -30);
            CHECK(*NTH(tree1, 3) == -40);
            CHECK(*NTH(tree1, 4) == -50);
            CHECK(*NTH(tree1, 5) == -60);
            CHECK(*NTH(tree1, 6) == -70);
            CHECK(*NTH(tree1, 7) == -80);
            CHECK(*NTH(tree1, 8) == -90);
        }
        else
        {
            CHECK(tree1.size() == 0);
        }

        CHECK(tree2.size() == 9);
        CHECK(*NTH(tree2, 0) == 10);
        CHECK(*NTH(tree2, 1) == 20);
        CHECK(*NTH(tree2, 2) == 30);
        CHECK(*NTH(tree2, 3) == 40);
        CHECK(*NTH(tree2, 4) == 50);
        CHECK(*NTH(tree2, 5) == 60);
        CHECK(*NTH(tree2, 6) == 70);
        CHECK(*NTH(tree2, 7) == 80);
        CHECK(*NTH(tree2, 8) == 90);
    }
}

PRINT("Test container(const container&&, const Allocator&)");
{
    {
        sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void> tree1;

        CHECK(tree1.size() == 0);

        ///////////////////////////////////////////////////////////////////////

        TPARAM_ALLOCATOR<xint> alloc;

        sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void> tree2(std::move(tree1), alloc);

        CHECK(tree1.size() == 0);
        CHECK(tree2.size() == 0);
    }

    {
        sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void> tree1;

        tree1.emplace_unique(10);
        tree1.emplace_unique(20);
        tree1.emplace_unique(30);
        tree1.emplace_unique(40);
        tree1.emplace_unique(50);
        tree1.emplace_unique(60);
        tree1.emplace_unique(70);
        tree1.emplace_unique(80);
        tree1.emplace_unique(90);

        CHECK(tree1.size() == 9);
        CHECK(*NTH(tree1, 0) == 10);
        CHECK(*NTH(tree1, 1) == 20);
        CHECK(*NTH(tree1, 2) == 30);
        CHECK(*NTH(tree1, 3) == 40);
        CHECK(*NTH(tree1, 4) == 50);
        CHECK(*NTH(tree1, 5) == 60);
        CHECK(*NTH(tree1, 6) == 70);
        CHECK(*NTH(tree1, 7) == 80);
        CHECK(*NTH(tree1, 8) == 90);

        ///////////////////////////////////////////////////////////////////////

        TPARAM_ALLOCATOR<xint> alloc;

        sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void> tree2(std::move(tree1), alloc);

        if (tree1.size() == 9)
        {
            // Elements are moved one-by-one
            CHECK(*NTH(tree1, 0) == -10);
            CHECK(*NTH(tree1, 1) == -20);
            CHECK(*NTH(tree1, 2) == -30);
            CHECK(*NTH(tree1, 3) == -40);
            CHECK(*NTH(tree1, 4) == -50);
            CHECK(*NTH(tree1, 5) == -60);
            CHECK(*NTH(tree1, 6) == -70);
            CHECK(*NTH(tree1, 7) == -80);
            CHECK(*NTH(tree1, 8) == -90);
        }
        else
        {
            CHECK(tree1.size() == 0);
        }

        CHECK(tree2.size() == 9);
        CHECK(*NTH(tree2, 0) == 10);
        CHECK(*NTH(tree2, 1) == 20);
        CHECK(*NTH(tree2, 2) == 30);
        CHECK(*NTH(tree2, 3) == 40);
        CHECK(*NTH(tree2, 4) == 50);
        CHECK(*NTH(tree2, 5) == 60);
        CHECK(*NTH(tree2, 6) == 70);
        CHECK(*NTH(tree2, 7) == 80);
        CHECK(*NTH(tree2, 8) == 90);
    }
}

///////////////////////////////////////////////////////////////////////////////

PRINT("Test operator=(const container&)");
{
    #define CONDITION tree1.size() == tree2.size()
    {
        sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void> tree1, tree2;

        tree1.emplace_unique(10);
        tree1.emplace_unique(20);
        tree1.emplace_unique(30);

        CHECK(tree1.size() == 3);
        CHECK(*NTH(tree1, 0) == 10);
        CHECK(*NTH(tree1, 1) == 20);
        CHECK(*NTH(tree1, 2) == 30);

        tree2.emplace_unique(40);
        tree2.emplace_unique(50);
        tree2.emplace_unique(60);

        CHECK(tree2.size() == 3);
        CHECK(*NTH(tree2, 0) == 40);
        CHECK(*NTH(tree2, 1) == 50);
        CHECK(*NTH(tree2, 2) == 60);

        ///////////////////////////////////////////////////////////////////////

        CHECK(CONDITION);

        tree2 = tree1;

        CHECK(tree1.size() == 3);
        CHECK(*NTH(tree1, 0) == 10);
        CHECK(*NTH(tree1, 1) == 20);
        CHECK(*NTH(tree1, 2) == 30);

        CHECK(tree2.size() == 3);
        CHECK(*NTH(tree2, 0) == 10);
        CHECK(*NTH(tree2, 1) == 20);
        CHECK(*NTH(tree2, 2) == 30);
    }
    #undef CONDITION

    #define CONDITION tree1.size() < tree2.size()
    {
        sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void> tree1, tree2;

        tree1.emplace_unique(10);
        tree1.emplace_unique(20);
        tree1.emplace_unique(30);

        CHECK(tree1.size() == 3);
        CHECK(*NTH(tree1, 0) == 10);
        CHECK(*NTH(tree1, 1) == 20);
        CHECK(*NTH(tree1, 2) == 30);

        tree2.emplace_unique(40);
        tree2.emplace_unique(50);
        tree2.emplace_unique(60);
        tree2.emplace_unique(70);
        tree2.emplace_unique(80);
        tree2.emplace_unique(90);

        CHECK(tree2.size() == 6);
        CHECK(*NTH(tree2, 0) == 40);
        CHECK(*NTH(tree2, 1) == 50);
        CHECK(*NTH(tree2, 2) == 60);
        CHECK(*NTH(tree2, 3) == 70);
        CHECK(*NTH(tree2, 4) == 80);
        CHECK(*NTH(tree2, 5) == 90);

        ///////////////////////////////////////////////////////////////////////

        CHECK(CONDITION);

        tree2 = tree1;

        CHECK(tree1.size() == 3);
        CHECK(*NTH(tree1, 0) == 10);
        CHECK(*NTH(tree1, 1) == 20);
        CHECK(*NTH(tree1, 2) == 30);

        CHECK(tree2.size() == 3);
        CHECK(*NTH(tree2, 0) == 10);
        CHECK(*NTH(tree2, 1) == 20);
        CHECK(*NTH(tree2, 2) == 30);
    }
    #undef CONDITION

    #define CONDITION tree1.size() > tree2.size()
    {
        sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void> tree1, tree2;

        tree1.emplace_unique(10);
        tree1.emplace_unique(20);
        tree1.emplace_unique(30);
        tree1.emplace_unique(40);
        tree1.emplace_unique(50);
        tree1.emplace_unique(60);

        CHECK(tree1.size() == 6);
        CHECK(*NTH(tree1, 0) == 10);
        CHECK(*NTH(tree1, 1) == 20);
        CHECK(*NTH(tree1, 2) == 30);
        CHECK(*NTH(tree1, 3) == 40);
        CHECK(*NTH(tree1, 4) == 50);
        CHECK(*NTH(tree1, 5) == 60);

        tree2.emplace_unique(70);
        tree2.emplace_unique(80);
        tree2.emplace_unique(90);

        CHECK(tree2.size() == 3);
        CHECK(*NTH(tree2, 0) == 70);
        CHECK(*NTH(tree2, 1) == 80);
        CHECK(*NTH(tree2, 2) == 90);

        ///////////////////////////////////////////////////////////////////////

        CHECK(CONDITION);

        tree2 = tree1;

        CHECK(tree1.size() == 6);
        CHECK(*NTH(tree1, 0) == 10);
        CHECK(*NTH(tree1, 1) == 20);
        CHECK(*NTH(tree1, 2) == 30);
        CHECK(*NTH(tree1, 3) == 40);
        CHECK(*NTH(tree1, 4) == 50);
        CHECK(*NTH(tree1, 5) == 60);

        CHECK(tree2.size() == 6);
        CHECK(*NTH(tree2, 0) == 10);
        CHECK(*NTH(tree2, 1) == 20);
        CHECK(*NTH(tree2, 2) == 30);
        CHECK(*NTH(tree2, 3) == 40);
        CHECK(*NTH(tree2, 4) == 50);
        CHECK(*NTH(tree2, 5) == 60);
    }
    #undef CONDITION
}

PRINT("Test operator=(const container&&)");
{
    #define CONDITION tree1.size() == tree2.size()
    {
        sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void> tree1, tree2;

        tree1.emplace_unique(10);
        tree1.emplace_unique(20);
        tree1.emplace_unique(30);

        CHECK(tree1.size() == 3);
        CHECK(*NTH(tree1, 0) == 10);
        CHECK(*NTH(tree1, 1) == 20);
        CHECK(*NTH(tree1, 2) == 30);

        tree2.emplace_unique(40);
        tree2.emplace_unique(50);
        tree2.emplace_unique(60);

        CHECK(tree2.size() == 3);
        CHECK(*NTH(tree2, 0) == 40);
        CHECK(*NTH(tree2, 1) == 50);
        CHECK(*NTH(tree2, 2) == 60);

        ///////////////////////////////////////////////////////////////////////

        CHECK(CONDITION);

        tree2 = std::move(tree1);

        if (tree1.size() == 3)
        {
            // Elements are moved one-by-one
            CHECK(*NTH(tree1, 0) == -10);
            CHECK(*NTH(tree1, 1) == -20);
            CHECK(*NTH(tree1, 2) == -30);
        }
        else
        {
            CHECK(tree1.size() == 0);
        }

        CHECK(tree2.size() == 3);
        CHECK(*NTH(tree2, 0) == 10);
        CHECK(*NTH(tree2, 1) == 20);
        CHECK(*NTH(tree2, 2) == 30);
    }
    #undef CONDITION

    #define CONDITION tree1.size() < tree2.size()
    {
        sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void> tree1, tree2;

        tree1.emplace_unique(10);
        tree1.emplace_unique(20);
        tree1.emplace_unique(30);

        CHECK(tree1.size() == 3);
        CHECK(*NTH(tree1, 0) == 10);
        CHECK(*NTH(tree1, 1) == 20);
        CHECK(*NTH(tree1, 2) == 30);

        tree2.emplace_unique(40);
        tree2.emplace_unique(50);
        tree2.emplace_unique(60);
        tree2.emplace_unique(70);
        tree2.emplace_unique(80);
        tree2.emplace_unique(90);

        CHECK(tree2.size() == 6);
        CHECK(*NTH(tree2, 0) == 40);
        CHECK(*NTH(tree2, 1) == 50);
        CHECK(*NTH(tree2, 2) == 60);
        CHECK(*NTH(tree2, 3) == 70);
        CHECK(*NTH(tree2, 4) == 80);
        CHECK(*NTH(tree2, 5) == 90);

        ///////////////////////////////////////////////////////////////////////

        CHECK(CONDITION);

        tree2 = std::move(tree1);

        if (tree1.size() == 3)
        {
            // Elements are moved one-by-one
            CHECK(*NTH(tree1, 0) == -10);
            CHECK(*NTH(tree1, 1) == -20);
            CHECK(*NTH(tree1, 2) == -30);
        }
        else
        {
            CHECK(tree1.size() == 0);
        }

        CHECK(tree2.size() == 3);
        CHECK(*NTH(tree2, 0) == 10);
        CHECK(*NTH(tree2, 1) == 20);
        CHECK(*NTH(tree2, 2) == 30);
    }
    #undef CONDITION

    #define CONDITION tree1.size() > tree2.size()
    {
        sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void> tree1, tree2;

        tree1.emplace_unique(10);
        tree1.emplace_unique(20);
        tree1.emplace_unique(30);
        tree1.emplace_unique(40);
        tree1.emplace_unique(50);
        tree1.emplace_unique(60);

        CHECK(tree1.size() == 6);
        CHECK(*NTH(tree1, 0) == 10);
        CHECK(*NTH(tree1, 1) == 20);
        CHECK(*NTH(tree1, 2) == 30);
        CHECK(*NTH(tree1, 3) == 40);
        CHECK(*NTH(tree1, 4) == 50);
        CHECK(*NTH(tree1, 5) == 60);

        tree2.emplace_unique(70);
        tree2.emplace_unique(80);
        tree2.emplace_unique(90);

        CHECK(tree2.size() == 3);
        CHECK(*NTH(tree2, 0) == 70);
        CHECK(*NTH(tree2, 1) == 80);
        CHECK(*NTH(tree2, 2) == 90);

        ///////////////////////////////////////////////////////////////////////

        CHECK(CONDITION);

        tree2 = std::move(tree1);

        if (tree1.size() == 6)
        {
            // Elements are moved one-by-one
            CHECK(*NTH(tree1, 0) == -10);
            CHECK(*NTH(tree1, 1) == -20);
            CHECK(*NTH(tree1, 2) == -30);
            CHECK(*NTH(tree1, 3) == -40);
            CHECK(*NTH(tree1, 4) == -50);
            CHECK(*NTH(tree1, 5) == -60);
        }
        else
        {
            CHECK(tree1.size() == 0);
        }

        CHECK(tree2.size() == 6);
        CHECK(*NTH(tree2, 0) == 10);
        CHECK(*NTH(tree2, 1) == 20);
        CHECK(*NTH(tree2, 2) == 30);
        CHECK(*NTH(tree2, 3) == 40);
        CHECK(*NTH(tree2, 4) == 50);
        CHECK(*NTH(tree2, 5) == 60);
    }
    #undef CONDITION
}

PRINT("Test assign_range_equal(InputIt, InputIt)");
{
    #define CONDITION n == tree.size()
    {
        using tree_type = sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void>;

        tree_type tree;

        tree.emplace_unique(10);
        tree.emplace_unique(20);
        tree.emplace_unique(30);

        CHECK(tree.size() == 3);
        CHECK(*NTH(tree, 0) == 10);
        CHECK(*NTH(tree, 1) == 20);
        CHECK(*NTH(tree, 2) == 30);

        std::vector<int> data({40, 50, 60});

        const tree_type::size_type n = data.size();

        ///////////////////////////////////////////////////////////////////////

        CHECK(CONDITION);

        tree.assign_range_equal(data.begin(), data.end());

        CHECK(tree.size() == 3);
        CHECK(*NTH(tree, 0) == 40);
        CHECK(*NTH(tree, 1) == 50);
        CHECK(*NTH(tree, 2) == 60);
    }
    #undef CONDITION

    #define CONDITION n < tree.size()
    {
        using tree_type = sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void>;

        tree_type tree;

        tree.emplace_unique(10);
        tree.emplace_unique(20);
        tree.emplace_unique(30);
        tree.emplace_unique(40);
        tree.emplace_unique(50);
        tree.emplace_unique(60);

        CHECK(tree.size() == 6);
        CHECK(*NTH(tree, 0) == 10);
        CHECK(*NTH(tree, 1) == 20);
        CHECK(*NTH(tree, 2) == 30);
        CHECK(*NTH(tree, 3) == 40);
        CHECK(*NTH(tree, 4) == 50);
        CHECK(*NTH(tree, 5) == 60);

        std::vector<int> data({70, 80, 90});

        const tree_type::size_type n = data.size();

        ///////////////////////////////////////////////////////////////////////

        CHECK(CONDITION);

        tree.assign_range_equal(data.begin(), data.end());

        CHECK(tree.size() == 3);
        CHECK(*NTH(tree, 0) == 70);
        CHECK(*NTH(tree, 1) == 80);
        CHECK(*NTH(tree, 2) == 90);
    }
    #undef CONDITION

    #define CONDITION n > tree.size()
    {
        using tree_type = sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void>;

        tree_type tree;

        tree.emplace_unique(10);
        tree.emplace_unique(20);
        tree.emplace_unique(30);

        CHECK(tree.size() == 3);
        CHECK(*NTH(tree, 0) == 10);
        CHECK(*NTH(tree, 1) == 20);
        CHECK(*NTH(tree, 2) == 30);

        std::vector<int> data({40, 50, 60, 70, 80, 90});

        const tree_type::size_type n = data.size();

        ///////////////////////////////////////////////////////////////////////

        CHECK(CONDITION);

        tree.assign_range_equal(data.begin(), data.end());

        CHECK(tree.size() == 6);
        CHECK(*NTH(tree, 0) == 40);
        CHECK(*NTH(tree, 1) == 50);
        CHECK(*NTH(tree, 2) == 60);
        CHECK(*NTH(tree, 3) == 70);
        CHECK(*NTH(tree, 4) == 80);
        CHECK(*NTH(tree, 5) == 90);
    }
    #undef CONDITION

    // Input iterator range with duplicates
    {
        using tree_type = sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void>;

        tree_type tree;

        tree.emplace_unique(10);
        tree.emplace_unique(20);
        tree.emplace_unique(30);

        CHECK(tree.size() == 3);
        CHECK(*NTH(tree, 0) == 10);
        CHECK(*NTH(tree, 1) == 20);
        CHECK(*NTH(tree, 2) == 30);

        std::vector<int> data({40, 50, 60, 50, 50});

        ///////////////////////////////////////////////////////////////////////

        tree.assign_range_equal(data.begin(), data.end());

        CHECK(tree.size() == 5);
        CHECK(*NTH(tree, 0) == 40);
        CHECK(*NTH(tree, 1) == 50);
        CHECK(*NTH(tree, 2) == 50);
        CHECK(*NTH(tree, 3) == 50);
        CHECK(*NTH(tree, 4) == 60);
    }

    // Move iterator range with duplicates
    {
        sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void> tree;

        tree.emplace_unique(10);
        tree.emplace_unique(20);
        tree.emplace_unique(30);

        CHECK(tree.size() == 3);
        CHECK(*NTH(tree, 0) == 10);
        CHECK(*NTH(tree, 1) == 20);
        CHECK(*NTH(tree, 2) == 30);

        std::vector<xint> data({40, 50, 60, 50, 50});

        ///////////////////////////////////////////////////////////////////////

        tree.assign_range_equal(std::make_move_iterator(data.begin()), std::make_move_iterator(data.end()));

        CHECK(tree.size() == 5);
        CHECK(*NTH(tree, 0) == 40);
        CHECK(*NTH(tree, 1) == 50);
        CHECK(*NTH(tree, 2) == 50);
        CHECK(*NTH(tree, 3) == 50);
        CHECK(*NTH(tree, 4) == 60);

        CHECK(data.size() == 5);
        CHECK(data[0] == -40);
        CHECK(data[1] == -50);
        CHECK(data[2] == -60);
        CHECK(data[3] == -50);
        CHECK(data[4] == -50);
    }
}

PRINT("Test assign_range_unique(InputIt, InputIt)");
{
    #define CONDITION n == tree.size()
    {
        using tree_type = sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void>;

        tree_type tree;

        tree.emplace_unique(10);
        tree.emplace_unique(20);
        tree.emplace_unique(30);

        CHECK(tree.size() == 3);
        CHECK(*NTH(tree, 0) == 10);
        CHECK(*NTH(tree, 1) == 20);
        CHECK(*NTH(tree, 2) == 30);

        std::vector<int> data({40, 50, 60});

        const tree_type::size_type n = data.size();

        ///////////////////////////////////////////////////////////////////////

        CHECK(CONDITION);

        tree.assign_range_unique(data.begin(), data.end());

        CHECK(tree.size() == 3);
        CHECK(*NTH(tree, 0) == 40);
        CHECK(*NTH(tree, 1) == 50);
        CHECK(*NTH(tree, 2) == 60);
    }
    #undef CONDITION

    #define CONDITION n < tree.size()
    {
        using tree_type = sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void>;

        tree_type tree;

        tree.emplace_unique(10);
        tree.emplace_unique(20);
        tree.emplace_unique(30);
        tree.emplace_unique(40);
        tree.emplace_unique(50);
        tree.emplace_unique(60);

        CHECK(tree.size() == 6);
        CHECK(*NTH(tree, 0) == 10);
        CHECK(*NTH(tree, 1) == 20);
        CHECK(*NTH(tree, 2) == 30);
        CHECK(*NTH(tree, 3) == 40);
        CHECK(*NTH(tree, 4) == 50);
        CHECK(*NTH(tree, 5) == 60);

        std::vector<int> data({70, 80, 90});

        const tree_type::size_type n = data.size();

        ///////////////////////////////////////////////////////////////////////

        CHECK(CONDITION);

        tree.assign_range_unique(data.begin(), data.end());

        CHECK(tree.size() == 3);
        CHECK(*NTH(tree, 0) == 70);
        CHECK(*NTH(tree, 1) == 80);
        CHECK(*NTH(tree, 2) == 90);
    }
    #undef CONDITION

    #define CONDITION n > tree.size()
    {
        using tree_type = sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void>;

        tree_type tree;

        tree.emplace_unique(10);
        tree.emplace_unique(20);
        tree.emplace_unique(30);

        CHECK(tree.size() == 3);
        CHECK(*NTH(tree, 0) == 10);
        CHECK(*NTH(tree, 1) == 20);
        CHECK(*NTH(tree, 2) == 30);

        std::vector<int> data({40, 50, 60, 70, 80, 90});

        const tree_type::size_type n = data.size();

        ///////////////////////////////////////////////////////////////////////

        CHECK(CONDITION);

        tree.assign_range_unique(data.begin(), data.end());

        CHECK(tree.size() == 6);
        CHECK(*NTH(tree, 0) == 40);
        CHECK(*NTH(tree, 1) == 50);
        CHECK(*NTH(tree, 2) == 60);
        CHECK(*NTH(tree, 3) == 70);
        CHECK(*NTH(tree, 4) == 80);
        CHECK(*NTH(tree, 5) == 90);
    }
    #undef CONDITION

    // Input iterator range with duplicates
    {
        using tree_type = sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void>;

        tree_type tree;

        tree.emplace_unique(10);
        tree.emplace_unique(20);
        tree.emplace_unique(30);

        CHECK(tree.size() == 3);
        CHECK(*NTH(tree, 0) == 10);
        CHECK(*NTH(tree, 1) == 20);
        CHECK(*NTH(tree, 2) == 30);

        std::vector<int> data({40, 50, 60, 50, 50});

        ///////////////////////////////////////////////////////////////////////

        tree.assign_range_unique(data.begin(), data.end());

        CHECK(tree.size() == 3);
        CHECK(*NTH(tree, 0) == 40);
        CHECK(*NTH(tree, 1) == 50);
        CHECK(*NTH(tree, 2) == 60);
    }

    // Move iterator range with duplicates
    {
        sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void> tree;

        tree.emplace_unique(10);
        tree.emplace_unique(20);
        tree.emplace_unique(30);

        CHECK(tree.size() == 3);
        CHECK(*NTH(tree, 0) == 10);
        CHECK(*NTH(tree, 1) == 20);
        CHECK(*NTH(tree, 2) == 30);

        std::vector<xint> data({40, 50, 60, 50, 50});

        ///////////////////////////////////////////////////////////////////////

        tree.assign_range_unique(std::make_move_iterator(data.begin()), std::make_move_iterator(data.end()));

        CHECK(tree.size() == 3);
        CHECK(*NTH(tree, 0) == 40);
        CHECK(*NTH(tree, 1) == 50);
        CHECK(*NTH(tree, 2) == 60);

        CHECK(data.size() == 5);
        CHECK(data[0] == -40);
        CHECK(data[1] == -50);
        CHECK(data[2] == -60);
        CHECK(data[3] == +50);
        CHECK(data[4] == +50);
    }
}

///////////////////////////////////////////////////////////////////////////////

PRINT("Test NON-MEMBER comparison operators");
{
    sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void> tree1, tree2;

    tree1.emplace_unique(10);
    tree1.emplace_unique(20);
    tree1.emplace_unique(30);

    tree2.emplace_unique(10);
    tree2.emplace_unique(20);
    tree2.emplace_unique(30);
    tree2.emplace_unique(40);
    tree2.emplace_unique(50);

    CHECK((tree1 == tree1) == true);
    CHECK((tree1 == tree2) == false);
    CHECK((tree2 == tree1) == false);
    CHECK((tree2 == tree2) == true);

    CHECK((tree1 != tree1) == false);
    CHECK((tree1 != tree2) == true);
    CHECK((tree2 != tree1) == true);
    CHECK((tree2 != tree2) == false);

    CHECK((tree1 < tree1) == false);
    CHECK((tree1 < tree2) == true);
    CHECK((tree2 < tree1) == false);
    CHECK((tree2 < tree2) == false);

    CHECK((tree1 > tree1) == false);
    CHECK((tree1 > tree2) == false);
    CHECK((tree2 > tree1) == true);
    CHECK((tree2 > tree2) == false);

    CHECK((tree1 <= tree1) == true);
    CHECK((tree1 <= tree2) == true);
    CHECK((tree2 <= tree1) == false);
    CHECK((tree2 <= tree2) == true);

    CHECK((tree1 >= tree1) == true);
    CHECK((tree1 >= tree2) == false);
    CHECK((tree2 >= tree1) == true);
    CHECK((tree2 >= tree2) == true);
}

PRINT("Test NON-MEMBER swap(container&)");
{
    sfl::dtl::rb_tree<xint, xint, sfl::dtl::identity, std::less<xint>, TPARAM_ALLOCATOR<xint>, void> tree1, tree2;

    tree1.emplace_unique(10);
    tree1.emplace_unique(20);
    tree1.emplace_unique(30);
    tree1.emplace_unique(40);
    tree1.emplace_unique(50);
    tree1.emplace_unique(60);

    tree2.emplace_unique(70);
    tree2.emplace_unique(80);
    tree2.emplace_unique(90);

    CHECK(tree1.size() == 6);
    CHECK(*NTH(tree1, 0) == 10);
    CHECK(*NTH(tree1, 1) == 20);
    CHECK(*NTH(tree1, 2) == 30);
    CHECK(*NTH(tree1, 3) == 40);
    CHECK(*NTH(tree1, 4) == 50);
    CHECK(*NTH(tree1, 5) == 60);

    CHECK(tree2.size() == 3);
    CHECK(*NTH(tree2, 0) == 70);
    CHECK(*NTH(tree2, 1) == 80);
    CHECK(*NTH(tree2, 2) == 90);

    ///////////////////////////////////////////////////////////////////////////

    swap(tree1, tree2);

    CHECK(tree1.size() == 3);
    CHECK(*NTH(tree1, 0) == 70);
    CHECK(*NTH(tree1, 1) == 80);
    CHECK(*NTH(tree1, 2) == 90);

    CHECK(tree2.size() == 6);
    CHECK(*NTH(tree2, 0) == 10);
    CHECK(*NTH(tree2, 1) == 20);
    CHECK(*NTH(tree2, 2) == 30);
    CHECK(*NTH(tree2, 3) == 40);
    CHECK(*NTH(tree2, 4) == 50);
    CHECK(*NTH(tree2, 5) == 60);
}
