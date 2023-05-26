#include <catch2/catch.hpp>
#include "eeprom_journal.hpp"
#include "crc32.h"
#include "dummy_eeprom_chip.h"
#include "configuration_store.hpp"
#include "common/eeprom_storage.hpp"
static constexpr const char *key = "data";
static constexpr size_t header_size = 6;

using namespace Journal;
using namespace journal_config_store;
inline Journal::Backend &Test_EEPROM_journal() {
    return Journal::backend_instance<100, 8096 - 100, EEPROMInstance>();
}
void reinit_journal() {
    new (&Test_EEPROM_journal()) Backend(0, 8096, EEPROMInstance());
}
CATCH_REGISTER_ENUM(Backend::BankState, Backend::BankState::Valid, Backend::BankState::MissingEndItem, Backend::BankState::Corrupted)
size_t create_transaction(size_t num_of_items, std::span<uint8_t> data, uint16_t start_id = 0) {
    if (num_of_items == 0) {
        return 0;
    }
    num_of_items += start_id;

    Backend::ItemHeader header = { false, 1, 10 };
    size_t pos = 0;

    for (; start_id < num_of_items - 1; ++start_id) {
        header.id = start_id;
        memcpy(data.data() + pos, reinterpret_cast<uint8_t *>(&header), sizeof(header));
        pos += sizeof(header) + header.len;
    }

    header.last_item = true;
    header.id = start_id;
    memcpy(data.data() + pos, reinterpret_cast<uint8_t *>(&header), sizeof(header));
    pos += sizeof(header) + header.len;

    uint32_t crc = crc32_calc_ex(0, data.data(), pos);
    memcpy(data.data() + pos, reinterpret_cast<uint8_t *>(&crc), sizeof(crc));
    pos += sizeof(crc);
    return pos;
}

size_t create_last_item(std::span<uint8_t> data) {
    memcpy(data.data(), &Backend::LAST_ITEM_STOP, sizeof(Backend::LAST_ITEM_STOP));
    uint32_t crc = crc32_calc_ex(0, data.data(), sizeof(Backend::LAST_ITEM_STOP));
    memcpy(data.data() + sizeof(Backend::LAST_ITEM_STOP), reinterpret_cast<uint8_t *>(&crc), sizeof(crc));
    return sizeof(Backend::LAST_ITEM_STOP) + sizeof(crc);
}

TEST_CASE("Test transaction validation") {
    DummyEepromChip storage;
    size_t pos = create_transaction(3, storage.get(0, 1024));
    Backend journal(0, 1024, storage);

    SECTION("Correct data") {
        auto pos_read = journal.get_next_transaction(0, 1024);
        REQUIRE(pos_read.has_value());
        auto [address, read_pos, num_of_items_loc] = pos_read.value();
        REQUIRE(read_pos == pos);
        REQUIRE(num_of_items_loc == 3);
    }
    SECTION("Short buffer") {
        auto pos_read = journal.get_next_transaction(0, pos - 10);
        REQUIRE_FALSE(pos_read.has_value());
    }
    SECTION("Corrupetd data") {
        storage.set(4, 0xcf);
        storage.set(5, 0xcf);
        auto pos_read = journal.get_next_transaction(0, 1024);
        REQUIRE_FALSE(pos_read.has_value());
    }
    SECTION("Missing last item") {
        Backend::ItemHeader header = { false, 1, 10 };
        storage.set(pos - 4 - 10 - 3, reinterpret_cast<uint8_t *>(&header), sizeof(header));
        auto pos_read = journal.get_next_transaction(0, 1024);
        REQUIRE_FALSE(pos_read.has_value());
    }
}

