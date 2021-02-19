/**
 * @file fsm_types_test.cpp
 * @author Radek Vana
 * @brief Unit tests for fsm types, especially fsm::Queue
 * @date 2021-02-22
 */

//#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include "catch2/catch.hpp"

#include "fsm_types.hpp"

void test_type(ClientFSM_Command command, ClientFSM type) {
    fsm::type_t tst(command, type);
    REQUIRE(command == tst.GetCommand());
    REQUIRE(type == tst.GetType());
}

void test_type_all_commands(ClientFSM type) {
    test_type(ClientFSM_Command::none, type);
    test_type(ClientFSM_Command::create, type);
    test_type(ClientFSM_Command::destroy, type);
    test_type(ClientFSM_Command::change, type);
}

void test_create(ClientFSM type, uint8_t data) {
    fsm::create_t tst(type, data);

    REQUIRE(tst.type.GetCommand() == ClientFSM_Command::create);
    REQUIRE(tst.type.GetType() == type);
    REQUIRE(tst.data == data);
}

void test_destroy(ClientFSM type) {
    fsm::destroy_t tst(type);

    REQUIRE(tst.type.GetCommand() == ClientFSM_Command::destroy);
    REQUIRE(tst.type.GetType() == type);
}

void test_change(ClientFSM type, uint8_t phase, uint8_t progress_tot, uint8_t progress) {
    fsm::change_t tst(type, phase, progress_tot, progress);

    REQUIRE(tst.type.GetCommand() == ClientFSM_Command::change);
    REQUIRE(tst.type.GetType() == type);
    REQUIRE(tst.phase == phase);
    REQUIRE(tst.progress_tot == progress_tot);
    REQUIRE(tst.progress == progress);
}

void test_variant(ClientFSM type) {
    fsm::variant_t tst_create(fsm::create_t(type, 0));
    fsm::variant_t tst_destroy(fsm::destroy_t({ type }));
    fsm::variant_t tst_change(fsm::change_t(type, 0, 0, 0));

    REQUIRE(tst_create.GetCommand() == ClientFSM_Command::create);
    REQUIRE(tst_create.GetType() == type);

    REQUIRE(tst_destroy.GetCommand() == ClientFSM_Command::destroy);
    REQUIRE(tst_destroy.GetType() == type);

    REQUIRE(tst_change.GetCommand() == ClientFSM_Command::change);
    REQUIRE(tst_change.GetType() == type);
}

class MockQueue : public fsm::Queue {
public:
    uint8_t GetCount() const { return count; }

    void TestEmpty() {
        REQUIRE(GetCount() == 0);
        REQUIRE(Front().GetCommand() == ClientFSM_Command::none);
        REQUIRE(Back().GetCommand() == ClientFSM_Command::none);
    }
};

/*****************************************************************************/
//tests
TEST_CASE("fsm::type_t", "[fsm_type]") {
    test_type_all_commands(ClientFSM(0));
    test_type_all_commands(ClientFSM::_none); // _none != 0
    test_type_all_commands(ClientFSM::Load_unload);
}

TEST_CASE("fsm::create_t", "[fsm_create]") {
    test_create(ClientFSM(0), 0x00);
    test_create(ClientFSM(0), 0xAB);
    test_create(ClientFSM(0), 0xFF);

    // _none != 0
    test_create(ClientFSM::_none, 0x00);
    test_create(ClientFSM::_none, 0xAB);
    test_create(ClientFSM::_none, 0xFF);

    test_create(ClientFSM::Load_unload, 0x00);
    test_create(ClientFSM::Printing, 0xAB);
    test_create(ClientFSM::FirstLayer, 0xFF);
}

TEST_CASE("fsm::destroy_t", "[fsm_destroy]") {
    test_destroy(ClientFSM(0));
    test_destroy(ClientFSM::_none); // _none != 0
    test_destroy(ClientFSM::Load_unload);
}

TEST_CASE("fsm::change_t", "[fsm_change]") {
    test_change(ClientFSM(0), 0x00, 0x00, 0x00);
    test_change(ClientFSM(0), 0xAB, 0x12, 0x23);
    test_change(ClientFSM(0), 0xFF, 0xFF, 0xFF);

    // _none != 0
    test_change(ClientFSM::_none, 0x00, 0x00, 0x00);
    test_change(ClientFSM::_none, 0x01, 0x02, 0x03);
    test_change(ClientFSM::_none, 0xFF, 0xFF, 0xFF);

    test_change(ClientFSM::Load_unload, 0x00, 0x00, 0x00);
    test_change(ClientFSM::Printing, 0x00, 0x01, 0xFF);
    test_change(ClientFSM::FirstLayer, 0xFF, 0xFF, 0xFF);
}

