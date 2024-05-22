#include <stdx/intrusive_list.hpp>

#include <catch2/catch_test_macros.hpp>

namespace {
struct int_node {
    int value{};
    int_node *prev{};
    int_node *next{};
};
#if __cpp_concepts >= 201907L
static_assert(stdx::double_linkable<int_node>);
#endif

struct double_link_node {
    double_link_node *prev{};
    double_link_node *next{};
};

struct bad_double_link_node {
    int *prev{};
    int *next{};
};
} // namespace

#if __cpp_concepts >= 201907L
TEST_CASE("double_linkable", "[intrusive_list]") {
    static_assert(not stdx::double_linkable<int>);
    static_assert(not stdx::double_linkable<bad_double_link_node>);
    static_assert(stdx::double_linkable<double_link_node>);
}
#endif

TEST_CASE("push_back, pop_front", "[intrusive_list]") {
    stdx::intrusive_list<int_node> list{};
    int_node n{5};

    list.push_back(&n);
    auto poppedNode = list.pop_front();

    CHECK(poppedNode->value == 5);
}

TEST_CASE("push_front, pop_back", "[intrusive_list]") {
    stdx::intrusive_list<int_node> list{};
    int_node n{5};

    list.push_front(&n);
    auto poppedNode = list.pop_back();

    CHECK(poppedNode->value == 5);
}

TEST_CASE("empty", "[intrusive_list]") {
    stdx::intrusive_list<int_node> list{};
    int_node n{5};

    CHECK(list.empty());

    list.push_back(&n);
    REQUIRE_FALSE(list.empty());

    list.pop_front();
    CHECK(list.empty());
}

TEST_CASE("push_back 2,  pop_front 2", "[intrusive_list]") {
    stdx::intrusive_list<int_node> list{};
    int_node n1{1};
    int_node n2{2};

    list.push_back(&n1);
    list.push_back(&n2);

    CHECK(list.pop_front()->value == 1);
    CHECK(list.pop_front()->value == 2);
    CHECK(list.empty());
}

TEST_CASE("push_back 2, pop_front 2 (sequentially)", "[intrusive_list]") {
    stdx::intrusive_list<int_node> list{};
    int_node n1{1};
    int_node n2{2};

    list.push_back(&n1);
    CHECK(list.pop_front()->value == 1);
    CHECK(list.empty());

    list.push_back(&n2);
    CHECK(list.pop_front()->value == 2);
    CHECK(list.empty());
}

TEST_CASE("push_back 2, pop_front 2 (multiple lists)", "[intrusive_list]") {
    stdx::intrusive_list<int_node> listA{};
    stdx::intrusive_list<int_node> listB{};
    int_node n1{1};
    int_node n2{2};

    listA.push_back(&n1);
    listA.push_back(&n2);
    listA.pop_front();
    listA.pop_front();
    CHECK(listA.empty());

    listB.push_back(&n1);
    CHECK(listB.pop_front()->value == 1);
    CHECK(listB.empty());
}

TEST_CASE("remove middle node", "[intrusive_list]") {
    stdx::intrusive_list<int_node> list{};
    int_node n1{1};
    int_node n2{2};
    int_node n3{3};

    list.push_back(&n1);
    list.push_back(&n2);
    list.push_back(&n3);

    list.remove(&n2);

    CHECK(list.pop_front()->value == 1);
    CHECK(list.pop_front()->value == 3);
    CHECK(list.empty());
}

TEST_CASE("remove only node", "[intrusive_list]") {
    stdx::intrusive_list<int_node> list{};
    int_node n1{1};

    list.push_back(&n1);

    list.remove(&n1);

    CHECK(list.empty());
}

TEST_CASE("remove first node", "[intrusive_list]") {
    stdx::intrusive_list<int_node> list{};
    int_node n1{1};
    int_node n2{2};

    list.push_back(&n1);
    list.push_back(&n2);

    list.remove(&n1);

    CHECK(list.pop_front()->value == 2);
    CHECK(list.empty());
}

TEST_CASE("remove last node", "[intrusive_list]") {
    stdx::intrusive_list<int_node> list{};
    int_node n1{1};
    int_node n2{2};

    list.push_back(&n1);
    list.push_back(&n2);

    list.remove(&n2);

    CHECK(list.pop_front()->value == 1);
    CHECK(list.empty());
}

TEST_CASE("begin", "[intrusive_list]") {
    stdx::intrusive_list<int_node> list{};
    int_node n1{1};
    int_node n2{2};
    int_node n3{3};

    list.push_back(&n1);
    list.push_back(&n2);
    list.push_back(&n3);

    CHECK(std::begin(list)->value == 1);
    CHECK(std::cbegin(list)->value == 1);
}

TEST_CASE("iterator preincrement", "[intrusive_list]") {
    stdx::intrusive_list<int_node> list{};
    int_node n1{1};
    int_node n2{2};
    int_node n3{3};

    list.push_back(&n1);
    list.push_back(&n2);
    list.push_back(&n3);

    auto i = std::begin(list);
    CHECK(i->value == 1);

    CHECK((++i)->value == 2);
    CHECK(i->value == 2);

    CHECK((++i)->value == 3);
    CHECK(i->value == 3);
}

TEST_CASE("iterator postincrement", "[intrusive_list]") {
    stdx::intrusive_list<int_node> list{};
    int_node n1{1};
    int_node n2{2};
    int_node n3{3};

    list.push_back(&n1);
    list.push_back(&n2);
    list.push_back(&n3);

    auto i = std::begin(list);
    CHECK(i->value == 1);

    CHECK((i++)->value == 1);
    CHECK(i->value == 2);

    CHECK((i++)->value == 2);
    CHECK(i->value == 3);
}

TEST_CASE("iterator equality", "[intrusive_list]") {
    stdx::intrusive_list<int_node> list{};
    int_node n1{1};

    list.push_back(&n1);

    CHECK(std::begin(list) == std::begin(list));
}

TEST_CASE("iterator inequality", "[intrusive_list]") {
    stdx::intrusive_list<int_node> list{};
    int_node n1{1};
    int_node n2{2};
    int_node n3{3};

    list.push_back(&n1);
    list.push_back(&n2);
    list.push_back(&n3);

    auto i = std::begin(list);
    CHECK(i == std::begin(list));
    CHECK(i != std::end(list));

    i++;
    CHECK(i != std::begin(list));
    CHECK(i != std::end(list));

    i++;
    CHECK(i != std::begin(list));
    CHECK(i != std::end(list));

    i++;
    CHECK(i != std::begin(list));
    CHECK(i == std::end(list));
}

TEST_CASE("clear", "[intrusive_list]") {
    stdx::intrusive_list<int_node> list{};
    int_node n1{1};
    int_node n2{2};

    list.push_back(&n1);
    list.push_back(&n2);
    list.clear();

    CHECK(list.empty());
}

TEST_CASE("remove and re-add same node", "[intrusive_list]") {
    stdx::intrusive_list<int_node> list{};
    int_node n1{1};
    int_node n2{2};

    list.push_back(&n2);
    list.push_back(&n1);

    list.remove(&n2);
    list.push_back(&n2);
    CHECK(list.pop_front()->value == 1);
    CHECK(list.pop_front()->value == 2);
    CHECK(list.empty());
}

TEST_CASE("remove from empty list", "[intrusive_list]") {
    stdx::intrusive_list<int_node> list{};
    int_node n1{1};
    list.remove(&n1);
    CHECK(list.empty());
}
