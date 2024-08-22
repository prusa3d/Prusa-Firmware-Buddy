#pragma once
#include <inplace_function.hpp>
#include <stdint.h>
#include "crc32.h"
#include <stdlib.h>
#include <string.h>
#include <optional>
#include "st25dv64k.h"
#include <algorithm>
#include "bsod.h"
#include <freertos/mutex.hpp>
#include <mutex>
#include <variant>
#include "store_item.hpp"
#include <span>
#include <memory>
#include <storage_drivers/storage.hpp>
#include <assert.h>
#include <type_traits>

namespace journal {

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
 * When we reach the end of bank or there is more than one transaction or we have migrated item in the configuration store we will migrate the bank to the next bank.
 * This means we will create a transaction in the next bank, which contains all non default values.
 */
class Backend {
public:
    using Address = uint16_t;
    using Offset = uint16_t;
    using Id = uint16_t;
    using BankSequenceId = uint32_t;

    enum class JournalState {
        ColdStart,
        ValidStart,
        MissingEndItem,
        CorruptedBank,
    };

    static constexpr Id LAST_ITEM_ID = 0x0F;
    static constexpr size_t MAX_ITEM_SIZE = 512;
    static constexpr auto RESERVED_IDS = std::to_array<Id>({ LAST_ITEM_ID });

    /**
     * @brief Array of these is passed in init to be run in order of oldest -> newest. The journal is scanned in search of deprecated_ids and if it finds them, it is marked that 'this and all newer migration functions need to be run'.
     *
     */
    struct MigrationFunction {
        using MigrationFnT = void (*)(Backend &backend);
        MigrationFnT migration_fn;
        std::span<const Id> deprecated_ids;
    };

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

        bool operator==(const BankHeader &) const = default;
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

    using CRCType = uint32_t;

    static constexpr ItemHeader LAST_ITEM_STOP = { .last_item = true, .id = LAST_ITEM_ID, .len = 0 };
    static constexpr size_t ITEM_HEADER_SIZE = sizeof(ItemHeader);
    static constexpr size_t BANK_HEADER_SIZE = sizeof(BankHeader);
    static constexpr size_t CRC_SIZE = sizeof(CRCType);
    static constexpr size_t END_ITEM_SIZE_WITH_CRC = ITEM_HEADER_SIZE + CRC_SIZE;
    static constexpr size_t BANK_HEADER_SIZE_WITH_CRC = BANK_HEADER_SIZE + CRC_SIZE;

    using CallbackFunction = stdext::inplace_function<void(ItemHeader, std::array<uint8_t, MAX_ITEM_SIZE> &)>;

    struct Transaction {
        enum class Type {
            bank_migration, // bank flipping
            transaction, // writing to normal bank
            version_migration, // writing to the next bank (needed during migrations from older version)
        };

        Backend &backend;

        Type type = Type::transaction;
        Address last_item_address = type == Type::version_migration ? backend.current_next_address : backend.current_address;
        CRCType crc = 0;
        CRCType last_item_crc = 0;
        ItemHeader last_item_header = { true, 0, 0 };
        uint16_t item_count = 0;

        Transaction(Type type, Backend &backend);
        ~Transaction();
        void calculate_crc(Id id, const std::span<const uint8_t> &data);
        void store_item(Id id, const std::span<const uint8_t> &data);

        /// Called if bank migration happens during a transaction – that renders the transaction invalid.
        /// Throws away the previous transaction data and reinitializes the transaction context, so that the transaction can continue.
        /// In this case, we lose the atomicity of the transaction.
        void reinitialize();
    };

    /**
     * @brief Helper type for TransactionGuards
     *
     * @tparam (Backend::*StartFnc)()
     * @tparam (Backend::*EndFnc)()
     */
    template <void (Backend::*StartFnc)(), void (Backend::*EndFnc)()>
    struct [[nodiscard]] TransactionRAII {
        TransactionRAII(Backend &backend_)
            : backend(backend_) {
            (backend.*StartFnc)();
        }
        ~TransactionRAII() {
            (backend.*EndFnc)();
        }

        TransactionRAII(const TransactionRAII &other) = delete;
        TransactionRAII(TransactionRAII &&other) = delete;
        TransactionRAII &operator=(const TransactionRAII &other) = delete;
        TransactionRAII &operator=(TransactionRAII &&other) = delete;

        Backend &backend;
    };

    /**
     * @brief Reads all items from the current bank and executes callback with stored the header and binary data
     *
     * @param callback
     */
    void read_all_current_bank_items(const CallbackFunction &callback);
    /**
     * @brief   Reads all items from both banks and executes callback with the stored header and binary data. This is meant to be used by migrating functions from older versions, otherwise this will read 'old' data rather than 'temporary migrated data'.
     *
     * @param callback
     */
    void read_items_for_migrations(const CallbackFunction &callback);

    void erase_storage_area();

    /**
     * @brief If needed to initialize without having state set to cold_start, this will override the cold_start to valid_start
     *
     */
    void override_cold_start_state();

    /**
     * @brief Shorthand meant for migrating functions to easily save data.
     *
     * @tparam T - intentionally prevented from being automatically deduced, it has bitten us before - see BFW-5938.
     * We need precise control over the type of the items that we're saving,
     * because if the record size does not match the expected value type size of a config store item,
     * bsods happen (or even worse, they do not happen and the printer just doesn't boot).
     *
     * So we're enforcing explicit specification of T to prevent accidental wrong type deductions,
     * for example "0" being deduced as int, when the record type is uint8_t.
     *
     * @param hashed_id
     * @param item_to_be_saved
     */
    template <typename T>
    void save_migration_item(Id hashed_id, const std::type_identity_t<T> &item_to_be_saved) {
        static_assert(sizeof(T) <= MAX_ITEM_SIZE, "Trying to save an item too big");
        assert(transaction.has_value() && transaction->type == Transaction::Type::version_migration); // migrating transaction must be in progress

        std::array<uint8_t, sizeof(T)> buffer;
        memcpy(buffer.data(), &item_to_be_saved, sizeof(T)); // Load the buffer with data
        save(hashed_id, { buffer.data(), sizeof(T) }); // Save the data into the backend
    }

private:
#ifdef EEPROM_UNITTEST
public:
#endif

