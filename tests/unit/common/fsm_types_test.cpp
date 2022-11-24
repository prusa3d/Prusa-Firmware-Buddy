/**
 * @file fsm_types_test.cpp
 * @author Radek Vana
 * @brief Unit tests for fsm types, especially fsm::SmartQueue
 * @date 2021-02-22
 */

//#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include "catch2/catch.hpp"

#include "log.h"
#include "fsm_types.hpp"

using namespace fsm;

void test_type(ClientFSM_Command command, ClientFSM type) {
    type_t tst(command, type);
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
    create_t tst(type, data);

    REQUIRE(tst.type.GetCommand() == ClientFSM_Command::create);
    REQUIRE(tst.type.GetType() == type);
    REQUIRE(tst.data == data);
}

void test_destroy(ClientFSM type) {
    destroy_t tst(type);

    REQUIRE(tst.type.GetCommand() == ClientFSM_Command::destroy);
    REQUIRE(tst.type.GetType() == type);
}

void test_change(ClientFSM type, BaseData data) {
    change_t tst(type, data);

    REQUIRE(tst.type.GetCommand() == ClientFSM_Command::change);
    REQUIRE(tst.type.GetType() == type);
    REQUIRE(tst.data == data);
}

void test_variant(ClientFSM type) {
    fsm::variant_t tst_create(create_t(type, 0));
    fsm::variant_t tst_destroy(destroy_t({ type })); // destroy_t(type) would not call ctor !!!
    fsm::variant_t tst_change(change_t(type, BaseData()));

    REQUIRE(tst_create.GetCommand() == ClientFSM_Command::create);
    REQUIRE(tst_create.GetType() == type);

    REQUIRE(tst_destroy.GetCommand() == ClientFSM_Command::destroy);
    REQUIRE(tst_destroy.GetType() == type);

    REQUIRE(tst_change.GetCommand() == ClientFSM_Command::change);
    REQUIRE(tst_change.GetType() == type);
}

class MockQueue : public fsm::SmartQueue {
public:
    size_t GetCount() const { return queue0.GetCount() + queue1.GetCount(); }

    void TestEmpty() {
        REQUIRE(GetCount() == 0);
        REQUIRE(Front().GetCommand() == ClientFSM_Command::none);
        REQUIRE(Back().GetCommand() == ClientFSM_Command::none);
    }
};

