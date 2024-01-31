/**
 * @file fsm_types_test.cpp
 * @author Radek Vana
 * @brief Unit tests for fsm types, especially fsm::SmartQueue
 * @date 2021-02-22
 */

// #define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include "catch2/catch.hpp"

#include "log.h"
#include "fsm_types.hpp"

using namespace fsm;
class MockQueue : public fsm::SmartQueue {
public:
    size_t GetCount() const { return queue0.count() + queue1.count() + queue2.count(); }

    void TestEmpty() {
        REQUIRE(GetCount() == 0);

        REQUIRE_FALSE(queue0.has_pending_create_command());
        REQUIRE_FALSE(queue1.has_pending_create_command());
    }
};

/*****************************************************************************/
// tests
TEST_CASE("fsm::Change", "[fsm]") {
    ClientFSM generator_fsm_type = GENERATE(ClientFSM(0), ClientFSM::_none, ClientFSM::Load_unload); // _none != 0 , it is last
    QueueIndex generator_queue_index = GENERATE(QueueIndex::q0, QueueIndex::q1, QueueIndex::q2);
    BaseData generator_data = GENERATE(BaseData(0x00, PhaseData({ 0x10, 0x20, 0x30, 0x40 })), BaseData(0xFF, PhaseData({ 0x01, 0x02, 0x03, 0x04 })), BaseData(0x02, PhaseData({ 0x00, 0x00, 0x00, 0x00 })), BaseData(0x05, PhaseData({ 0xFF, 0xFF, 0xFF, 0xFF })));

    // test getters and ctor
    Change change(generator_queue_index, generator_fsm_type, generator_data);
    REQUIRE(change.get_fsm_type() == generator_fsm_type);
    REQUIRE(change.get_queue_index() == generator_queue_index);
    REQUIRE(change.get_data() == generator_data);

    // test serialize, deserialize ctor and operator ==
    std::pair<uint32_t, uint16_t> serialized = change.serialize();
    Change change2(serialized);
    REQUIRE(change == change2);
    REQUIRE(change2.get_fsm_type() == generator_fsm_type);
    REQUIRE(change2.get_queue_index() == generator_queue_index);
    REQUIRE(change2.get_data() == generator_data);
}

TEST_CASE("Equality of BaseData", "[fsm]") {
    const BaseData base_data12(0x01, { { 0x02, 0x00, 0x00, 0x00 } });
    const BaseData base_data12_b(0x01, { { 0x02, 0x00, 0x00, 0x00 } });
    const BaseData base_data11(0x01, { { 0x01, 0x00, 0x00, 0x00 } });
    const BaseData base_data21(0x02, { { 0x01, 0x00, 0x00, 0x00 } });
    const BaseData base_data22(0x02, { { 0x02, 0x00, 0x00, 0x00 } });

    REQUIRE(base_data12 == base_data12_b);
    REQUIRE_FALSE(base_data12 == base_data11);
    REQUIRE_FALSE(base_data12 == base_data21);
    REQUIRE_FALSE(base_data12 == base_data22);

    REQUIRE_FALSE(base_data12 != base_data12_b);
    REQUIRE(base_data12 != base_data11);
    REQUIRE(base_data12 != base_data21);
    REQUIRE(base_data12 != base_data22);
}

