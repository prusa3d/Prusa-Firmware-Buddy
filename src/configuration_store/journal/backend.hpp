#pragma once
#include <vector>
#include <stdint.h>
#include "crc32.h"
#include <stdlib.h>
#include <string.h>
#include <optional>
#include "st25dv64k.h"
#include <algorithm>
#include "bsod.h"
#include "freertos_mutex.hpp"
#include <mutex>
#include <variant>
#include "store_item.hpp"
#include <span>
#include <memory>
#include "indices.hpp"
#include "storage.hpp"
namespace Journal {

/**
 * This class does the most of work for the Journal eeprom.
 *
 * The eeprom part given to the backend is split to two banks. Each bank has header which contains header and transactions.
 *
 * Transaction can have 1 to N items. Last item in transaction has LastItem flag set to true and that means that 4 bytes after the data is CRC of the transaction
 *
 *  +------------+              +-------------+
 *  | ItemHeader |              | Transaction |
 *  +------------+              +-------------+
 *  | LastItem   |              |   Items     |
 *  | Id         |              |     .       |
 *  | DataLen    |              |     .       |
 *  +------------+              |     .       |
 *  | Data       |              |  LastItem   | <- has LastItem set to true
 *  +------------+              +-------------+
 *                              | CRC32       |
 *                              +-------------+
 *
 * When we reach the end of bank or there is more than one transaction or we have migrated item in the configuration store we will migrate the bank to the other bank.
 * This means we will create a transaction in the other bank, which contains all non default values.
 */
class Backend {
    using Address = uint16_t;
    using Offset = uint16_t;
    using Id = uint16_t;
    using BankSequenceId = uint32_t;

public:
    enum class JournalState {
        ColdStart,
        ValidStart,
        MissingEndItem,
        CorruptedBank,
    };

    static constexpr Id LAST_ITEM_ID = 0x0F;
    static constexpr size_t MAX_ITEM_SIZE = 512;
    static constexpr auto RESERVED_IDS = std::to_array<Id>({ LAST_ITEM_ID });

private:
#ifdef EEPROM_UNITTEST
public:
#endif
    struct [[gnu::packed]] ItemHeader {
        bool last_item : 1;
        unsigned int id : 14;
        uint16_t len : 9;
    };
    static_assert(sizeof(ItemHeader) == 3);

    static constexpr uint16_t CURRENT_VERSION = 1;
    struct [[gnu::packed]] BankHeader {
        uint32_t sequence_id;
        uint16_t version;

        bool operator==(const BankHeader &other) const {
            return sequence_id == other.sequence_id && version == other.version;
        }
    };
    static_assert(sizeof(BankHeader) == 6);

    enum class BankState {
        Valid, // found ending item or at least one item
        MissingEndItem,
        Corrupted, // no valid transaction present
    };

    enum class BankSelector {
        First,
        Second
    };

    struct MultipleTransactionValidationResult {
        BankState state;
        uint16_t num_of_transactions;
        uint16_t end_of_last_transaction;
    };
    struct TransactionValidationResult {
        uint16_t address;
        uint16_t transaction_len;
        uint16_t num_of_items;
    };
    struct ItemLoadResult {
        ItemHeader header;
        std::span<uint8_t> data;
    };

    struct BanksState {
        BankHeader main_bank;
        uint16_t main_bank_address;

        std::optional<BankHeader> secondary_bank;
        std::optional<uint16_t> secondary_bank_address;
    };

    enum class LoadAction {
        Nothing,
        ChangeBank,
        MigrateBank,
        FixEndItem,
        FixEndItemAndMigrate,
    };

    static constexpr ItemHeader LAST_ITEM_STOP = { .last_item = true, .id = LAST_ITEM_ID, .len = 0 };
    static constexpr size_t ITEM_HEADER_SIZE = sizeof(ItemHeader);
    static constexpr size_t BANK_HEADER_SIZE = sizeof(BankHeader);
    using CRCType = uint32_t;
    static constexpr size_t CRC_SIZE = sizeof(CRCType);