TEST_CASE("fsm::variant_t", "[fsm_variant]") {
    test_variant(ClientFSM(0));
    test_variant(ClientFSM::_none); // _none != 0
    test_variant(ClientFSM::Load_unload);
}

TEST_CASE("fsm::Queue", "[fsm_Queue]") {
    MockQueue q;
    fsm::variant_t front = q.Front();
    fsm::variant_t back = q.Back();

    SECTION("Empty queue") {
        q.TestEmpty();
    }

    SECTION("Pass without queing") {
        q.TestEmpty();

        const uint8_t data = 3;
        q.PushCreate(ClientFSM::Preheat, data);
        front = q.Front();
        back = q.Back();
        REQUIRE(q.GetCount() == 1);
        REQUIRE(back.data == front.data);
        REQUIRE(front.GetCommand() == ClientFSM_Command::create);
        REQUIRE(front.GetType() == ClientFSM::Preheat);
        REQUIRE(front.create.data == data);
        q.Pop();
        q.TestEmpty();

        const uint8_t phase = 0x21;
        const uint8_t progress_tot = 0x32;
        const uint8_t progress = 0x45;
        q.PushChange(ClientFSM::Preheat, phase, progress_tot, progress);
        front = q.Front();
        back = q.Back();
        REQUIRE(q.GetCount() == 1);
        REQUIRE(back.data == front.data);
        REQUIRE(front.GetCommand() == ClientFSM_Command::change);
        REQUIRE(front.GetType() == ClientFSM::Preheat);
        REQUIRE(front.change.phase == phase);
        REQUIRE(front.change.progress_tot == progress_tot);
        REQUIRE(front.change.progress == progress);
        q.Pop();
        q.TestEmpty();

        q.PushDestroy(ClientFSM::Preheat);
        front = q.Front();
        back = q.Back();
        REQUIRE(q.GetCount() == 1);
        REQUIRE(back.data == front.data);
        REQUIRE(front.GetCommand() == ClientFSM_Command::destroy);
        REQUIRE(front.GetType() == ClientFSM::Preheat);
        q.Pop();
        q.TestEmpty();
    }

    SECTION("Multiple destroy") {
        q.TestEmpty();

        q.PushDestroy(ClientFSM::Preheat);
        front = q.Front();
        back = q.Back();
        REQUIRE(q.GetCount() == 1);
        REQUIRE(back.data == front.data);
        REQUIRE(front.GetCommand() == ClientFSM_Command::destroy);
        REQUIRE(front.GetType() == ClientFSM::Preheat);

        q.PushDestroy(ClientFSM::Load_unload);
        front = q.Front();
        back = q.Back();
        REQUIRE(q.GetCount() == 1);
        REQUIRE(back.data == front.data);
        REQUIRE(front.GetCommand() == ClientFSM_Command::destroy);
        REQUIRE(front.GetType() == ClientFSM::Preheat); // new destroy command cannot rewrite old one

        q.Pop();
        q.TestEmpty();
    }

    SECTION("Multiple create") {
        q.TestEmpty();

        const uint8_t data = 0x25;
        q.PushCreate(ClientFSM::Preheat, data);
        front = q.Front();
        back = q.Back();
        REQUIRE(q.GetCount() == 1);
        REQUIRE(back.data == front.data);
        REQUIRE(front.GetCommand() == ClientFSM_Command::create);
        REQUIRE(front.GetType() == ClientFSM::Preheat);
        REQUIRE(front.create.data == data);

        //insertion must fail
        const uint8_t data2 = 0xAA;
        q.PushCreate(ClientFSM::Load_unload, data2);
        front = q.Front();
        back = q.Back();
        REQUIRE(q.GetCount() == 1);
        REQUIRE(back.data == front.data);
        REQUIRE(front.GetCommand() == ClientFSM_Command::create);
        REQUIRE(front.GetType() == ClientFSM::Preheat); // old one
        REQUIRE(front.create.data == data);             // old one

        q.Pop();
        q.TestEmpty();
    }

    SECTION("Multiple change") {
        q.TestEmpty();

        const uint8_t phase = 0x21;
        const uint8_t progress_tot = 0x32;
        const uint8_t progress = 0x45;
        q.PushChange(ClientFSM::Preheat, phase, progress_tot, progress);
        front = q.Front();
        back = q.Back();
        REQUIRE(q.GetCount() == 1);
        REQUIRE(back.data == front.data);
        REQUIRE(front.GetCommand() == ClientFSM_Command::change);
        REQUIRE(front.GetType() == ClientFSM::Preheat);
        REQUIRE(front.change.phase == phase);
        REQUIRE(front.change.progress_tot == progress_tot);
        REQUIRE(front.change.progress == progress);

        //insertion must fail .. wrong type
        const uint8_t phase2 = 0x12;
        const uint8_t progress_tot2 = 0x23;
        const uint8_t progress2 = 0x5A;
        q.PushChange(ClientFSM::Load_unload, phase2, progress_tot2, progress2);
        front = q.Front();
        back = q.Back();
        REQUIRE(q.GetCount() == 1);
        REQUIRE(back.data == front.data);
        REQUIRE(front.GetCommand() == ClientFSM_Command::change);
        REQUIRE(front.GetType() == ClientFSM::Preheat);     //old one
        REQUIRE(front.change.phase == phase);               //old one
        REQUIRE(front.change.progress_tot == progress_tot); //old one
        REQUIRE(front.change.progress == progress);         //old one

        //insertion must rewrite last one .. same type
        q.PushChange(ClientFSM::Preheat, phase2, progress_tot2, progress2);
        front = q.Front();
        back = q.Back();
        REQUIRE(q.GetCount() == 1);
        REQUIRE(back.data == front.data);
        REQUIRE(front.GetCommand() == ClientFSM_Command::change);
        REQUIRE(front.GetType() == ClientFSM::Preheat);      //new one (but same as old one)
        REQUIRE(front.change.phase == phase2);               //new one
        REQUIRE(front.change.progress_tot == progress_tot2); //new one
        REQUIRE(front.change.progress == progress2);         //new one

        q.Pop();
        q.TestEmpty();
    }

    SECTION("Multiple max queue length") {
        q.TestEmpty();

        //destroy
        q.PushDestroy(ClientFSM::Preheat);
        back = q.Back();
        REQUIRE(q.GetCount() == 1);
        REQUIRE(back.GetCommand() == ClientFSM_Command::destroy);
        REQUIRE(back.GetType() == ClientFSM::Preheat);

        //create
        const uint8_t data = 0x25;
        q.PushCreate(ClientFSM::Preheat, data);
        back = q.Back();
        REQUIRE(q.GetCount() == 2);
        REQUIRE(back.GetCommand() == ClientFSM_Command::create);
        REQUIRE(back.GetType() == ClientFSM::Preheat);
        REQUIRE(back.create.data == data);

        //change
        const uint8_t phase = 0x21;
        const uint8_t progress_tot = 0x32;
        const uint8_t progress = 0x45;
        q.PushChange(ClientFSM::Preheat, phase, progress_tot, progress);
        back = q.Back();
        REQUIRE(q.GetCount() == 3);
        REQUIRE(back.GetCommand() == ClientFSM_Command::change);
        REQUIRE(back.GetType() == ClientFSM::Preheat);
        REQUIRE(back.change.phase == phase);
        REQUIRE(back.change.progress_tot == progress_tot);
        REQUIRE(back.change.progress == progress);

        /*
        //insertion must rewrite last change .. same type
        q.PushChange(ClientFSM::Preheat, phase2, progress_tot2, progress2);
        front = q.Front();
        back = q.Back();
        REQUIRE(q.GetCount() == 3);
        REQUIRE(back.data == front.data);
        REQUIRE(front.GetCommand() == ClientFSM_Command::change);
        REQUIRE(front.GetType() == ClientFSM::Preheat);      //new one (but same as old one)
        REQUIRE(front.change.phase == phase2);               //new one
        REQUIRE(front.change.progress_tot == progress_tot2); //new one
        REQUIRE(front.change.progress == progress2);         //new one
*/
        q.Pop();
        REQUIRE(q.GetCount() == 2);
        q.Pop();
        REQUIRE(q.GetCount() == 1);
        q.Pop();
        q.TestEmpty();
    }
}
