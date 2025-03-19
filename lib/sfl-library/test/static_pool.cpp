#undef NDEBUG // This is very important. Must be in the first line.

#include "sfl/detail/static_pool.hpp"

#include "sfl/detail/uninitialized_memory_algorithms.hpp"

#include "check.hpp"
#include "print.hpp"

#include "xint.hpp"

int main()
{
    using sfl::test::xint;

    PRINT("Test static_pool<xint, 4>");
    {
        sfl::dtl::static_pool<xint, 4> pool;

        PRINT("Make 10");
        auto* p1 = pool.allocate();
        sfl::dtl::construct_at(p1, 10);

        PRINT("Make 20");
        auto* p2 = pool.allocate();
        sfl::dtl::construct_at(p2, 20);

        PRINT("Make 30");
        auto* p3 = pool.allocate();
        sfl::dtl::construct_at(p3, 30);

        CHECK(p1 < p2);
        CHECK(p2 < p3);

        ///////////////////////////////////////////////////////////////////////

        PRINT("Dump 20");
        sfl::dtl::destroy_at(p2);
        pool.deallocate(p2);

        PRINT("Make 21");
        p2 = pool.allocate();
        sfl::dtl::construct_at(p2, 21);

        // New element should be at the same address as old element.
        CHECK(p1 < p2);
        CHECK(p2 < p3);

        ///////////////////////////////////////////////////////////////////////

        PRINT("Make 40");
        auto* p4 = pool.allocate();
        sfl::dtl::construct_at(p4, 40);

        // New element is last.
        CHECK(p1 < p2);
        CHECK(p2 < p3);
        CHECK(p3 < p4);

        ///////////////////////////////////////////////////////////////////////

        // The following line aborts because pool is full.
        // auto* p5 = pool.allocate(); (void)p5;

        ///////////////////////////////////////////////////////////////////////

        PRINT("Dump 10");
        sfl::dtl::destroy_at(p1);
        pool.deallocate(p1);

        PRINT("Make 11");
        p1 = pool.allocate();
        sfl::dtl::construct_at(p1, 11);

        // New element should be at the same address as old element.
        CHECK(p1 < p2);
        CHECK(p2 < p3);
        CHECK(p3 < p4);

        ///////////////////////////////////////////////////////////////////////

        // Deallocate elements if FIFO order!!!

        PRINT("Dump 11");
        sfl::dtl::destroy_at(p1);
        pool.deallocate(p1);

        PRINT("Dump 21");
        sfl::dtl::destroy_at(p2);
        pool.deallocate(p2);

        PRINT("Dump 30");
        sfl::dtl::destroy_at(p3);
        pool.deallocate(p3);

        PRINT("Dump 40");
        sfl::dtl::destroy_at(p4);
        pool.deallocate(p4);

        ///////////////////////////////////////////////////////////////////////

        PRINT("Make 10");
        p1 = pool.allocate();
        sfl::dtl::construct_at(p1, 10);

        PRINT("Make 20");
        p2 = pool.allocate();
        sfl::dtl::construct_at(p2, 20);

        PRINT("Make 30");
        p3 = pool.allocate();
        sfl::dtl::construct_at(p3, 30);

        PRINT("Make 40");
        p4 = pool.allocate();
        sfl::dtl::construct_at(p4, 40);

        // Pointer are now in reversed order because old elements were not deallocated if LIFO order.
        CHECK(p4 < p3);
        CHECK(p3 < p2);
        CHECK(p2 < p1);

        ///////////////////////////////////////////////////////////////////////

        // Deallocate elements. Order is irrelevant since this is end of test.

        PRINT("Dump 10");
        sfl::dtl::destroy_at(p1);
        pool.deallocate(p1);

        PRINT("Dump 20");
        sfl::dtl::destroy_at(p2);
        pool.deallocate(p2);

        PRINT("Dump 30");
        sfl::dtl::destroy_at(p3);
        pool.deallocate(p3);

        PRINT("Dump 40");
        sfl::dtl::destroy_at(p4);
        pool.deallocate(p4);
    }
}