TEST_CASE("Test multiple transactions validation") {
    DummyEepromChip storage;
    size_t pos = 0;
    Backend journal(0, 1024, storage);

    SECTION("Just valid header") {
        auto res = journal.validate_transactions(0);
        auto [state, num_of_transactions, last_transaction] = res;
        REQUIRE(state == Backend::BankState::Corrupted);
        REQUIRE(last_transaction == pos);
        REQUIRE(num_of_transactions == 0);
    }

    SECTION("Empty bank") {
        create_last_item(storage.get(0, 1024));
        auto res = journal.validate_transactions(0);
        auto [state, num_of_transactions, last_transaction] = res;
        REQUIRE(state == Backend::BankState::Valid);
        REQUIRE(last_transaction == pos);
        REQUIRE(num_of_transactions == 0);
    }

    SECTION("Transaction without end item") {
        pos += create_transaction(3, storage.get(0, 1024));
        auto res = journal.validate_transactions(0);
        auto [state, num_of_transactions, last_transaction] = res;
        REQUIRE(state == Backend::BankState::MissingEndItem);
        REQUIRE(last_transaction == pos);
        REQUIRE(num_of_transactions == 1);
    }

    SECTION("Transaction with end item") {
        pos += create_transaction(3, storage.get(0, 1024));
        create_last_item(storage.get(pos, 1024 - pos));

        auto res = journal.validate_transactions(0);
        auto [state, num_of_transactions, last_transaction] = res;
        REQUIRE(state == Backend::BankState::Valid);
        REQUIRE(last_transaction == pos);
        REQUIRE(num_of_transactions == 1);
    }

    SECTION("Multiple transactions") {
        pos += create_transaction(3, storage.get(0 + pos, 1024 - pos));
        pos += create_transaction(5, storage.get(0 + pos, 1024 - pos));
        pos += create_transaction(8, storage.get(0 + pos, 1024 - pos));
        create_last_item(storage.get(0 + pos, 1024 - pos));

        auto res = journal.validate_transactions(0);
        auto [state, num_of_transactions, last_transaction] = res;
        REQUIRE(state == Backend::BankState::Valid);
        REQUIRE(last_transaction == pos);
        REQUIRE(num_of_transactions == 3);
    }

    SECTION("Multiple transactions without ending item") {
        pos += create_transaction(3, storage.get(0 + pos, 1024 - pos));
        pos += create_transaction(5, storage.get(0 + pos, 1024 - pos));
        pos += create_transaction(8, storage.get(0 + pos, 1024 - pos));

        auto res = journal.validate_transactions(0);
        auto [state, num_of_transactions, last_transaction] = res;
        REQUIRE(state == Backend::BankState::MissingEndItem);
        REQUIRE(last_transaction == pos);
        REQUIRE(num_of_transactions == 3);
    }
}

TEST_CASE("Test item loading") {
    DummyEepromChip storage;
    size_t pos = 0;
    Backend journal(0, 1024, storage);

    size_t num_of_items = 0;

    const auto load_fnc = [&num_of_items](uint16_t id, std::array<uint8_t, 512> &buffer, uint16_t used_bytes) {
        REQUIRE(id == num_of_items);
        num_of_items++;
        REQUIRE(used_bytes == 10);
        return true;
    };

    SECTION("Single transaction") {
        pos += create_transaction(3, storage.get(0 + pos, 1024 - pos));
        journal.load_items(0, pos, load_fnc);
        REQUIRE(num_of_items == 3);
    }
    SECTION("Multiple transactions") {
        pos += create_transaction(3, storage.get(pos, 1024 - pos));
        pos += create_transaction(3, storage.get(pos, 1024 - pos), 3);
        journal.load_items(0, pos, load_fnc);
        REQUIRE(num_of_items == 6);
    }
}