/*****************************************************************************/
//tests
TEST_CASE("fsm::type_t, create_t, destroy_t, change_t, variant_t", "[fsm]") {
    ClientFSM generator_client = GENERATE(ClientFSM(0), ClientFSM::_none, ClientFSM::Load_unload); // _none != 0 , it is last

    SECTION("type_t") {
        test_type_all_commands(generator_client);
    }

    SECTION("create_t") {
        uint8_t generator_data = GENERATE(0x00, 0xAB, 0xFF);
        test_create(generator_client, generator_data);
    }

    SECTION("destroy_t") {
        test_destroy(generator_client);
    }

    SECTION("change_t") {
        uint8_t generator_phase = GENERATE(0x00, 0xAB, 0xFF);
        PhaseData generator_phaseData = GENERATE(PhaseData({ 0x00, 0x00, 0x00, 0x00 }), PhaseData({ { 0x12, 0x23, 0x34, 0x56 } }), PhaseData({ { 0xFF, 0xFF, 0xFF, 0xFF } }));

        test_change(generator_client, BaseData(generator_phase, generator_phaseData));
    }

    SECTION("variant_t") {
        test_variant(generator_client);
    }
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
        REQUIRE(back == front);
        REQUIRE(front.GetCommand() == ClientFSM_Command::create);
        REQUIRE(front.GetType() == ClientFSM::Preheat);
        REQUIRE(front.create.data == data);
        q.Pop();
        q.TestEmpty();

        const BaseData base_data(0xCD, { { 0xAB, 0xBA, 0x1A, 0xF0 } });
        q.PushChange(ClientFSM::Preheat, base_data);
        front = q.Front();
        back = q.Back();
        REQUIRE(q.GetCount() == 1);
        REQUIRE(back == front);
        REQUIRE(front.GetCommand() == ClientFSM_Command::change);
        REQUIRE(front.GetType() == ClientFSM::Preheat);
        REQUIRE(front.change.data == base_data);
        q.Pop();
        q.TestEmpty();

        q.PushDestroy(ClientFSM::Preheat);
        front = q.Front();
        back = q.Back();
        REQUIRE(q.GetCount() == 1);
        REQUIRE(back == front);
        REQUIRE(front.GetCommand() == ClientFSM_Command::destroy);
        REQUIRE(front.GetType() == ClientFSM::Preheat);
        q.Pop();
        q.TestEmpty();
    }

    SECTION("Destroy without create") {
        q.TestEmpty();

        q.PushDestroy(ClientFSM::Preheat);
        REQUIRE(q.GetCount() == 0);
        q.TestEmpty();
    }

    SECTION("Wrong destroy") {
        q.TestEmpty();

        const uint8_t data = 3;
        q.PushCreate(ClientFSM::Load_unload, data);
        front = q.Front();
        back = q.Back();
        REQUIRE(q.GetCount() == 1);
        REQUIRE(back == front);
        REQUIRE(front.GetCommand() == ClientFSM_Command::create);
        REQUIRE(front.GetType() == ClientFSM::Load_unload);
        REQUIRE(front.create.data == data);

        //wrong destroy - nothing changed
        q.PushDestroy(ClientFSM::Preheat);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(back == front);
        REQUIRE(front.GetCommand() == ClientFSM_Command::create);
        REQUIRE(front.GetType() == ClientFSM::Load_unload);
        REQUIRE(front.create.data == data);

        //correct destroy
        q.PushDestroy(ClientFSM::Load_unload);
        q.TestEmpty();
    }

    SECTION("Multiple destroy") {
        q.TestEmpty();

        //insert create, so queue is in correct state
        const uint8_t data = 3;
        q.PushCreate(ClientFSM::Load_unload, data);
        front = q.Front();
        back = q.Back();
        REQUIRE(q.GetCount() == 1);
        REQUIRE(back == front);
        REQUIRE(front.GetCommand() == ClientFSM_Command::create);
        REQUIRE(front.GetType() == ClientFSM::Load_unload);
        REQUIRE(front.create.data == data);
        q.Pop();
        q.TestEmpty();

        //correct destroy
        q.PushDestroy(ClientFSM::Load_unload);
        front = q.Front();
        back = q.Back();
        REQUIRE(q.GetCount() == 1);
        REQUIRE(back == front);
        REQUIRE(front.GetCommand() == ClientFSM_Command::destroy);
        REQUIRE(front.GetType() == ClientFSM::Load_unload);

        //2nd destroy of same type - cannot be inserted
        q.PushDestroy(ClientFSM::Load_unload);
        front = q.Front();
        back = q.Back();
        REQUIRE(q.GetCount() == 1);
        REQUIRE(back == front);
        REQUIRE(front.GetCommand() == ClientFSM_Command::destroy);
        REQUIRE(front.GetType() == ClientFSM::Load_unload);

        //2nd destroy of wrong type - cannot be inserted
        q.PushDestroy(ClientFSM::Preheat);
        front = q.Front();
        back = q.Back();
        REQUIRE(q.GetCount() == 1);
        REQUIRE(back == front);
        REQUIRE(front.GetCommand() == ClientFSM_Command::destroy);
        REQUIRE(front.GetType() == ClientFSM::Load_unload); // new destroy command cannot rewrite old one

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
        REQUIRE(back == front);
        REQUIRE(front.GetCommand() == ClientFSM_Command::create);
        REQUIRE(front.GetType() == ClientFSM::Preheat);
        REQUIRE(front.create.data == data);

        //insertion must not fail - we have 2 level queue
        const uint8_t data2 = 0xAA;
        q.PushCreate(ClientFSM::Load_unload, data2);

        front = q.Front();
        back = q.Back();
        REQUIRE(q.GetCount() == 2);
        REQUIRE_FALSE(back == front);
        REQUIRE(front.GetCommand() == ClientFSM_Command::create);
        REQUIRE(front.GetType() == ClientFSM::Preheat); // old one
        REQUIRE(front.create.data == data);             // old one

        q.Pop();
        q.Pop();
        q.TestEmpty();
    }

    SECTION("Change without creation") {
        q.TestEmpty();

        const BaseData base_data(0x1D, { { 0xF0, 0xAB, 0xBA, 0x1A } });
        q.PushChange(ClientFSM::Preheat, base_data);
        q.TestEmpty();
    }

    SECTION("Multiple change") {
        q.TestEmpty();

        //insert create, so queue is in correct state
        const uint8_t data = 3;
        q.PushCreate(ClientFSM::Preheat, data);
        front = q.Front();
        back = q.Back();
        REQUIRE(q.GetCount() == 1);
        REQUIRE(back == front);
        REQUIRE(front.GetCommand() == ClientFSM_Command::create);
        REQUIRE(front.GetType() == ClientFSM::Preheat);
        REQUIRE(front.create.data == data);
        q.Pop();
        q.TestEmpty();

        const BaseData base_data(0x1D, { { 0xF0, 0xAB, 0xBA, 0x1A } });
        q.PushChange(ClientFSM::Preheat, base_data);
        front = q.Front();
        back = q.Back();
        REQUIRE(q.GetCount() == 1);
        REQUIRE(back == front);
        REQUIRE(front.GetCommand() == ClientFSM_Command::change);
        REQUIRE(front.GetType() == ClientFSM::Preheat);
        REQUIRE(front.change.data == base_data);

        //insertion must fail .. wrong type
        const BaseData base_data2(0x20, { { 0xF0, 0xAB, 0xBA, 0x1A } });
        q.PushChange(ClientFSM::Load_unload, base_data2);
        front = q.Front();
        back = q.Back();
        REQUIRE(q.GetCount() == 1);
        REQUIRE(back == front);
        REQUIRE(front.GetCommand() == ClientFSM_Command::change);
        REQUIRE(front.GetType() == ClientFSM::Preheat); //old one
        REQUIRE(front.change.data == base_data);        //old one

        //insertion must rewrite last one .. same type
        q.PushChange(ClientFSM::Preheat, base_data2);
        front = q.Front();
        back = q.Back();
        REQUIRE(q.GetCount() == 1);
        REQUIRE(back == front);
        REQUIRE(front.GetCommand() == ClientFSM_Command::change);
        REQUIRE(front.GetType() == ClientFSM::Preheat); //new one (but same as old one)
        REQUIRE(front.change.data == base_data2);       //new one

        q.Pop();
        q.TestEmpty();
    }

    //q0_destroy, q0_create, q0_change, q1_create, q1_change
    SECTION("Max queue length") {
        q.TestEmpty();

        //insert create, so queue is in correct state
        uint8_t data = 3;
        q.PushCreate(ClientFSM::Preheat, data);
        front = q.Front();
        back = q.Back();
        REQUIRE(q.GetCount() == 1);
        REQUIRE(back == front);
        REQUIRE(front.GetCommand() == ClientFSM_Command::create);
        REQUIRE(front.GetType() == ClientFSM::Preheat);
        REQUIRE(front.create.data == data);
        q.Pop();
        q.TestEmpty();

        //destroy
        q.PushDestroy(ClientFSM::Preheat);
        back = q.Back();
        REQUIRE(q.GetCount() == 1);
        REQUIRE(back.GetCommand() == ClientFSM_Command::destroy);
        REQUIRE(back.GetType() == ClientFSM::Preheat);

        //create
        data = 0x25;
        q.PushCreate(ClientFSM::Preheat, data);
        back = q.Back();
        REQUIRE(q.GetCount() == 2);
        REQUIRE(back.GetCommand() == ClientFSM_Command::create);
        REQUIRE(back.GetType() == ClientFSM::Preheat);
        REQUIRE(back.create.data == data);

        //change
        const BaseData base_data(0x1D, { { 0xF0, 0xAB, 0xBA, 0x1A } });
        q.PushChange(ClientFSM::Preheat, base_data);
        back = q.Back();
        REQUIRE(q.GetCount() == 3);
        REQUIRE(back.GetCommand() == ClientFSM_Command::change);
        REQUIRE(back.GetType() == ClientFSM::Preheat);
        REQUIRE(back.change.data == base_data);

        //insertion must rewrite last change .. same type
        const BaseData base_data2(0x20, { { 0xA0, 0xAB, 0xCD, 0xFA } });
        q.PushChange(ClientFSM::Preheat, base_data2);
        back = q.Back();
        REQUIRE(q.GetCount() == 3);
        REQUIRE(back.GetCommand() == ClientFSM_Command::change); //new one (but same as old one)
        REQUIRE(back.GetType() == ClientFSM::Preheat);           //new one (but same as old one)
        REQUIRE(back.change.data == base_data2);                 //new one

        //insertion of 2nd level
        const uint8_t data2 = 0xAA;
        q.PushCreate(ClientFSM::Load_unload, data2);
        REQUIRE(q.GetCount() == 4);

        front = q.Front(); //destroy
        REQUIRE(front.GetCommand() == ClientFSM_Command::destroy);
        REQUIRE(front.GetType() == ClientFSM::Preheat);

        //insertion of 3rd level must fail
        const uint8_t data3 = 0xA0;
        q.PushCreate(ClientFSM::CrashRecovery, data3);
        REQUIRE(q.GetCount() == 4);

        front = q.Front(); //destroy
        REQUIRE(front.GetCommand() == ClientFSM_Command::destroy);
        REQUIRE(front.GetType() == ClientFSM::Preheat);

        //insertion of 2nd level change
        const BaseData base_data3(0x10, { { 0x0, 0xAB, 0xCD, 0xFA } });
        q.PushChange(ClientFSM::Load_unload, base_data3);
        back = q.Back();
        REQUIRE(q.GetCount() == 5);
        REQUIRE(back.GetCommand() == ClientFSM_Command::change); //second level change
        REQUIRE(back.GetType() == ClientFSM::Preheat);           //second level change
        REQUIRE(back.change.data == base_data2);                 //second level change

        //erase destroy from 1st level queue
        q.Pop();
        front = q.Front(); //create
        REQUIRE(q.GetCount() == 4);
        REQUIRE(front.GetCommand() == ClientFSM_Command::create);
        REQUIRE(front.GetType() == ClientFSM::Preheat);
        REQUIRE(front.create.data == data);

        //erase create from 1st level queue
        q.Pop();
        front = q.Front(); //change with rewritten data
        REQUIRE(q.GetCount() == 3);
        REQUIRE(front.GetCommand() == ClientFSM_Command::create); //second level create
        REQUIRE(front.GetType() == ClientFSM::Load_unload);       //second level create
        REQUIRE(front.create.data == data2);                      //second level create

        q.Pop();
        front = q.Front();
        back = q.Back();
        REQUIRE(q.GetCount() == 2);
        REQUIRE(front.GetCommand() == ClientFSM_Command::change); //second level change
        REQUIRE(front.GetType() == ClientFSM::Load_unload);       //second level change
        REQUIRE(front.change.data == base_data3);                 //second level change
        REQUIRE(back.GetCommand() == ClientFSM_Command::change);  //first level change
        REQUIRE(back.GetType() == ClientFSM::Preheat);            //first level change
        REQUIRE(back.change.data == base_data2);                  //first level change

        //push destroy to clear 2nd level change
        q.PushDestroy(ClientFSM::Load_unload);
        front = q.Front();
        back = q.Back();
        REQUIRE(q.GetCount() == 2);
        REQUIRE(front.GetCommand() == ClientFSM_Command::destroy); //second level destroy
        REQUIRE(front.GetType() == ClientFSM::Load_unload);        //second level destroy
        REQUIRE(back.GetCommand() == ClientFSM_Command::change);   //first level change
        REQUIRE(back.GetType() == ClientFSM::Preheat);             //first level change
        REQUIRE(back.change.data == base_data2);                   //first level change

        //push destroy to clear 1st level change
        q.PushDestroy(ClientFSM::Preheat);
        front = q.Front();
        back = q.Back();
        REQUIRE(q.GetCount() == 2);
        REQUIRE(front.GetCommand() == ClientFSM_Command::destroy); //second level destroy
        REQUIRE(front.GetType() == ClientFSM::Load_unload);        //second level destroy
        REQUIRE(back.GetCommand() == ClientFSM_Command::destroy);  //first level destroy
        REQUIRE(back.GetType() == ClientFSM::Preheat);             //first level destroy

        //pop will clear 2nd level queue
        q.Pop();
        front = q.Front();
        back = q.Back();
        REQUIRE(q.GetCount() == 1);
        REQUIRE(back == front);
        REQUIRE(back.GetCommand() == ClientFSM_Command::destroy); //first level destroy
        REQUIRE(back.GetType() == ClientFSM::Preheat);            //first level destroy

        q.Pop();
        q.TestEmpty();
    }

    SECTION("Erase queue by pushing destroy") {
        q.TestEmpty();

        //create - to set correct state
        uint8_t data = 0x25;
        q.PushCreate(ClientFSM::Load_unload, data);
        back = q.Back();
        REQUIRE(q.GetCount() == 1);
        REQUIRE(back.GetCommand() == ClientFSM_Command::create);
        REQUIRE(back.GetType() == ClientFSM::Load_unload);
        REQUIRE(back.create.data == data);

        q.Pop();
        q.TestEmpty();

        //destroy
        q.PushDestroy(ClientFSM::Load_unload);
        back = q.Back();
        REQUIRE(q.GetCount() == 1);
        REQUIRE(back.GetCommand() == ClientFSM_Command::destroy);
        REQUIRE(back.GetType() == ClientFSM::Load_unload);

        //create
        data = 0x25;
        q.PushCreate(ClientFSM::Preheat, data);
        back = q.Back();
        REQUIRE(q.GetCount() == 2);
        REQUIRE(back.GetCommand() == ClientFSM_Command::create);
        REQUIRE(back.GetType() == ClientFSM::Preheat);
        REQUIRE(back.create.data == data);

        //change
        const BaseData base_data(0x1D, { { 0xF0, 0xAB, 0xBA, 0x1A } });
        q.PushChange(ClientFSM::Preheat, base_data);
        back = q.Back();
        REQUIRE(q.GetCount() == 3);
        REQUIRE(back.GetCommand() == ClientFSM_Command::change);
        REQUIRE(back.GetType() == ClientFSM::Preheat);
        REQUIRE(back.change.data == base_data);

        //destroy must clear queue, without changing existing destroy
        q.PushDestroy(ClientFSM::Preheat);
        back = q.Back();
        REQUIRE(q.GetCount() == 1);
        REQUIRE(back.GetCommand() == ClientFSM_Command::destroy);
        REQUIRE(back.GetType() == ClientFSM::Load_unload); // new destroy command cannot rewrite old one

        q.Pop();
        q.TestEmpty();
    }

    //q0_create, q1_create, q1_destroy, q0_destroy
    SECTION("Erase queue by pushing destroy, second level") {
        q.TestEmpty();

        //create1
        const uint8_t data = 0x25;
        q.PushCreate(ClientFSM::Preheat, data);
        back = q.Back();
        front = q.Front();
        REQUIRE(front == back);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(back.GetCommand() == ClientFSM_Command::create);
        REQUIRE(back.GetType() == ClientFSM::Preheat);
        REQUIRE(back.create.data == data);

        //insertion of 2nd level
        const uint8_t data2 = 0xAA;
        q.PushCreate(ClientFSM::Load_unload, data2);
        REQUIRE(q.GetCount() == 2);
        back = q.Back();
        front = q.Front();
        REQUIRE(front.GetCommand() == ClientFSM_Command::create); //first level create
        REQUIRE(front.GetType() == ClientFSM::Preheat);           //first level create
        REQUIRE(front.create.data == data);                       //first level create
        REQUIRE(back.GetCommand() == ClientFSM_Command::create);  //second level create
        REQUIRE(back.GetType() == ClientFSM::Load_unload);        //second level create
        REQUIRE(back.create.data == data2);                       //second level create

        //push destroy to clear 2nd level change
        q.PushDestroy(ClientFSM::Load_unload);
        front = q.Front();
        back = q.Back();
        REQUIRE(front == back);
        REQUIRE(q.GetCount() == 1);
        REQUIRE(front.GetCommand() == ClientFSM_Command::create); //first level create
        REQUIRE(front.GetType() == ClientFSM::Preheat);           //first level create
        REQUIRE(front.create.data == data);                       //first level create

        //push destroy to clear 1st level change
        q.PushDestroy(ClientFSM::Preheat);
        q.TestEmpty();
    }

    SECTION("Erase queue") {
        q.TestEmpty();

        //create
        const uint8_t data = 0x25;
        q.PushCreate(ClientFSM::Preheat, data);
        back = q.Back();
        REQUIRE(q.GetCount() == 1);
        REQUIRE(back.GetCommand() == ClientFSM_Command::create);
        REQUIRE(back.GetType() == ClientFSM::Preheat);
        REQUIRE(back.create.data == data);

        //change
        const BaseData base_data(0x1D, { { 0xF0, 0xAB, 0xBA, 0x1A } });
        q.PushChange(ClientFSM::Preheat, base_data);
        back = q.Back();
        REQUIRE(q.GetCount() == 2);
        REQUIRE(back.GetCommand() == ClientFSM_Command::change);
        REQUIRE(back.GetType() == ClientFSM::Preheat);
        REQUIRE(back.change.data == base_data);

        //there is no destroy must clear queue
        q.PushDestroy(ClientFSM::Preheat);
        q.TestEmpty();
    }

    SECTION("Erase queue and insert new destroy since there was no create/destroy") {
        q.TestEmpty();

        //create - to set correct state
        uint8_t data = 0x25;
        q.PushCreate(ClientFSM::Preheat, data);
        back = q.Back();
        REQUIRE(q.GetCount() == 1);
        REQUIRE(back.GetCommand() == ClientFSM_Command::create);
        REQUIRE(back.GetType() == ClientFSM::Preheat);
        REQUIRE(back.create.data == data);

        q.Pop();
        q.TestEmpty();

        //change
        const BaseData base_data(0x1D, { { 0xF0, 0xAB, 0xBA, 0x1A } });
        q.PushChange(ClientFSM::Preheat, base_data);
        back = q.Back();
        REQUIRE(q.GetCount() == 1);
        REQUIRE(back.GetCommand() == ClientFSM_Command::change);
        REQUIRE(back.GetType() == ClientFSM::Preheat);
        REQUIRE(back.change.data == base_data);

        //destroy must clear queue
        q.PushDestroy(ClientFSM::Preheat);
        back = q.Back();
        REQUIRE(q.GetCount() == 1);
        REQUIRE(back.GetCommand() == ClientFSM_Command::destroy); // change rewritten to destroy
        REQUIRE(back.GetType() == ClientFSM::Preheat);            // change rewritten to destroy

        q.Pop();
        q.TestEmpty();
    }
}