    void transaction_start();
    void transaction_end();
    using TransactionGuard = TransactionRAII<&Backend::transaction_start, &Backend::transaction_end>;

    void bank_migration_start();
    void bank_migration_end();
    using BankMigrationGuard = TransactionRAII<&Backend::bank_migration_start, &Backend::bank_migration_end>;
    BankMigrationGuard bank_migration_guard();

    void version_migration_start();
    void version_migration_end();
    using VersionMigratingTransactionGuard = TransactionRAII<&Backend::version_migration_start, &Backend::version_migration_end>;
    VersionMigratingTransactionGuard version_migration_guard();

    std::optional<Transaction> transaction = std::nullopt;
    std::optional<Transaction> bank_migration = std::nullopt;
    const Address start_address;
    Offset bank_size;

    Address current_address = 0; // current position of the main bank 'end' (where next item will be stored ie without end item transaction)
    Address current_next_address = 0; // current position of the next bank 'end' (needed for migrating_transaction)
    uint32_t current_bank_id = 0;

    /**
     * @brief Generates intermediary transactions of migration functions into the next bank
     * @param migration_functions array of previous 'migration versions' that will run (and all following functions) if one of deprecated_ids is found in the journal
     *
     * @return true if found a deprecated item
     */

    bool generate_version_migration_intermediaries(std::span<const MigrationFunction> migration_functions);

    /**
     * @brief Finds the oldest index of migration version in the migration_functions span.
     *
     * @param migration_functions migration versions
     * @return returns min index into migration_functions that has at least one of deprecated_ids found in current bank
     */
    size_t find_oldest_version_migration_index(std::span<const MigrationFunction> migration_functions);

    /**
     * @brief Loads migrated intermediary data from the next bank into RAM mirror
     *
     * @param update_function
     */
    void load_version_migrated_data(const UpdateFunction &update_function);

    JournalState journal_state = JournalState::ValidStart;

    stdext::inplace_function<void(void)> dump_callback;
    configuration_store::Storage &storage;

    freertos::Mutex mutex;

    static std::optional<BankHeader> validate_bank_header(const std::span<const uint8_t> &data);

    std::optional<ItemLoadResult> load_item(Address address, Offset free_space, const std::span<uint8_t> &buffer);
    void load_items(Address address, Offset len_of_transactions, const UpdateFunction &update_function);

    /**
     * @brief Reads all items in the given range
     *
     * @param address Where to start reading
     * @param free_space Where to stop reading
     * @param fnc Function to be executed with every read item
     */
    void read_all_items(Address address, Offset free_space, const CallbackFunction &fnc);
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
    static std::optional<CRCType> get_crc(const std::span<const uint8_t> data);
    static CRCType calculate_crc(const Backend::ItemHeader &, const std::span<const uint8_t> &data, CRCType crc = 0);

    void init_bank(const BankSelector bank, uint32_t id, bool is_next_bank = false);
    std::optional<Backend::BanksState> choose_bank() const;
    void migrate_bank();

    bool fits_in_current_bank(uint16_t size) const;

    uint16_t get_free_space_in_bank(Address address_in_bank) const;
    uint16_t get_free_space_in_current_bank() const {
        return get_free_space_in_bank(current_address);
    }

    Address get_bank_start_address(const BankSelector selector);
    Address get_bank_start_address(Address address_in_bank) const;
    inline Address get_current_bank_start_address() const {
        return get_bank_start_address(current_address);
    }

    [[nodiscard]] Address get_next_bank_start_address() const;
    BankSelector get_next_bank();

    uint16_t write_item(Address address, Backend::ItemHeader, const std::span<const uint8_t> &data, std::optional<CRCType> crc);
    uint16_t write_end_item(Address address);
    void store_single_item(Id id, const std::span<const uint8_t> &data);

public:
    void load_all(const UpdateFunction &update_function, std::span<const MigrationFunction> migration_functions);

    void init(const DumpCallback &callback);

    void save(uint16_t id, std::span<const uint8_t> data);
    std::unique_lock<freertos::Mutex> lock();
    JournalState get_journal_state() const;

    /**
     * @brief Invalidates the values stored in eeprom in both banks
     *
     * Writes 0xff to each address in eeprom
     * Restart or reinitialization of eeprom is needed after this function
     */
    void reset();

    /**
     * @brief In case there's gonna be multiple writes in succession,  all the writes can be put into one transaction by starting a transaction via this guard and releasing the guard once the transaction is done
     *
     */
    TransactionGuard transaction_guard();

    Backend(uint16_t offset, uint16_t size, configuration_store::Storage &storage);
    Backend(const Backend &other) = delete;
    Backend(Backend &&other) = delete;
    Backend &operator=(const Backend &other) = delete;
    Backend &operator=(Backend &&other) = delete;
};

template <uint16_t ADDRESS, uint16_t SIZE, configuration_store::Storage &(storage)()>
inline Backend &backend_instance() {
    static Backend eepromJournal { ADDRESS, SIZE, storage() };
    return eepromJournal;
}
} // namespace journal