TEST_CASE("Test Bank choosing") {
    eeprom_chip.clear();
    Backend eeprom_journal(0, 8096, EEPROMInstance());

    constexpr static uint16_t second_bank_address = 8096 / 2;
    SECTION("Cold start") {
        auto res = eeprom_journal.choose_bank();
        REQUIRE_FALSE(res.has_value());
    }
    SECTION("Bank 1 valid") {
        Backend::BankHeader header { .sequence_id = 0, .version = Backend::CURRENT_VERSION };
        eeprom_journal.init_bank(Backend::BankSelector::First, header.sequence_id);
        auto res = eeprom_journal.choose_bank();
        REQUIRE(res.has_value());
        REQUIRE(res->main_bank == header);
        REQUIRE(res->main_bank_address == 0);
        REQUIRE_FALSE(res->secondary_bank.has_value());
        REQUIRE_FALSE(res->secondary_bank_address.has_value());
    }
    SECTION("Bank 2 valid") {
        Backend::BankHeader header { .sequence_id = 0, .version = Backend::CURRENT_VERSION };
        eeprom_journal.init_bank(Backend::BankSelector::Second, header.sequence_id);
        auto res = eeprom_journal.choose_bank();
        REQUIRE(res.has_value());
        REQUIRE(res->main_bank == header);
        REQUIRE(res->main_bank_address == second_bank_address);
        REQUIRE_FALSE(res->secondary_bank.has_value());
        REQUIRE_FALSE(res->secondary_bank_address.has_value());
    }
    SECTION("Both valid, bank 1 newer") {
        Backend::BankHeader primary_header { .sequence_id = 1, .version = Backend::CURRENT_VERSION };
        eeprom_journal.init_bank(Backend::BankSelector::First, primary_header.sequence_id);
        Backend::BankHeader secondary_header { .sequence_id = 0, .version = Backend::CURRENT_VERSION };
        eeprom_journal.init_bank(Backend::BankSelector::Second, secondary_header.sequence_id);
        auto res = eeprom_journal.choose_bank();
        REQUIRE(res.has_value());
        REQUIRE(res->main_bank.sequence_id == primary_header.sequence_id);
        REQUIRE(res->main_bank_address == 0);
        REQUIRE(res->secondary_bank.has_value());
        REQUIRE(res->secondary_bank_address.has_value());
        REQUIRE(res->secondary_bank->sequence_id == secondary_header.sequence_id);
        REQUIRE(res->secondary_bank_address == second_bank_address);
    }
    SECTION("Both valid, bank 2 newer") {
        Backend::BankHeader primary_header { .sequence_id = 1, .version = Backend::CURRENT_VERSION };
        eeprom_journal.init_bank(Backend::BankSelector::Second, primary_header.sequence_id);
        Backend::BankHeader secondary_header { .sequence_id = 0, .version = Backend::CURRENT_VERSION };
        eeprom_journal.init_bank(Backend::BankSelector::First, secondary_header.sequence_id);
        auto res = eeprom_journal.choose_bank();
        REQUIRE(res.has_value());
        REQUIRE(res->main_bank.sequence_id == primary_header.sequence_id);
        REQUIRE(res->main_bank_address == second_bank_address);
        REQUIRE(res->secondary_bank.has_value());
        REQUIRE(res->secondary_bank_address.has_value());
        REQUIRE(res->secondary_bank->sequence_id == secondary_header.sequence_id);
        REQUIRE(res->secondary_bank_address.value() == 0);
    }
    SECTION("Both valid, edge id values") {
        Backend::BankHeader primary_header { .sequence_id = std::numeric_limits<uint32_t>::min(), .version = Backend::CURRENT_VERSION };
        eeprom_journal.init_bank(Backend::BankSelector::Second, primary_header.sequence_id);
        Backend::BankHeader secondary_header { .sequence_id = std::numeric_limits<uint32_t>::max(), .version = Backend::CURRENT_VERSION };
        eeprom_journal.init_bank(Backend::BankSelector::First, secondary_header.sequence_id);
        auto res = eeprom_journal.choose_bank();
        REQUIRE(res.has_value());
        REQUIRE(res->main_bank.sequence_id == primary_header.sequence_id);
        REQUIRE(res->main_bank_address == second_bank_address);
        REQUIRE(res->secondary_bank.has_value());
        REQUIRE(res->secondary_bank_address.has_value());
        REQUIRE(res->secondary_bank->sequence_id == secondary_header.sequence_id);
        REQUIRE(res->secondary_bank_address.value() == 0);
    }
}

struct TestStruct {
    int32_t int32 = 0;
    std::array<uint8_t, 10> data = {};
    bool operator==(const TestStruct &rhs) const {
        return int32 == rhs.int32 && data == rhs.data;
    }
    bool operator!=(const TestStruct &rhs) const {
        return !(rhs == *this);
    }
};
struct NestedStruct {
    bool operator==(const NestedStruct &rhs) const {
        return test_struct == rhs.test_struct && is_valid == rhs.is_valid;
    }
    bool operator!=(const NestedStruct &rhs) const {
        return !(rhs == *this);
    }
    TestStruct test_struct;
    bool is_valid = false;
};