    using CallbackFunction = std::function<void(ItemHeader, std::array<uint8_t, MAX_ITEM_SIZE> &)>;

    struct Transaction {
        Backend &backend;

        bool check_size = true;
        Address last_item_address = backend.current_address;
        CRCType crc = 0;
        CRCType last_item_crc = 0;
        ItemHeader last_item_header = { true, 0, 0 };
        uint16_t item_count = 0;

        Transaction(bool check_size, Backend &backend);
        ~Transaction();
        void calculate_crc(Id id, const std::span<uint8_t> &data);
        void store_item(Id id, const std::span<uint8_t> &data);
        void cancel();
    };

    void transaction_start();
    void transaction_end();

    void migration_start();
    void migration_end();

    std::optional<Transaction> transaction = std::nullopt;
    std::optional<Transaction> migration = std::nullopt;
    Address start_address;
    Offset bank_size;

    Address current_address = 0;
    uint32_t current_bank_id = 0;

    JournalState journal_state = JournalState::ValidStart;
    std::function<void(void)> dump_callback;
    Storage &storage;

    FreeRTOS_Mutex mutex;

    LoadAction handle_valid(const UpdateFunction &update_function, uint16_t num_of_transactions);
    LoadAction handle_missing_end_item(const UpdateFunction &update_function, uint16_t num_of_transactions);

    static std::optional<BankHeader> validate_bank_header(const std::span<uint8_t> &data);

    std::optional<ItemLoadResult> load_item(Address address, Offset free_space, const std::span<uint8_t> &buffer);
    bool load_items(Address address, Offset len_of_transactions, const UpdateFunction &update_function);

    std::optional<Offset> map_over_transaction_unchecked(const Address address, const Offset free_space, const CallbackFunction &fnc);
    /**
     * @attention fnc callback can be called with invalid data
     * the callback results are valid only if the returned value has value
     * currently only used to validate and count items in transactions
     */
    std::optional<Offset> map_over_transaction(const Address address, const Offset free_space, CallbackFunction fnc);
    std::optional<TransactionValidationResult> get_next_transaction(Address address, const Offset free_space);
    MultipleTransactionValidationResult validate_transactions(const Address address);

    std::optional<CRCType> get_crc(const Address address, const Offset free_space);
    static std::optional<CRCType> get_crc(const std::span<uint8_t> data);
    static CRCType calculate_crc(const Backend::ItemHeader &, const std::span<uint8_t> &data, CRCType crc = 0);

    void init_bank(const BankSelector bank, uint32_t id);
    std::optional<Backend::BanksState> choose_bank() const;
    void migrate_bank();
    bool fits_in_current_bank(uint16_t size) const;
    uint16_t get_free_space_in_current_bank() const;
    Address get_current_bank_start_address() const;
    [[nodiscard]] Address get_next_bank_start_address() const;
    BankSelector get_next_bank();
    Address get_bank_start_address(const BankSelector selector);

    uint16_t write_item(const Address address, const Backend::ItemHeader &, const std::span<uint8_t> &data, std::optional<CRCType> crc);
    uint16_t write_end_item(Address address);
    void store_single_item(Id id, const std::span<uint8_t> &data);

public:
    void load_all(const UpdateFunction &update_function);

    void init(const DumpCallback &callback);

    void save(uint16_t id, std::span<uint8_t> data);
    std::unique_lock<FreeRTOS_Mutex> lock();

    /**
     * @brief Invalidates the values stored in eeprom in both banks
     *
     * Writes 0xff to each address in eeprom
     * Restart or reinitialization of eeprom is needed after this function
     */
    void reset();

    Backend(uint16_t offset, uint16_t size, Storage &storage);
};

template <uint16_t ADDRESS, uint16_t SIZE, Storage &(storage)()>
inline Backend &backend_instance() {
    static Backend eepromJournal(ADDRESS, SIZE, storage());
    return eepromJournal;
}
}
