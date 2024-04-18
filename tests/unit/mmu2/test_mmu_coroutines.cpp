#include <catch2/catch.hpp>
#include <mmu2/mmu2_bootloader_coroutine.hpp>
#include <memory_resource>

using namespace MMU2::bootloader;

TEST_CASE("MMU2::coroutine") {
    class Manager {

    public:
        std::coroutine_handle<> resume_point = {};
        std::pmr::memory_resource &co_memory_resource = *std::pmr::new_delete_resource();
    };

    struct Yield {
        bool await_ready() const noexcept {
            return false;
        }
        void await_resume() const noexcept {
            REQUIRE(mgr.resume_point);
            mgr.resume_point = {};
        }

        void await_suspend(std::coroutine_handle<> h) {
            REQUIRE(!mgr.resume_point);
            mgr.resume_point = h;
        }

        Manager &mgr;
    };

    class M : public Manager {

    public:
        enum class Phase {
            init,

            task1_stage1,
            task1_stage2,
            task1_stage3,
            task1_stage4,
            task1_stage5,
            task1_stage6,

            task2_stage1,
            task2_stage2,
            task2_stage3,
            task2_stage4,

            task3_stage1,
        };
        Phase phase = Phase::init;

        Yield yield() {
            return Yield { *this };
        }

        Task<void> task3() {
            REQUIRE(phase == Phase::task1_stage5);
            phase = Phase::task3_stage1;
            co_await std::suspend_never();
        }
        Task<void> task2() {
            REQUIRE(phase == Phase::task1_stage3);
            phase = Phase::task2_stage2;

            co_await yield();
            // a bit of task1 was done in the meantime
            REQUIRE(phase == Phase::task1_stage4);
            phase = Phase::task2_stage3;

            co_await yield();
            REQUIRE(phase == Phase::task2_stage3);
            phase = Phase::task2_stage4;
        }
        Task<void> task1() {
            REQUIRE(phase == Phase::init);
            phase = Phase::task1_stage1;

            co_await yield();
            REQUIRE(phase == Phase::task1_stage1);
            phase = Phase::task1_stage2;

            co_await yield();
            REQUIRE(phase == Phase::task1_stage2);
            phase = Phase::task1_stage3;

            // Run task2 up to its first co_await
            auto task = task2();
            REQUIRE(phase == Phase::task2_stage2);
            phase = Phase::task1_stage4;

            // Wait till the rest of task2 finishes
            co_await task;
            REQUIRE(phase == Phase::task2_stage4);
            phase = Phase::task1_stage5;

            // Non-suspending function
            co_await task3();
            REQUIRE(phase == Phase::task3_stage1);
            phase = Phase::task1_stage6;
        }

        void test() {
            REQUIRE(phase == Phase::init);

            // The coroutine should run the first part of the body and stop on the first co_await in task1
            auto fh = task1();
            REQUIRE(phase == Phase::task1_stage1);

            // Here, we should continue from the first co_await and stop on the second co_await in task2
            resume_point();
            REQUIRE(phase == Phase::task1_stage2);

            // Continue from the second co_await in task1, start processing task2, stop on its co_await, which till then do a bit in task1 till it co_awaits the task2
            resume_point();
            REQUIRE(phase == Phase::task1_stage4);

            // Continue from the co_await in task2, do a little bit in the task2
            resume_point();
            REQUIRE(phase == Phase::task2_stage3);

            // Finish the task2, continue in task1 (which does task3 without suspending), finish task1
            resume_point();
            REQUIRE(phase == Phase::task1_stage6);
            REQUIRE(!fh.is_active());
        }
    };

    M m;
    m.test();
    REQUIRE(m.phase == M::Phase::task1_stage6);
}