uint32_t calculate_crc(uint32_t crc, uint16_t id, bool last_item, std::span<uint8_t> data) {
    Backend::ItemHeader header { .last_item = last_item, .id = id, .len = static_cast<uint16_t>(data.size()) };
    return Backend::calculate_crc(header, data, crc);
}

TEST_CASE("Test transaction creation") {
    DummyEepromChip storage;
    static constexpr uint16_t bank_size = 8096 / 2;
    Backend journal(0, bank_size, storage);
    std::array<uint8_t, 10> data { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };

    journal.init_bank(Backend::BankSelector::First, 1);
    journal.current_address = Backend::BANK_HEADER_SIZE + Backend::CRC_SIZE;

    SECTION("Single item transaction") {
        journal.store_single_item(1, data);

        auto const [state, num_of_transactions, end_of_last_transaction] = journal.validate_transactions(Backend::BANK_HEADER_SIZE + Backend::CRC_SIZE);
        REQUIRE(state == Backend::BankState::Valid);
        REQUIRE(num_of_transactions == 1);
    }

    SECTION("Multiple item transaction") {
        REQUIRE_FALSE(journal.transaction.has_value());
        journal.transaction_start();
        REQUIRE(journal.transaction.has_value());

        uint32_t crc = 0;
        size_t item_count = 4;
        uint16_t last_item_address = journal.current_address;

        for (size_t i = 0; i < item_count; i++) {

            journal.save(1, data);

            REQUIRE(last_item_address == journal.transaction->last_item_address);
            last_item_address = journal.current_address;

            uint32_t last_item_crc = calculate_crc(crc, 1, true, data);
            crc = calculate_crc(crc, 1, false, data);
            INFO("Current item loop number is " << i + 1);
            REQUIRE(crc == journal.transaction->crc);
            REQUIRE(last_item_crc == journal.transaction->last_item_crc);
            REQUIRE(journal.transaction->item_count == i + 1);
        }

        REQUIRE(journal.transaction.has_value());
        journal.transaction_end();
        REQUIRE_FALSE(journal.transaction.has_value());

        auto const [state, num_of_transactions, end_of_last_transaction] = journal.validate_transactions(Backend::BANK_HEADER_SIZE + Backend::CRC_SIZE);
        REQUIRE(state == Backend::BankState::Valid);
        REQUIRE(num_of_transactions == 1);
    }
}

struct TestEEPROMJournalConfigV0 : public CurrentStoreConfig<Backend, Test_EEPROM_journal> {
    StoreItem<int32_t, 0, 1> int_item;
    StoreItem<TestStruct, TestStruct {}, hash("Struct item")> struct_item;
    StoreItem<NestedStruct, NestedStruct {}, hash("Nested Struct")> nested_struct_item;
};
struct TestDeprecatedEEPROMJournalItemsV0 : public DeprecatedStoreConfig<Backend> {
};