TEST_CASE("fsm::SmartQueue", "[fsm]") {
    MockQueue q;

    SECTION("Empty queue") {
        q.TestEmpty();
    }

    SECTION("Pass without queuing") {
        q.TestEmpty();

        const BaseData data(0x01, { { 0x02, 0x00, 0x00, 0x00 } });
        REQUIRE(q.PushCreate(ClientFSM::Preheat, data) == SmartQueue::Selector::q0);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);
        std::optional<DequeStates> states = q.dequeue();
        REQUIRE(states.has_value());
        REQUIRE(states->current.get_fsm_type() == ClientFSM::Preheat);
        REQUIRE(states->current.get_queue_index() == QueueIndex::q0);
        REQUIRE(states->current.get_data() == data);
        REQUIRE(states->last_sent.get_fsm_type() == ClientFSM::_none);
        REQUIRE(states->last_sent.get_queue_index() == QueueIndex::q0);
        q.TestEmpty();
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        const BaseData base_data(0xCD, { { 0xAB, 0xBA, 0x1A, 0xF0 } });
        q.PushChange(ClientFSM::Preheat, base_data);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);
        states = q.dequeue();
        REQUIRE(states.has_value());
        REQUIRE(states->current.get_fsm_type() == ClientFSM::Preheat);
        REQUIRE(states->current.get_queue_index() == QueueIndex::q0);
        REQUIRE(states->current.get_data() == base_data);
        REQUIRE(states->last_sent.get_fsm_type() == ClientFSM::Preheat);
        REQUIRE(states->last_sent.get_queue_index() == QueueIndex::q0);
        q.TestEmpty();
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        q.PushDestroy(ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);
        states = q.dequeue();
        REQUIRE(states.has_value());
        REQUIRE(states->current.get_fsm_type() == ClientFSM::_none);
        REQUIRE(states->current.get_queue_index() == QueueIndex::q0);
        REQUIRE(states->last_sent.get_fsm_type() == ClientFSM::Preheat);
        REQUIRE(states->last_sent.get_queue_index() == QueueIndex::q0);
        q.TestEmpty();
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);
    }

    SECTION("Destroy without create - BSOD") {
        q.TestEmpty();

        REQUIRE_THROWS(q.PushDestroy(ClientFSM::Preheat));
    }

    SECTION("Destroy wrong type - one queue full - BSOD") {
        q.TestEmpty();

        // push create to Q0
        const BaseData base_data(0x25, { { 0x03, 0x00, 0x00, 0x00 } });
        REQUIRE(q.PushCreate(ClientFSM::Preheat, base_data) == SmartQueue::Selector::q0);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // push wrong type
        REQUIRE_THROWS(q.PushDestroy(ClientFSM::CrashRecovery));
    }

    SECTION("Destroy wrong type - all queues full - BSOD") {
        q.TestEmpty();

        // push create to Q0
        const BaseData base_data(0x25, { { 0x03, 0x00, 0x00, 0x00 } });
        REQUIRE(q.PushCreate(ClientFSM::Preheat, base_data) == SmartQueue::Selector::q0);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // push create to Q1
        const BaseData base_data2(0xAA, { { 0x02, 0x00, 0x00, 0x00 } });
        REQUIRE(q.PushCreate(ClientFSM::Load_unload, base_data2) == SmartQueue::Selector::q1);
        REQUIRE(q.GetCount() == 2);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::Load_unload);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        const BaseData base_data3(0xAA, { { 0x00, 0x00, 0x00, 0x02 } });
        REQUIRE(q.PushCreate(ClientFSM::Warning, base_data3) == SmartQueue::Selector::q2);
        REQUIRE(q.GetCount() == 3);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::Load_unload);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::Warning);

        // push wrong type
        REQUIRE_THROWS(q.PushDestroy(ClientFSM::Selftest));
    }

    SECTION("Multiple destroy - BSOD") {
        q.TestEmpty();

        // insert create, so queue is in correct state
        const BaseData data(0x01, { { 0x02, 0x00, 0xFF, 0x00 } });
        REQUIRE(q.PushCreate(ClientFSM::Load_unload, data) == SmartQueue::Selector::q0);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Load_unload);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);
        std::optional<DequeStates> states = q.dequeue();
        REQUIRE(states.has_value());
        REQUIRE(states->current.get_fsm_type() == ClientFSM::Load_unload);
        REQUIRE(states->current.get_queue_index() == QueueIndex::q0);
        REQUIRE(states->current.get_data() == data);
        REQUIRE(states->last_sent.get_fsm_type() == ClientFSM::_none);
        REQUIRE(states->last_sent.get_queue_index() == QueueIndex::q0);
        q.TestEmpty();
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Load_unload);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // correct destroy
        q.PushDestroy(ClientFSM::Load_unload);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // 2nd destroy of same type - cannot be inserted
        REQUIRE_THROWS(q.PushDestroy(ClientFSM::Load_unload));
    }

    SECTION("Destroy ClientFSM::_none - both queues empty - BSOD") {
        q.TestEmpty();

        // cannot destroy ClientFSM::_none
        REQUIRE_THROWS(q.PushDestroy(ClientFSM::_none));
    }

    SECTION("Destroy ClientFSM::_none - one queue full - BSOD") {
        q.TestEmpty();

        // push create to Q0
        const BaseData base_data(0x25, { { 0x03, 0x00, 0x00, 0x00 } });
        REQUIRE(q.PushCreate(ClientFSM::Preheat, base_data) == SmartQueue::Selector::q0);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // cannot destroy ClientFSM::_none
        REQUIRE_THROWS(q.PushDestroy(ClientFSM::_none));
    }

    SECTION("Destroy ClientFSM::_none - all queues full - BSOD") {
        q.TestEmpty();

        // push create to Q0
        const BaseData base_data(0x25, { { 0x03, 0x00, 0x00, 0x00 } });
        REQUIRE(q.PushCreate(ClientFSM::Preheat, base_data) == SmartQueue::Selector::q0);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // push create to Q1
        const BaseData base_data2(0xAA, { { 0x02, 0x00, 0x00, 0x00 } });
        REQUIRE(q.PushCreate(ClientFSM::Load_unload, base_data2) == SmartQueue::Selector::q1);
        REQUIRE(q.GetCount() == 2);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::Load_unload);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        const BaseData base_data3(0xAA, { { 0x00, 0x00, 0x00, 0x02 } });
        REQUIRE(q.PushCreate(ClientFSM::Warning, base_data3) == SmartQueue::Selector::q2);
        REQUIRE(q.GetCount() == 3);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::Load_unload);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::Warning);

        // cannot destroy ClientFSM::_none
        REQUIRE_THROWS(q.PushDestroy(ClientFSM::_none));
    }

    SECTION("Multiple create") {
        q.TestEmpty();

        const BaseData data(0xFF, { { 0x12, 0x25, 0x52, 0x11 } });
        REQUIRE(q.PushCreate(ClientFSM::Preheat, data) == SmartQueue::Selector::q0);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // insertion must not fail - we have 3 level queue
        const BaseData data2(0xFF, { { 0x12, 0x25, 0x52, 0x00 } });
        q.PushCreate(ClientFSM::Load_unload, data2);
        REQUIRE(q.GetCount() == 2);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::Load_unload);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        const BaseData data3(0xAA, { { 0x00, 0x00, 0x00, 0x02 } });
        REQUIRE(q.PushCreate(ClientFSM::Warning, data3) == SmartQueue::Selector::q2);
        REQUIRE(q.GetCount() == 3);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::Load_unload);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::Warning);

        std::optional<DequeStates> states = q.dequeue();
        REQUIRE(states.has_value());
        REQUIRE(states->current.get_fsm_type() == ClientFSM::Preheat);
        REQUIRE(states->current.get_queue_index() == QueueIndex::q0);
        REQUIRE(states->current.get_data() == data);
        REQUIRE(states->last_sent.get_fsm_type() == ClientFSM::_none);
        REQUIRE(states->last_sent.get_queue_index() == QueueIndex::q0);

        states = q.dequeue();
        REQUIRE(states.has_value());
        REQUIRE(states->current.get_fsm_type() == ClientFSM::Load_unload);
        REQUIRE(states->current.get_queue_index() == QueueIndex::q1);
        REQUIRE(states->current.get_data() == data2);
        REQUIRE(states->last_sent.get_fsm_type() == ClientFSM::_none);
        REQUIRE(states->last_sent.get_queue_index() == QueueIndex::q1);

        states = q.dequeue();
        REQUIRE(states.has_value());
        REQUIRE(states->current.get_fsm_type() == ClientFSM::Warning);
        REQUIRE(states->current.get_queue_index() == QueueIndex::q2);
        REQUIRE(states->current.get_data() == data3);
        REQUIRE(states->last_sent.get_fsm_type() == ClientFSM::_none);
        REQUIRE(states->last_sent.get_queue_index() == QueueIndex::q2);

        q.TestEmpty();
    }

    SECTION("Create 4th level - BSOD") {
        q.TestEmpty();

        // push create to Q0
        const BaseData base_data(0x25, { { 0x03, 0x00, 0x00, 0x00 } });
        REQUIRE(q.PushCreate(ClientFSM::Preheat, base_data) == SmartQueue::Selector::q0);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // push create to Q1
        const BaseData base_data2(0xAA, { { 0x02, 0x00, 0x00, 0x00 } });
        REQUIRE(q.PushCreate(ClientFSM::Load_unload, base_data2) == SmartQueue::Selector::q1);
        REQUIRE(q.GetCount() == 2);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::Load_unload);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        const BaseData base_data3(0xAA, { { 0x00, 0x00, 0x00, 0x02 } });
        REQUIRE(q.PushCreate(ClientFSM::Warning, base_data3) == SmartQueue::Selector::q2);
        REQUIRE(q.GetCount() == 3);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::Load_unload);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::Warning);
        // insertion of 4th level must fail
        REQUIRE_THROWS(q.PushCreate(ClientFSM::CrashRecovery, base_data));
    }

    SECTION("Create ClientFSM::_none - BSOD") {
        q.TestEmpty();

        // cannot push create of ClientFSM::_none to Q0
        const BaseData base_data(0x25, { { 0x03, 0x00, 0x00, 0x00 } });
        REQUIRE_THROWS(q.PushCreate(ClientFSM::_none, base_data));
    }

    SECTION("Change without creation - BSOD") {
        q.TestEmpty();

        const BaseData base_data(0x1D, { { 0xF0, 0xAB, 0xBA, 0x1A } });
        REQUIRE_THROWS(q.PushChange(ClientFSM::Preheat, base_data));
    }

    SECTION("Change wrong type - one queue full - BSOD") {
        q.TestEmpty();

        // push create to Q0
        const BaseData base_data(0x25, { { 0x03, 0x00, 0x00, 0x00 } });
        REQUIRE(q.PushCreate(ClientFSM::Preheat, base_data) == SmartQueue::Selector::q0);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // push wrong type
        REQUIRE_THROWS(q.PushChange(ClientFSM::CrashRecovery, base_data));
    }

    SECTION("Change wrong type - all queues full - BSOD") {
        q.TestEmpty();

        // push create to Q0
        const BaseData base_data(0x25, { { 0x03, 0x00, 0x00, 0x00 } });
        REQUIRE(q.PushCreate(ClientFSM::Preheat, base_data) == SmartQueue::Selector::q0);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // push create to Q1
        const BaseData base_data2(0xAA, { { 0x02, 0x00, 0x00, 0x00 } });
        REQUIRE(q.PushCreate(ClientFSM::Load_unload, base_data2) == SmartQueue::Selector::q1);
        REQUIRE(q.GetCount() == 2);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::Load_unload);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        const BaseData base_data3(0xAA, { { 0x00, 0x00, 0x00, 0x02 } });
        REQUIRE(q.PushCreate(ClientFSM::Warning, base_data3) == SmartQueue::Selector::q2);
        REQUIRE(q.GetCount() == 3);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::Load_unload);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::Warning);
        // push wrong type
        REQUIRE_THROWS(q.PushChange(ClientFSM::Selftest, base_data2));
    }

    SECTION("Change ClientFSM::_none - all queues empty - BSOD") {
        q.TestEmpty();

        // cannot change to ClientFSM::_none
        const BaseData base_data(0x25, { { 0x03, 0x00, 0x00, 0x00 } });
        REQUIRE_THROWS(q.PushChange(ClientFSM::_none, base_data));
    }

    SECTION("Change ClientFSM::_none - one queue full - BSOD") {
        q.TestEmpty();

        // push create to Q0
        const BaseData base_data(0x25, { { 0x03, 0x00, 0x00, 0x00 } });
        REQUIRE(q.PushCreate(ClientFSM::Preheat, base_data) == SmartQueue::Selector::q0);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // cannot change to ClientFSM::_none
        REQUIRE_THROWS(q.PushChange(ClientFSM::_none, base_data));
    }

    SECTION("Change ClientFSM::_none - all queues full - BSOD") {
        q.TestEmpty();

        // push create to Q0
        const BaseData base_data(0x25, { { 0x03, 0x00, 0x00, 0x00 } });
        REQUIRE(q.PushCreate(ClientFSM::Preheat, base_data) == SmartQueue::Selector::q0);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // push create to Q1
        const BaseData base_data2(0xAA, { { 0x02, 0x00, 0x00, 0x00 } });
        REQUIRE(q.PushCreate(ClientFSM::Load_unload, base_data2) == SmartQueue::Selector::q1);
        REQUIRE(q.GetCount() == 2);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::Load_unload);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        const BaseData base_data3(0xAA, { { 0x00, 0x00, 0x00, 0x02 } });
        REQUIRE(q.PushCreate(ClientFSM::Warning, base_data3) == SmartQueue::Selector::q2);
        REQUIRE(q.GetCount() == 3);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::Load_unload);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::Warning);
        // cannot change to ClientFSM::_none
        REQUIRE_THROWS(q.PushChange(ClientFSM::_none, base_data));
    }

    SECTION("Multiple change") {
        q.TestEmpty();

        // insert create, so queue is in correct state
        const BaseData data(0x0D, { { 0xF0, 0x0B, 0xB0, 0x1A } });
        q.PushCreate(ClientFSM::Preheat, data);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        std::optional<DequeStates> states = q.dequeue();
        REQUIRE(states->current.get_fsm_type() == ClientFSM::Preheat);
        REQUIRE(states->current.get_queue_index() == QueueIndex::q0);
        REQUIRE(states->current.get_data() == data);
        REQUIRE(states->last_sent.get_fsm_type() == ClientFSM::_none);
        REQUIRE(states->last_sent.get_queue_index() == QueueIndex::q0);
        q.TestEmpty();
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        const BaseData base_data(0x1D, { { 0xF0, 0xAB, 0xBA, 0x1A } });
        q.PushChange(ClientFSM::Preheat, base_data);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);
        states = q.dequeue();
        REQUIRE(states.has_value());
        REQUIRE(states->current.get_fsm_type() == ClientFSM::Preheat);
        REQUIRE(states->current.get_queue_index() == QueueIndex::q0);
        REQUIRE(states->current.get_data() == base_data);
        REQUIRE(states->last_sent.get_fsm_type() == ClientFSM::Preheat);
        REQUIRE(states->last_sent.get_queue_index() == QueueIndex::q0);

        // insertion must rewrite last one .. same type
        const BaseData base_data3(0xAA, { { 0xF0, 0xAB, 0xBA, 0x1A } });
        q.PushChange(ClientFSM::Preheat, base_data3);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        states = q.dequeue();
        REQUIRE(states.has_value());
        REQUIRE(states->current.get_fsm_type() == ClientFSM::Preheat);
        REQUIRE(states->current.get_queue_index() == QueueIndex::q0);
        REQUIRE(states->current.get_data() == base_data3); // must get data from last insertion
        REQUIRE(states->last_sent.get_fsm_type() == ClientFSM::Preheat);
        REQUIRE(states->last_sent.get_queue_index() == QueueIndex::q0);

        q.TestEmpty();
    }

    // q0_create - dequeue, q0_destroy - dont dequeue
    // q0_create - dequeue
    // otherwise we second q0_create get pushed to q1
    SECTION("Stay level 0") {
        q.TestEmpty();

        // first q0_create
        const BaseData data(0x01, { { 0xF0, 0x0B, 0xB0, 0x1A } });
        q.PushCreate(ClientFSM::Preheat, data);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);
        // dequeue of q0_create
        std::optional<DequeStates> states = q.dequeue();
        REQUIRE(states.has_value());
        REQUIRE(states->current.get_fsm_type() == ClientFSM::Preheat);
        REQUIRE(states->current.get_queue_index() == QueueIndex::q0);
        REQUIRE(states->current.get_data() == data);
        REQUIRE(states->last_sent.get_fsm_type() == ClientFSM::_none);
        REQUIRE(states->last_sent.get_queue_index() == QueueIndex::q0);
        q.TestEmpty();
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // q0_destroy
        q.PushDestroy(ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);
        // do not dequeue

        // second q0_create
        const BaseData data2(0x02, { { 0x02, 0x02, 0x02, 0x02 } });
        q.PushCreate(ClientFSM::Load_unload, data2);
        REQUIRE(q.GetCount() == 1); // it must replace destroy in q0
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Load_unload); // must be inserted to q0
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none); // can not be inserted to q1
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        states = q.dequeue();
        REQUIRE(states.has_value());
        REQUIRE(states->current.get_fsm_type() == ClientFSM::Load_unload);
        REQUIRE(states->current.get_queue_index() == QueueIndex::q0);
        REQUIRE(states->current.get_data() == data2);
        REQUIRE(states->last_sent.get_fsm_type() == ClientFSM::Preheat);
        REQUIRE(states->last_sent.get_queue_index() == QueueIndex::q0);
        q.TestEmpty();
    }

    // q0_create - dequeue
    // q0_destroy - dont dequeue
    // same q0_create with same data - dequeue - should not send anything
    SECTION("Max queue length") {
        q.TestEmpty();

        // insert create, so queue is in correct state
        const BaseData data(0x01, { { 0x02, 0x00, 0x00, 0x00 } });
        REQUIRE(q.PushCreate(ClientFSM::Preheat, data) == SmartQueue::Selector::q0);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);
        std::optional<DequeStates> states = q.dequeue();
        q.TestEmpty();

        // destroy
        q.PushDestroy(ClientFSM::Preheat);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // do not dequeue

        // push the same create as the last one
        REQUIRE(q.PushCreate(ClientFSM::Preheat, data) == SmartQueue::Selector::q0);
        // count is irrelevant, it might or might not be cleared
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // deque must not generate data to send
        states = q.dequeue();
        REQUIRE_FALSE(states.has_value());

        q.TestEmpty();
    }

    // q0_destroy, q0_create, q0_change, q1_create, q1_change, q2 create, q2 change
    SECTION("Max queue length") {
        q.TestEmpty();

        // insert create, so queue is in correct state
        const BaseData data(0x01, { { 0x02, 0x00, 0x00, 0x00 } });
        REQUIRE(q.PushCreate(ClientFSM::Preheat, data) == SmartQueue::Selector::q0);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);
        std::optional<DequeStates> states = q.dequeue();
        q.TestEmpty();

        // destroy
        q.PushDestroy(ClientFSM::Preheat);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // do not dequeue

        // push create to q0
        const BaseData data2(0x25, { { 0x03, 0x00, 0x00, 0x00 } });
        REQUIRE(q.PushCreate(ClientFSM::Preheat, data2) == SmartQueue::Selector::q0);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // push change to q0
        const BaseData base_data(0x1D, { { 0xF0, 0xAB, 0xBA, 0x1A } });
        q.PushChange(ClientFSM::Preheat, base_data);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // insertion must rewrite last change .. same type
        const BaseData base_data2(0x20, { { 0xA0, 0xAB, 0xCD, 0xFA } });
        q.PushChange(ClientFSM::Preheat, base_data2);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat); // new one (but same as old one)
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // insertion of 2nd level
        const BaseData data3(0xAA, { { 0x02, 0x00, 0x00, 0x00 } });
        REQUIRE(q.PushCreate(ClientFSM::Load_unload, data3) == SmartQueue::Selector::q1);
        REQUIRE(q.GetCount() == 2);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat); // new one (but same as old one)
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::Load_unload);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // insertion of 2nd level change
        const BaseData base_data3(0x10, { { 0x0, 0xAB, 0xCD, 0xFA } });
        REQUIRE(q.PushChange(ClientFSM::Load_unload, base_data3) == SmartQueue::Selector::q1);
        REQUIRE(q.GetCount() == 2);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::Load_unload); // new one (but same as old one)
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // insertion of 3rd level
        const BaseData data4(0xAA, { { 0x0f, 0x00, 0x00, 0x00 } });
        REQUIRE(q.PushCreate(ClientFSM::ESP, data4) == SmartQueue::Selector::q2);
        REQUIRE(q.GetCount() == 3);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat); // new one (but same as old one)
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::Load_unload);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::ESP);

        // insertion of 3rd level change
        const BaseData base_data4(0x10, { { 0x0, 0xAB, 0xCD, 0xFA } });
        REQUIRE(q.PushChange(ClientFSM::ESP, base_data4) == SmartQueue::Selector::q2);
        REQUIRE(q.GetCount() == 3);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::Load_unload); // new one (but same as old one)
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::ESP);

        // get change from 2st level queue, will be before 3rd level, because it is a creation also
        states = q.dequeue();
        REQUIRE(states.has_value());
        REQUIRE(states->current.get_fsm_type() == ClientFSM::Load_unload);
        REQUIRE(states->current.get_queue_index() == QueueIndex::q1);
        REQUIRE(states->current.get_data() == base_data3);
        REQUIRE(states->last_sent.get_fsm_type() == ClientFSM::_none); // create was never send, by setting _none here we tell client to do both create and change
        REQUIRE(states->last_sent.get_queue_index() == QueueIndex::q1);

        // get change from 3rd level queue
        states = q.dequeue();
        REQUIRE(states.has_value());
        REQUIRE(states->current.get_fsm_type() == ClientFSM::ESP);
        REQUIRE(states->current.get_queue_index() == QueueIndex::q2);
        REQUIRE(states->current.get_data() == base_data4);
        REQUIRE(states->last_sent.get_fsm_type() == ClientFSM::_none); // create was never send, by setting _none here we tell client to do both create and change
        REQUIRE(states->last_sent.get_queue_index() == QueueIndex::q2);

        // second dequeue must fail
        // q2 fsm is active, but has no data
        states = q.dequeue();
        REQUIRE_FALSE(states.has_value());

        // push destroy to clear 3rd level queue
        REQUIRE(q.PushDestroy(ClientFSM::ESP) == SmartQueue::Selector::q2);
        REQUIRE(q.GetCount() == 2); // TODO destroy in q2 + change in q0
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::Load_unload);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // push destroy to clear 2nd level queue
        REQUIRE(q.PushDestroy(ClientFSM::Load_unload) == SmartQueue::Selector::q1);
        REQUIRE(q.GetCount() == 3); // destroy in q2 + destroy in q1 + change in q0
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // push destroy to clear 1st level queue
        REQUIRE(q.PushDestroy(ClientFSM::Preheat) == SmartQueue::Selector::q0);
        REQUIRE(q.GetCount() == 3); // destroy in all queues
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // dequeue will clear 3rd level queue
        states = q.dequeue();
        REQUIRE(states.has_value());
        REQUIRE(q.GetCount() == 2);
        REQUIRE(states->current.get_fsm_type() == ClientFSM::_none);
        REQUIRE(states->current.get_queue_index() == QueueIndex::q2);
        REQUIRE(states->last_sent.get_fsm_type() == ClientFSM::ESP); // previous state is important for client
        REQUIRE(states->last_sent.get_queue_index() == QueueIndex::q2);

        // dequeue will clear 2nd level queue
        states = q.dequeue();
        REQUIRE(states.has_value());
        REQUIRE(q.GetCount() == 1);
        REQUIRE(states->current.get_fsm_type() == ClientFSM::_none);
        REQUIRE(states->current.get_queue_index() == QueueIndex::q1);
        REQUIRE(states->last_sent.get_fsm_type() == ClientFSM::Load_unload); // previous state is important for client
        REQUIRE(states->last_sent.get_queue_index() == QueueIndex::q1);

        // dequeue will clear 1nd level queue
        states = q.dequeue();
        REQUIRE(states.has_value());
        REQUIRE(states->current.get_fsm_type() == ClientFSM::_none);
        REQUIRE(states->current.get_queue_index() == QueueIndex::q0);
        REQUIRE(states->last_sent.get_fsm_type() == ClientFSM::Preheat); // previous state is important for client
        REQUIRE(states->last_sent.get_queue_index() == QueueIndex::q0);

        q.TestEmpty();
    }

    SECTION("Erase queue by pushing destroy") {
        q.TestEmpty();

        // create - to set correct state
        const BaseData data(0x25, { { 0x02, 0x00, 0x00, 0x00 } });
        REQUIRE(q.PushCreate(ClientFSM::Load_unload, data) == SmartQueue::Selector::q0);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Load_unload);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);
        std::optional<DequeStates> states = q.dequeue();
        q.TestEmpty();

        // destroy - just push in
        REQUIRE(q.PushDestroy(ClientFSM::Load_unload) == SmartQueue::Selector::q0);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // create - rewrite destroy
        const BaseData data2(0x55, { { 0x25, 0x05, 0x50, 0x05 } });
        q.PushCreate(ClientFSM::Preheat, data);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // change - rewrite create
        const BaseData base_data(0x1D, { { 0xF0, 0xAB, 0xBA, 0x1A } });
        q.PushChange(ClientFSM::Preheat, base_data);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // destroy must clear queue, without changing existing destroy
        // currently destroy means to change to ClientFSM::_none
        q.PushDestroy(ClientFSM::Preheat);
        REQUIRE(q.GetCount() == 1); // previous destroy must be stored there
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        states = q.dequeue();
        REQUIRE(states.has_value());
        REQUIRE(states->current.get_fsm_type() == ClientFSM::_none);
        REQUIRE(states->current.get_queue_index() == QueueIndex::q0);
        REQUIRE(states->last_sent.get_fsm_type() == ClientFSM::Load_unload); // this is the point of the test, it cannot be ClientFSM::Preheat, it must be the state from the last dequeued create
        REQUIRE(states->last_sent.get_queue_index() == QueueIndex::q0);

        q.TestEmpty();
    }

    // q0_create, q1_create, q1_destroy, dequeue, q0_destroy
    //  dequeue must return q0_create
    SECTION("Erase queue by pushing destroy, second level") {
        q.TestEmpty();

        // create1
        const BaseData data(0x0D, { { 0xF0, 0x0B, 0xB0, 0x1A } });
        REQUIRE(q.PushCreate(ClientFSM::Preheat, data) == SmartQueue::Selector::q0);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // insertion of 2nd level
        const BaseData data2(0x0D, { { 0xF0, 0x0B, 0xB0, 0x1B } });
        REQUIRE(q.PushCreate(ClientFSM::Load_unload, data2) == SmartQueue::Selector::q1);
        REQUIRE(q.GetCount() == 2);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::Load_unload);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // push destroy to clear 2nd level change
        REQUIRE(q.PushDestroy(ClientFSM::Load_unload) == SmartQueue::Selector::q1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // dequeue must return create of Q1
        std::optional<DequeStates> states = q.dequeue();
        REQUIRE(states.has_value());
        REQUIRE(states->current.get_fsm_type() == ClientFSM::Preheat);
        REQUIRE(states->current.get_queue_index() == QueueIndex::q0);
        REQUIRE(states->last_sent.get_fsm_type() == ClientFSM::_none);
        REQUIRE(states->last_sent.get_queue_index() == QueueIndex::q0);

        // push destroy to clear 1st level change
        REQUIRE(q.PushDestroy(ClientFSM::Preheat) == SmartQueue::Selector::q0);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        states = q.dequeue();
        REQUIRE(states.has_value());
        REQUIRE(states->current.get_fsm_type() == ClientFSM::_none);
        REQUIRE(states->current.get_queue_index() == QueueIndex::q0);
        REQUIRE(states->last_sent.get_fsm_type() == ClientFSM::Preheat);
        REQUIRE(states->last_sent.get_queue_index() == QueueIndex::q0);

        q.TestEmpty();
    }

    // q0_create, q1_create, q2_create, q2_destroy, q1_destroy, dequeue, q0_destroy
    //  dequeue must return q0_create
    SECTION("Erase queue by pushing destroy, third level") {
        q.TestEmpty();

        // create1
        const BaseData data(0x0D, { { 0xF0, 0x0B, 0xB0, 0x1A } });
        REQUIRE(q.PushCreate(ClientFSM::Preheat, data) == SmartQueue::Selector::q0);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // insertion of 2nd level
        const BaseData data2(0x0D, { { 0xF0, 0x0B, 0xB0, 0x1B } });
        REQUIRE(q.PushCreate(ClientFSM::Load_unload, data2) == SmartQueue::Selector::q1);
        REQUIRE(q.GetCount() == 2);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::Load_unload);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // insertion of 3rd level
        const BaseData data3(0x0D, { { 0xFF, 0x0B, 0xB0, 0x1B } });
        REQUIRE(q.PushCreate(ClientFSM::ESP, data3) == SmartQueue::Selector::q2);
        REQUIRE(q.GetCount() == 3);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::Load_unload);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::ESP);

        // push destroy to clear 3rd level change
        REQUIRE(q.PushDestroy(ClientFSM::ESP) == SmartQueue::Selector::q2);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::Load_unload);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // push destroy to clear 2nd level change
        REQUIRE(q.PushDestroy(ClientFSM::Load_unload) == SmartQueue::Selector::q1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // dequeue must return create of Q1
        std::optional<DequeStates> states = q.dequeue();
        REQUIRE(states.has_value());
        REQUIRE(states->current.get_fsm_type() == ClientFSM::Preheat);
        REQUIRE(states->current.get_queue_index() == QueueIndex::q0);
        REQUIRE(states->last_sent.get_fsm_type() == ClientFSM::_none);
        REQUIRE(states->last_sent.get_queue_index() == QueueIndex::q0);

        // push destroy to clear 1st level change
        REQUIRE(q.PushDestroy(ClientFSM::Preheat) == SmartQueue::Selector::q0);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        states = q.dequeue();
        REQUIRE(states.has_value());
        REQUIRE(states->current.get_fsm_type() == ClientFSM::_none);
        REQUIRE(states->current.get_queue_index() == QueueIndex::q0);
        REQUIRE(states->last_sent.get_fsm_type() == ClientFSM::Preheat);
        REQUIRE(states->last_sent.get_queue_index() == QueueIndex::q0);

        q.TestEmpty();
    }

    // q0_create, q1_create, q1_destroy, q0_destroy
    //  both queues must be empty, dequeue must fail
    SECTION("Erase queue by pushing destroy, second level - without dequeue between commands") {
        q.TestEmpty();

        // create1
        const BaseData data(0x0D, { { 0xF0, 0x0B, 0xB0, 0x1A } });
        REQUIRE(q.PushCreate(ClientFSM::Preheat, data) == SmartQueue::Selector::q0);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // insertion of 2nd level
        const BaseData data2(0x0D, { { 0xF0, 0x0B, 0xB0, 0x1B } });
        REQUIRE(q.PushCreate(ClientFSM::Load_unload, data2) == SmartQueue::Selector::q1);
        REQUIRE(q.GetCount() == 2);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::Load_unload);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // push destroy to clear 2nd level change
        REQUIRE(q.PushDestroy(ClientFSM::Load_unload) == SmartQueue::Selector::q1);
        REQUIRE(q.GetCount() == 1); // q1 erased
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // push destroy to clear 1st level change
        REQUIRE(q.PushDestroy(ClientFSM::Preheat) == SmartQueue::Selector::q0);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        q.TestEmpty(); // q0 erased

        std::optional<DequeStates> states = q.dequeue();
        REQUIRE_FALSE(states.has_value());
    }

    SECTION("Erase queue") {
        q.TestEmpty();

        // create
        const BaseData data(0x25, { { 0xF0, 0x0B, 0xB0, 0x1A } });
        REQUIRE(q.PushCreate(ClientFSM::Preheat, data) == SmartQueue::Selector::q0);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // change
        const BaseData base_data(0x1D, { { 0xF0, 0xAB, 0xBA, 0x1A } });
        q.PushChange(ClientFSM::Preheat, base_data);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // destroy
        q.PushDestroy(ClientFSM::Preheat);
        REQUIRE(q.GetCount() == 0); // create was not send, must clear the queue
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        std::optional<DequeStates> states = q.dequeue();
        REQUIRE_FALSE(states.has_value());

        q.TestEmpty();
    }

    SECTION("Erase queue and insert new destroy since there was no create/destroy") {
        q.TestEmpty();

        // create - to set correct state
        const BaseData data(0x25, { { 0xF0, 0x0B, 0xB0, 0x1A } });
        REQUIRE(q.PushCreate(ClientFSM::Preheat, data) == SmartQueue::Selector::q0);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        std::optional<DequeStates> states = q.dequeue();
        q.TestEmpty();

        // change
        const BaseData base_data(0x1D, { { 0xF0, 0xAB, 0xBA, 0x1A } });
        REQUIRE(q.PushChange(ClientFSM::Preheat, base_data) == SmartQueue::Selector::q0);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // destroy must replace change
        REQUIRE(q.PushDestroy(ClientFSM::Preheat) == SmartQueue::Selector::q0);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        states = q.dequeue();
        REQUIRE(states.has_value());
        REQUIRE(states->current.get_fsm_type() == ClientFSM::_none); // ClientFSM::_none == destroy
        REQUIRE(states->current.get_queue_index() == QueueIndex::q0);
        REQUIRE(states->last_sent.get_fsm_type() == ClientFSM::Preheat); // type of fsm being destroyed
        REQUIRE(states->last_sent.get_queue_index() == QueueIndex::q0);

        q.TestEmpty();
    }

    SECTION("Push change to lower level queue while higher level queue is active") {
        q.TestEmpty();

        // push create to Q0
        const BaseData cr_data_0(0x25, { { 0xF0, 0x0B, 0xB0, 0x1A } });
        REQUIRE(q.PushCreate(ClientFSM::Preheat, cr_data_0) == SmartQueue::Selector::q0);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // send create from Q0
        std::optional<DequeStates> states = q.dequeue();
        q.TestEmpty();

        // push create to Q1
        const BaseData cr_data_1(0x0D, { { 0xF0, 0x0B, 0xB0, 0x1B } });
        REQUIRE(q.PushCreate(ClientFSM::Load_unload, cr_data_1) == SmartQueue::Selector::q1);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::Load_unload);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // send create from Q1
        states = q.dequeue();
        q.TestEmpty();

        // push change to Q0 - while Q1 is open
        // it must be stored for later
        const BaseData ch_data_1(0x1D, { { 0xF0, 0xAB, 0xBA, 0x1A } });
        REQUIRE(q.PushChange(ClientFSM::Preheat, ch_data_1) == SmartQueue::Selector::q0);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::Load_unload);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // Q0 must not send data
        states = q.dequeue();
        REQUIRE(q.GetCount() == 1);
        REQUIRE_FALSE(states.has_value());

        // push destroy to Q1
        REQUIRE(q.PushDestroy(ClientFSM::Load_unload) == SmartQueue::Selector::q1);
        REQUIRE(q.GetCount() == 2);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // send destroy from Q1
        states = q.dequeue();
        REQUIRE(states.has_value());
        REQUIRE(states->current.get_fsm_type() == ClientFSM::_none); // ClientFSM::_none == destroy
        REQUIRE(states->current.get_queue_index() == QueueIndex::q1);
        REQUIRE(states->last_sent.get_fsm_type() == ClientFSM::Load_unload); // type of fsm being destroyed
        REQUIRE(states->last_sent.get_queue_index() == QueueIndex::q1);

        // send change from Q0
        states = q.dequeue();
        REQUIRE(states.has_value());
        REQUIRE(states->current.get_fsm_type() == ClientFSM::Preheat);
        REQUIRE(states->current.get_queue_index() == QueueIndex::q0);
        REQUIRE(states->current.get_data() == ch_data_1); // new data are the changed ones
        REQUIRE(states->last_sent.get_fsm_type() == ClientFSM::Preheat); // type did not change .. change command
        REQUIRE(states->last_sent.get_queue_index() == QueueIndex::q0);

        q.TestEmpty();
    }

    SECTION("Push change to lower level queue while higher level queue is active - do not send Q1") {
        q.TestEmpty();

        // push create to Q0
        const BaseData cr_data_0(0x25, { { 0xF0, 0x0B, 0xB0, 0x1A } });
        REQUIRE(q.PushCreate(ClientFSM::Preheat, cr_data_0) == SmartQueue::Selector::q0);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // send create from Q0
        std::optional<DequeStates> states = q.dequeue();
        q.TestEmpty();

        // push create to Q1
        const BaseData cr_data_1(0x0D, { { 0xF0, 0x0B, 0xB0, 0x1B } });
        REQUIRE(q.PushCreate(ClientFSM::Load_unload, cr_data_1) == SmartQueue::Selector::q1);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::Load_unload);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // push change to Q0 - while Q1 is open
        // it must be stored for later
        const BaseData ch_data_1(0x1D, { { 0xF0, 0xAB, 0xBA, 0x1A } });
        REQUIRE(q.PushChange(ClientFSM::Preheat, ch_data_1) == SmartQueue::Selector::q0);
        REQUIRE(q.GetCount() == 2);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::Load_unload);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // push destroy to Q1
        REQUIRE(q.PushDestroy(ClientFSM::Load_unload) == SmartQueue::Selector::q1);
        REQUIRE(q.GetCount() == 1); // Q1 was erased, Q0 has data
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // send change from Q0
        states = q.dequeue();
        REQUIRE(states.has_value());
        REQUIRE(states->current.get_fsm_type() == ClientFSM::Preheat);
        REQUIRE(states->current.get_queue_index() == QueueIndex::q0);
        REQUIRE(states->current.get_data() == ch_data_1); // new data are the changed ones
        REQUIRE(states->last_sent.get_fsm_type() == ClientFSM::Preheat); // type did not change .. change command
        REQUIRE(states->last_sent.get_queue_index() == QueueIndex::q0);

        q.TestEmpty();
    }

    SECTION("Push change to lower level queue while higher level queue is active - 2nd and 3rd level") {
        q.TestEmpty();

        // push create to Q0
        const BaseData cr_data_0(0x25, { { 0xF0, 0x0B, 0xB0, 0x1A } });
        REQUIRE(q.PushCreate(ClientFSM::Preheat, cr_data_0) == SmartQueue::Selector::q0);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // send create from Q0
        std::optional<DequeStates> states = q.dequeue();
        q.TestEmpty();

        // push create to Q1
        const BaseData cr_data_1(0x0D, { { 0xF0, 0x0B, 0xB0, 0x1B } });
        REQUIRE(q.PushCreate(ClientFSM::Load_unload, cr_data_1) == SmartQueue::Selector::q1);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::Load_unload);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // send create from Q1
        states = q.dequeue();
        q.TestEmpty();

        // push create to Q2
        const BaseData cr_data_2(0x0D, { { 0xFF, 0x0B, 0xB0, 0x1B } });
        REQUIRE(q.PushCreate(ClientFSM::ESP, cr_data_1) == SmartQueue::Selector::q2);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::Load_unload);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::ESP);

        // send create from Q2
        states = q.dequeue();
        q.TestEmpty();

        // push change to Q1 - while Q2 is open
        // it must be stored for later
        const BaseData ch_data_1(0x1D, { { 0xF0, 0xAB, 0xBA, 0x1A } });
        REQUIRE(q.PushChange(ClientFSM::Load_unload, ch_data_1) == SmartQueue::Selector::q1);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::Load_unload);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::ESP);

        // Q1 must not send data
        states = q.dequeue();
        REQUIRE(q.GetCount() == 1);
        REQUIRE_FALSE(states.has_value());

        // push destroy to Q2
        REQUIRE(q.PushDestroy(ClientFSM::ESP) == SmartQueue::Selector::q2);
        REQUIRE(q.GetCount() == 2);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::Load_unload);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // send destroy from Q2
        states = q.dequeue();
        REQUIRE(states.has_value());
        REQUIRE(states->current.get_fsm_type() == ClientFSM::_none); // ClientFSM::_none == destroy
        REQUIRE(states->current.get_queue_index() == QueueIndex::q2);
        REQUIRE(states->last_sent.get_fsm_type() == ClientFSM::ESP); // type of fsm being destroyed
        REQUIRE(states->last_sent.get_queue_index() == QueueIndex::q2);

        // send change from Q1
        states = q.dequeue();
        REQUIRE(states.has_value());
        REQUIRE(states->current.get_fsm_type() == ClientFSM::Load_unload);
        REQUIRE(states->current.get_queue_index() == QueueIndex::q1);
        REQUIRE(states->current.get_data() == ch_data_1); // new data are the changed ones
        REQUIRE(states->last_sent.get_fsm_type() == ClientFSM::Load_unload); // type did not change .. change command
        REQUIRE(states->last_sent.get_queue_index() == QueueIndex::q1);

        q.TestEmpty();
    }

    SECTION("Push change to lower level queue while higher level queue is active - 2nd and 3rd level Do not send Q2") {
        q.TestEmpty();

        // push create to Q0
        const BaseData cr_data_0(0x25, { { 0xF0, 0x0B, 0xB0, 0x1A } });
        REQUIRE(q.PushCreate(ClientFSM::Preheat, cr_data_0) == SmartQueue::Selector::q0);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // send create from Q0
        std::optional<DequeStates> states = q.dequeue();
        q.TestEmpty();

        // push create to Q1
        const BaseData cr_data_1(0x0D, { { 0xF0, 0x0B, 0xB0, 0x1B } });
        REQUIRE(q.PushCreate(ClientFSM::Load_unload, cr_data_1) == SmartQueue::Selector::q1);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::Load_unload);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // send create from Q1
        states = q.dequeue();
        q.TestEmpty();

        // push create to Q2
        const BaseData cr_data_2(0x0D, { { 0xFF, 0x0B, 0xB0, 0x1B } });
        REQUIRE(q.PushCreate(ClientFSM::ESP, cr_data_1) == SmartQueue::Selector::q2);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::Load_unload);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::ESP);

        // push change to Q1 - while Q2 is open
        // it must be stored for later
        const BaseData ch_data_1(0x1D, { { 0xF0, 0xAB, 0xBA, 0x1A } });
        REQUIRE(q.PushChange(ClientFSM::Load_unload, ch_data_1) == SmartQueue::Selector::q1);
        REQUIRE(q.GetCount() == 2);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::Load_unload);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::ESP);

        // push destroy to Q2
        REQUIRE(q.PushDestroy(ClientFSM::ESP) == SmartQueue::Selector::q2);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::Load_unload);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // send change from Q1
        states = q.dequeue();
        REQUIRE(states.has_value());
        REQUIRE(states->current.get_fsm_type() == ClientFSM::Load_unload);
        REQUIRE(states->current.get_queue_index() == QueueIndex::q1);
        REQUIRE(states->current.get_data() == ch_data_1); // new data are the changed ones
        REQUIRE(states->last_sent.get_fsm_type() == ClientFSM::Load_unload); // type did not change .. change command
        REQUIRE(states->last_sent.get_queue_index() == QueueIndex::q1);

        q.TestEmpty();
    }

    SECTION("Close Q0 while Q1 is active - BSOD") {
        q.TestEmpty();

        // push create to Q0
        const BaseData cr_data_0(0x00, { { 0x00, 0x00, 0x00, 0x00 } });
        REQUIRE(q.PushCreate(ClientFSM::Preheat, cr_data_0) == SmartQueue::Selector::q0);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // send create from Q0
        q.dequeue();
        q.TestEmpty();

        // push create to Q1
        const BaseData cr_data_1(0xFF, { { 0xFF, 0xFF, 0xFF, 0xFF } });
        REQUIRE(q.PushCreate(ClientFSM::Load_unload, cr_data_1) == SmartQueue::Selector::q1);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::Load_unload);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // push destroy to Q0 - while Q1 is open
        // it must fail
        REQUIRE_THROWS(q.PushDestroy(ClientFSM::Preheat));
    }

    SECTION("Close Q0 while Q2 is active - BSOD") {
        q.TestEmpty();

        // push create to Q0
        const BaseData cr_data_0(0x00, { { 0x00, 0x00, 0x00, 0x00 } });
        REQUIRE(q.PushCreate(ClientFSM::Preheat, cr_data_0) == SmartQueue::Selector::q0);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // send create from Q0
        q.dequeue();
        q.TestEmpty();

        // push create to Q1
        const BaseData cr_data_1(0xFF, { { 0xFF, 0xFF, 0xFF, 0xFF } });
        REQUIRE(q.PushCreate(ClientFSM::Load_unload, cr_data_1) == SmartQueue::Selector::q1);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::Load_unload);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // send create from Q1
        q.dequeue();
        q.TestEmpty();

        // push create to Q2
        const BaseData cr_data_2(0xFF, { { 0xFA, 0xFF, 0xFF, 0xFF } });
        REQUIRE(q.PushCreate(ClientFSM::ESP, cr_data_2) == SmartQueue::Selector::q2);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::Load_unload);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::ESP);

        // push destroy to Q0 - while Q1 is open
        // it must fail
        REQUIRE_THROWS(q.PushDestroy(ClientFSM::Preheat));
    }

    SECTION("Close Q0 while Q1 is active, after Q1 deque - BSOD") {
        q.TestEmpty();

        // push create to Q0
        const BaseData cr_data_0(0x00, { { 0x00, 0x00, 0x00, 0x00 } });
        REQUIRE(q.PushCreate(ClientFSM::Preheat, cr_data_0) == SmartQueue::Selector::q0);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // send create from Q0
        q.dequeue();
        q.TestEmpty();

        // push create to Q1
        const BaseData cr_data_1(0xFF, { { 0xFF, 0xFF, 0xFF, 0xFF } });
        REQUIRE(q.PushCreate(ClientFSM::Load_unload, cr_data_1) == SmartQueue::Selector::q1);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::Load_unload);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // send create from Q1
        q.dequeue();
        q.TestEmpty();

        // push destroy to Q0 - while Q1 is open
        // this time Q1 was dequeued
        // it must fail
        REQUIRE_THROWS(q.PushDestroy(ClientFSM::Preheat));
    }

    SECTION("Close Q0 while Q2 is active, after Q2 deque - BSOD") {
        q.TestEmpty();

        // push create to Q0
        const BaseData cr_data_0(0x00, { { 0x00, 0x00, 0x00, 0x00 } });
        REQUIRE(q.PushCreate(ClientFSM::Preheat, cr_data_0) == SmartQueue::Selector::q0);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // send create from Q0
        q.dequeue();
        q.TestEmpty();

        // push create to Q1
        const BaseData cr_data_1(0xFF, { { 0xFF, 0xFF, 0xFF, 0xFF } });
        REQUIRE(q.PushCreate(ClientFSM::Load_unload, cr_data_1) == SmartQueue::Selector::q1);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::Load_unload);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // send create from Q1
        q.dequeue();
        q.TestEmpty();

        // push create to Q2
        const BaseData cr_data_2(0xFF, { { 0xFA, 0xFF, 0xFF, 0xFF } });
        REQUIRE(q.PushCreate(ClientFSM::ESP, cr_data_2) == SmartQueue::Selector::q2);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::Load_unload);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::ESP);

        // send create from Q2
        q.dequeue();
        q.TestEmpty();

        // push destroy to Q0 - while Q1 is open
        // it must fail
        REQUIRE_THROWS(q.PushDestroy(ClientFSM::Preheat));
    }

    SECTION("Close Q0 while Q1 is active - without dequeue - BSOD") {
        q.TestEmpty();

        // push create to Q0
        const BaseData cr_data_0(0x20, { { 0x00, 0x00, 0x30, 0x00 } });
        REQUIRE(q.PushCreate(ClientFSM::Preheat, cr_data_0) == SmartQueue::Selector::q0);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // push create to Q1
        const BaseData cr_data_1(0xF0, { { 0xFF, 0x00, 0xFF, 0xFF } });
        REQUIRE(q.PushCreate(ClientFSM::Load_unload, cr_data_1) == SmartQueue::Selector::q1);
        REQUIRE(q.GetCount() == 2);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::Load_unload);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // push destroy to Q0 - while Q1 is open
        // it must fail
        REQUIRE_THROWS(q.PushDestroy(ClientFSM::Preheat));
    }

    SECTION("Close Q0 while Q2 is active, without deque - BSOD") {
        q.TestEmpty();

        // push create to Q0
        const BaseData cr_data_0(0x00, { { 0x00, 0x00, 0x00, 0x00 } });
        REQUIRE(q.PushCreate(ClientFSM::Preheat, cr_data_0) == SmartQueue::Selector::q0);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::_none);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // push create to Q1
        const BaseData cr_data_1(0xFF, { { 0xFF, 0xFF, 0xFF, 0xFF } });
        REQUIRE(q.PushCreate(ClientFSM::Load_unload, cr_data_1) == SmartQueue::Selector::q1);
        REQUIRE(q.GetCount() == 2);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::Load_unload);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::_none);

        // push create to Q2
        const BaseData cr_data_2(0xFF, { { 0xFA, 0xFF, 0xFF, 0xFF } });
        REQUIRE(q.PushCreate(ClientFSM::ESP, cr_data_2) == SmartQueue::Selector::q2);
        REQUIRE(q.GetCount() == 3);
        REQUIRE(q.GetOpenFsmQ0() == ClientFSM::Preheat);
        REQUIRE(q.GetOpenFsmQ1() == ClientFSM::Load_unload);
        REQUIRE(q.GetOpenFsmQ2() == ClientFSM::ESP);

        // push destroy to Q0 - while Q1 is open
        // it must fail
        REQUIRE_THROWS(q.PushDestroy(ClientFSM::Preheat));
    }
}