TEST_CASE("Config Store") {
    eeprom_chip.clear();
    reinit_journal();
    auto local_store = new ConfigStore<TestEEPROMJournalConfigV0, TestDeprecatedEEPROMJournalItemsV0>;
    local_store->init();
    local_store->load_all();
    REQUIRE(Test_EEPROM_journal().journal_state == Backend::JournalState::ColdStart);
    REQUIRE(Test_EEPROM_journal().current_address == Test_EEPROM_journal().start_address + 10);

    SECTION("Basic ops") {
        local_store->int_item.set(10);

        delete local_store;
        reinit_journal();
        local_store = new ConfigStore<TestEEPROMJournalConfigV0, TestDeprecatedEEPROMJournalItemsV0>;

        local_store->init();
        local_store->load_all();
        REQUIRE(Test_EEPROM_journal().current_address == Test_EEPROM_journal().start_address + 21);
        REQUIRE(Test_EEPROM_journal().journal_state == Backend::JournalState::ValidStart);
        REQUIRE(local_store->int_item.get() == 10);
        local_store->int_item.set(12);
        REQUIRE(Test_EEPROM_journal().current_address == Test_EEPROM_journal().start_address + 32);

        delete local_store;
        reinit_journal();
        local_store = new ConfigStore<TestEEPROMJournalConfigV0, TestDeprecatedEEPROMJournalItemsV0>;

        local_store->init();
        local_store->load_all();
        REQUIRE(Test_EEPROM_journal().current_address == Test_EEPROM_journal().start_address + Test_EEPROM_journal().bank_size + 21);
        REQUIRE(Test_EEPROM_journal().journal_state == Backend::JournalState::ValidStart);
        REQUIRE(local_store->int_item.get() == 12);
        local_store->int_item.set(13);

        delete local_store;
        reinit_journal();
        local_store = new ConfigStore<TestEEPROMJournalConfigV0, TestDeprecatedEEPROMJournalItemsV0>;

        local_store->init();
        local_store->load_all();
        REQUIRE(Test_EEPROM_journal().current_address == Test_EEPROM_journal().start_address + 21);
        REQUIRE(Test_EEPROM_journal().journal_state == Backend::JournalState::ValidStart);
        REQUIRE(local_store->int_item.get() == 13);
    }

    SECTION("Multiple items set and load") {
        REQUIRE(Test_EEPROM_journal().get_next_bank_start_address() == Test_EEPROM_journal().start_address + Test_EEPROM_journal().bank_size);
        local_store->int_item.set(10);
        local_store->int_item.set(11);
        local_store->int_item.set(12);

        TestStruct test_struct { 20, { 1, 2, 3, 45 } };
        NestedStruct nested_struct { test_struct, false };
        local_store->struct_item.set(test_struct);
        local_store->nested_struct_item.set(nested_struct);

        delete local_store;
        reinit_journal();
        local_store = new ConfigStore<TestEEPROMJournalConfigV0, TestDeprecatedEEPROMJournalItemsV0>;

        local_store->init();
        local_store->load_all();
        REQUIRE(Test_EEPROM_journal().get_next_bank_start_address() == Test_EEPROM_journal().start_address);
        REQUIRE(Test_EEPROM_journal().journal_state == Backend::JournalState::ValidStart);
        REQUIRE(local_store->int_item.get() == 12);
        REQUIRE(local_store->struct_item.get() == test_struct);
        REQUIRE(local_store->nested_struct_item.get() == nested_struct);
        local_store->int_item.set(14);

        delete local_store;
        local_store = new ConfigStore<TestEEPROMJournalConfigV0, TestDeprecatedEEPROMJournalItemsV0>;
        reinit_journal();
        local_store->init();
        local_store->load_all();

        REQUIRE(Test_EEPROM_journal().get_next_bank_start_address() == Test_EEPROM_journal().start_address + Test_EEPROM_journal().bank_size);
        REQUIRE(Test_EEPROM_journal().journal_state == Backend::JournalState::ValidStart);
        REQUIRE(local_store->int_item.get() == 14);
        REQUIRE(local_store->struct_item.get() == test_struct);
        REQUIRE(local_store->nested_struct_item.get() == nested_struct);
    }
}
TEST_CASE("Config store - cold start") {
    eeprom_chip.clear();
    reinit_journal();

    auto local_store = new ConfigStore<TestEEPROMJournalConfigV0, TestDeprecatedEEPROMJournalItemsV0>;
    local_store->init();
    local_store->load_all();
    REQUIRE(Test_EEPROM_journal().current_address == Test_EEPROM_journal().start_address + 10);
    REQUIRE(Test_EEPROM_journal().journal_state == Backend::JournalState::ColdStart);
    local_store->int_item.set(42);
    local_store->int_item.set(43);

    delete local_store;
    new (&Test_EEPROM_journal()) Backend(0, 1024, EEPROMInstance());
    local_store = new ConfigStore<TestEEPROMJournalConfigV0, TestDeprecatedEEPROMJournalItemsV0>;
    local_store->init();
    local_store->load_all();
    REQUIRE(Test_EEPROM_journal().journal_state == Backend::JournalState::ValidStart);
    REQUIRE(local_store->int_item.get() == 43);
}

TEST_CASE("Config store - error states") {
    eeprom_chip.clear();
    reinit_journal();
    auto local_store = new ConfigStore<TestEEPROMJournalConfigV0, TestDeprecatedEEPROMJournalItemsV0>;
    local_store->init();
    local_store->load_all();

    SECTION("Single valid bank") {

        SECTION("Missing end item in empty bank") {
            // break end item
            eeprom_chip.set(Test_EEPROM_journal().current_address + 1, eeprom_chip.get(Test_EEPROM_journal().current_address + 1) + 2);

            delete local_store;
            reinit_journal();
            local_store = new ConfigStore<TestEEPROMJournalConfigV0, TestDeprecatedEEPROMJournalItemsV0>;
            local_store->init();
            local_store->load_all();

            REQUIRE(Test_EEPROM_journal().journal_state == Backend::JournalState::CorruptedBank);
            REQUIRE(Test_EEPROM_journal().current_address == Test_EEPROM_journal().start_address + 10);

            delete local_store;
            reinit_journal();
            local_store = new ConfigStore<TestEEPROMJournalConfigV0, TestDeprecatedEEPROMJournalItemsV0>;
            local_store->init();
            local_store->load_all();

            REQUIRE(Test_EEPROM_journal().journal_state == Backend::JournalState::ValidStart);
            REQUIRE(Test_EEPROM_journal().current_address == Test_EEPROM_journal().start_address + 10);
        }

        SECTION("Missing end item with data in bank") {
            local_store->int_item.set(10);
            eeprom_chip.set(Test_EEPROM_journal().current_address + 1, eeprom_chip.get(Test_EEPROM_journal().current_address + 1) + 2);

            delete local_store;
            reinit_journal();
            local_store = new ConfigStore<TestEEPROMJournalConfigV0, TestDeprecatedEEPROMJournalItemsV0>;
            local_store->init();
            local_store->load_all();
            REQUIRE(Test_EEPROM_journal().journal_state == Backend::JournalState::MissingEndItem);
            REQUIRE(Test_EEPROM_journal().current_address == Test_EEPROM_journal().start_address + 21);
        }
    }

    SECTION("Both banks valid") {
        local_store->int_item.set(10);
        local_store->int_item.set(11);

        REQUIRE(Test_EEPROM_journal().get_current_bank_start_address() == Test_EEPROM_journal().start_address);
        delete local_store;
        reinit_journal();
        local_store = new ConfigStore<TestEEPROMJournalConfigV0, TestDeprecatedEEPROMJournalItemsV0>;
        local_store->init();
        local_store->load_all();
        REQUIRE(Test_EEPROM_journal().get_current_bank_start_address() == Test_EEPROM_journal().start_address + Test_EEPROM_journal().bank_size);
        REQUIRE(Test_EEPROM_journal().current_address == Test_EEPROM_journal().get_current_bank_start_address() + 21);

        SECTION("Missing end item, one transaction in bank") {
            eeprom_chip.set(Test_EEPROM_journal().current_address + 1, eeprom_chip.get(Test_EEPROM_journal().current_address + 1) + 2);

            delete local_store;
            reinit_journal();
            local_store = new ConfigStore<TestEEPROMJournalConfigV0, TestDeprecatedEEPROMJournalItemsV0>;
            local_store->init();
            local_store->load_all();

            REQUIRE(Test_EEPROM_journal().get_current_bank_start_address() == Test_EEPROM_journal().start_address + Test_EEPROM_journal().bank_size);

            REQUIRE(Test_EEPROM_journal().current_address == Test_EEPROM_journal().get_current_bank_start_address() + 21);
            REQUIRE(Test_EEPROM_journal().journal_state == Journal::Backend::JournalState::MissingEndItem);
            REQUIRE(local_store->int_item.get() == 11);

            local_store->int_item.set(12);

            delete local_store;
            reinit_journal();
            local_store = new ConfigStore<TestEEPROMJournalConfigV0, TestDeprecatedEEPROMJournalItemsV0>;
            local_store->init();
            local_store->load_all();

            REQUIRE(Test_EEPROM_journal().current_address == Test_EEPROM_journal().get_current_bank_start_address() + 21);
            REQUIRE(Test_EEPROM_journal().journal_state == Journal::Backend::JournalState::ValidStart);
            REQUIRE(local_store->int_item.get() == 12);
        }
        SECTION("Missing end item multiple transactions in bank") {
            local_store->int_item.set(12);
            eeprom_chip.set(Test_EEPROM_journal().current_address + 1, eeprom_chip.get(Test_EEPROM_journal().current_address + 1) + 2);

            delete local_store;
            reinit_journal();
            local_store = new ConfigStore<TestEEPROMJournalConfigV0, TestDeprecatedEEPROMJournalItemsV0>;
            local_store->init();
            local_store->load_all();

            REQUIRE(Test_EEPROM_journal().get_current_bank_start_address() == Test_EEPROM_journal().start_address);
            REQUIRE(Test_EEPROM_journal().current_address == Test_EEPROM_journal().get_current_bank_start_address() + 21);
            REQUIRE(Test_EEPROM_journal().journal_state == Journal::Backend::JournalState::MissingEndItem);
            REQUIRE(local_store->int_item.get() == 12);

            local_store->int_item.set(13);

            delete local_store;
            reinit_journal();
            local_store = new ConfigStore<TestEEPROMJournalConfigV0, TestDeprecatedEEPROMJournalItemsV0>;
            local_store->init();
            local_store->load_all();

            REQUIRE(Test_EEPROM_journal().get_current_bank_start_address() == Test_EEPROM_journal().start_address + Test_EEPROM_journal().bank_size);
            REQUIRE(Test_EEPROM_journal().current_address == Test_EEPROM_journal().get_current_bank_start_address() + 21);
            REQUIRE(Test_EEPROM_journal().journal_state == Journal::Backend::JournalState::ValidStart);
            REQUIRE(local_store->int_item.get() == 13);
        }
    }
}

constexpr std::array<int32_t, 64> default_array = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };

inline Journal::Backend &Small_Test_EEPROM_journal() {
    return Journal::backend_instance<100, 768, EEPROMInstance>();
}
struct TestEEPROMJournalConfigBigItem : public CurrentStoreConfig<Backend, Small_Test_EEPROM_journal> {
    StoreItem<std::array<int32_t, 64>, default_array, 1> random_data;
};

TEST_CASE("Bank migration during transaction") {
    eeprom_chip.clear();
    new (&Small_Test_EEPROM_journal()) Backend(100, 768, EEPROMInstance());

    auto local_store = new ConfigStore<TestEEPROMJournalConfigBigItem, TestDeprecatedEEPROMJournalItemsV0>;
    auto data = default_array;
    data[1] = 10;
    local_store->init();
    local_store->load_all();
    REQUIRE(Small_Test_EEPROM_journal().current_address == 110);
    local_store->random_data.set(data);
    REQUIRE(Small_Test_EEPROM_journal().current_address == 373);

    data[1] = 20;
    Small_Test_EEPROM_journal().transaction_start();
    local_store->random_data.set(data);
    REQUIRE(Small_Test_EEPROM_journal().current_address == 757);
    Small_Test_EEPROM_journal().transaction_end();
    REQUIRE(Small_Test_EEPROM_journal().current_address == 757);
    delete local_store;

    new (&Small_Test_EEPROM_journal()) Backend(100, 768, EEPROMInstance());
    local_store = new ConfigStore<TestEEPROMJournalConfigBigItem, TestDeprecatedEEPROMJournalItemsV0>;
    local_store->init();
    local_store->load_all();
    REQUIRE(Small_Test_EEPROM_journal().current_address == 757);
    REQUIRE(local_store->random_data.get() == data);
}

struct TestEEPROMJournalConfigV3 : public CurrentStoreConfig<Backend, Test_EEPROM_journal> {
    StoreItem<int32_t, 0, 4> int_item;
    StoreItem<TestStruct, TestStruct {}, hash("Struct item")> struct_item;
    StoreItem<NestedStruct, NestedStruct {}, hash("Nested Struct")> nested_struct_item;
};

struct TestEEPROMJournalConfigV2 : public CurrentStoreConfig<Backend, Test_EEPROM_journal> {
    StoreItem<int32_t, 0, 3> int_item;
    StoreItem<TestStruct, TestStruct {}, hash("Struct item")> struct_item;
    StoreItem<NestedStruct, NestedStruct {}, hash("Nested Struct")> nested_struct_item;
};

struct TestEEPROMJournalConfigV1 : public CurrentStoreConfig<Backend, Test_EEPROM_journal> {
    StoreItem<int32_t, 0, 2> int_item;
    StoreItem<TestStruct, TestStruct {}, hash("Struct item")> struct_item;
    StoreItem<NestedStruct, NestedStruct {}, hash("Nested Struct")> nested_struct_item;
};

struct TestDeprecatedEEPROMJournalItemsV1 : public DeprecatedStoreConfig<Backend> {
    DeprecatedStoreItem<int32_t, 0, 1, &TestEEPROMJournalConfigV1::int_item> int_item_v1;
};
struct TestDeprecatedEEPROMJournalItemsV2 : public DeprecatedStoreConfig<Backend> {
    DeprecatedStoreItem<int32_t, 0, 2, &TestEEPROMJournalConfigV2::int_item> int_item_v2;
    DeprecatedStoreItem<int32_t, 0, 1, &TestDeprecatedEEPROMJournalItemsV2::int_item_v2> int_item_v1;
};

struct TestDeprecatedEEPROMJournalItemsV3 : public DeprecatedStoreConfig<Backend> {
    DeprecatedStoreItem<int32_t, 0, 3, &TestEEPROMJournalConfigV3::int_item> int_item_v3;
    DeprecatedStoreItem<int32_t, 0, 2, &TestDeprecatedEEPROMJournalItemsV3::int_item_v3> int_item_v2;
    DeprecatedStoreItem<int32_t, 0, 1, &TestDeprecatedEEPROMJournalItemsV3::int_item_v2> int_item_v1;
};

TEST_CASE("Item migration") {
    eeprom_chip.clear();
    reinit_journal();
    auto old_store = new ConfigStore<TestEEPROMJournalConfigV0, TestDeprecatedEEPROMJournalItemsV0>;
    old_store->init();
    old_store->load_all();
    REQUIRE(Test_EEPROM_journal().journal_state == Backend::JournalState::ColdStart);
    REQUIRE(Test_EEPROM_journal().current_address == Test_EEPROM_journal().start_address + 10);

    old_store->int_item.set(10);
    REQUIRE(Test_EEPROM_journal().current_address == Test_EEPROM_journal().start_address + 21);

    delete old_store;
    reinit_journal();

    SECTION("First migration") {
        auto v1_store = new ConfigStore<TestEEPROMJournalConfigV1, TestDeprecatedEEPROMJournalItemsV1>;
        v1_store->init();
        v1_store->load_all();

        REQUIRE(Test_EEPROM_journal().journal_state == Backend::JournalState::ValidStart);
        REQUIRE(Test_EEPROM_journal().current_address == Test_EEPROM_journal().start_address + Test_EEPROM_journal().bank_size + 21);
        REQUIRE(v1_store->int_item.get() == 10);

        SECTION("Second migration") {
            auto v2_store = new ConfigStore<TestEEPROMJournalConfigV2, TestDeprecatedEEPROMJournalItemsV2>;
            v2_store->init();
            v2_store->load_all();

            REQUIRE(Test_EEPROM_journal().journal_state == Backend::JournalState::ValidStart);
            REQUIRE(Test_EEPROM_journal().current_address == Test_EEPROM_journal().start_address + 21);
            REQUIRE(v2_store->int_item.get() == 10);

            SECTION("Third migration") {
                auto v3_store = new ConfigStore<TestEEPROMJournalConfigV3, TestDeprecatedEEPROMJournalItemsV3>;
                v3_store->init();
                v3_store->load_all();

                REQUIRE(Test_EEPROM_journal().journal_state == Backend::JournalState::ValidStart);
                REQUIRE(Test_EEPROM_journal().current_address == Test_EEPROM_journal().start_address + Test_EEPROM_journal().bank_size + 21);
                REQUIRE(v3_store->int_item.get() == 10);
            }
        }
    }
}
