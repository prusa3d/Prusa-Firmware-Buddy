#include "backend.hpp"
#include "timing.h"
#include "assert.h"
#include <memory>
#include <utility>
#include <ranges>

namespace journal {
std::unique_lock<FreeRTOS_Mutex> Backend::lock() {
    return std::unique_lock<FreeRTOS_Mutex>(mutex);
}
std::optional<uint16_t> Backend::map_over_transaction(Backend::Address address, Backend::Offset free_space, CallbackFunction fnc) {
    CRCType crc_comp = 0;

    auto transaction_len = map_over_transaction_unchecked(address, free_space,
        [&crc_comp, &fnc](ItemHeader header, std::array<uint8_t, MAX_ITEM_SIZE> &buffer) {
            crc_comp = crc32_calc_ex(crc_comp, reinterpret_cast<const uint8_t *>(&header), ITEM_HEADER_SIZE);
            crc_comp = crc32_calc_ex(crc_comp, buffer.data(), header.len);

            // this is now OK, it is currently just used to count the items. No loading is taking place with this callback
            fnc(header, buffer);
        });
    if (!transaction_len.has_value()) {
        return std::nullopt;
    }

    auto crc_read = get_crc(address + transaction_len.value() - CRC_SIZE, free_space - transaction_len.value() + CRC_SIZE);
    if (!crc_read.has_value()) {
        return std::nullopt;
    }

    if (crc_read == crc_comp) {
        return transaction_len.value();
    }

    return std::nullopt;
}

void Backend::read_all_current_bank_items(const CallbackFunction &callback) {
    // precondition: current_address is set and current bank has valid data
    Address start_address = get_current_bank_start_address() + BANK_HEADER_SIZE + CRC_SIZE;
    read_all_items(start_address, current_address - start_address, callback);
}

void Backend::read_items_for_migrations(const CallbackFunction &callback) {
    // precondition: current_next_address is set and next bank has valid data (nothing or migrated intermediary transactions)
    read_all_current_bank_items(callback);

    Address start_address = get_next_bank_start_address() + BANK_HEADER_SIZE + CRC_SIZE;
    read_all_items(start_address, current_next_address - start_address, callback);
}

void Backend::read_all_items(Address address, Offset len_of_transactions, const CallbackFunction &fnc) {
    bool last_item = false;

    len_of_transactions = std::min(len_of_transactions, static_cast<Offset>(bank_size - BANK_HEADER_SIZE - CRC_SIZE));

    auto last_item_clb_wrapper = [&last_item, &fnc](journal::Backend::ItemHeader header, std::array<uint8_t, journal::Backend::MAX_ITEM_SIZE> &buffer) -> void {
        if (header.id == journal::Backend::LAST_ITEM_STOP.id) {
            last_item = true;
            return;
        }

        fnc(header, buffer);
    };

    while (!last_item && len_of_transactions > 0) {
        auto res = map_over_transaction_unchecked(address, len_of_transactions, last_item_clb_wrapper);
        if (!res.has_value()) {
            // should not happen, already checked validity
            bsod("Error while loading items");
        }
        address += res.value();
        len_of_transactions -= res.value();
    };
}

std::optional<uint16_t> Backend::map_over_transaction_unchecked(const Backend::Address address, const Backend::Offset free_space, const CallbackFunction &callback) {
    std::array<uint8_t, MAX_ITEM_SIZE> buffer {};

    for (uint16_t pos = 0; pos < free_space;) {
        auto header_opt = load_item(address + pos, free_space - pos, buffer);
        if (!header_opt.has_value()) {
            // item did not fit inside bank
            return std::nullopt;
        }
        auto [header, item_data] = header_opt.value();

        callback(header, buffer);

        pos += header.len + ITEM_HEADER_SIZE;

        if (header.last_item) {
            if (pos + CRC_SIZE > free_space) {
                return std::nullopt;
            } else {
                return pos + CRC_SIZE;
            }
        }
    }
    return std::nullopt;
}
std::optional<Backend::TransactionValidationResult> Backend::get_next_transaction(uint16_t address, const Offset free_space) {
    uint16_t num_of_items = 0;

    auto fnc = [&num_of_items]([[maybe_unused]] ItemHeader header, [[maybe_unused]] std::array<uint8_t, MAX_ITEM_SIZE> &buffer) {
        num_of_items++;
    };

    auto transaction_len = map_over_transaction(address, free_space, fnc);
    if (!transaction_len.has_value()) {
        return std::nullopt;
    }

    return TransactionValidationResult { .address = address, .transaction_len = transaction_len.value(), .num_of_items = num_of_items };
}

std::optional<Backend::ItemLoadResult> Backend::load_item(uint16_t address, uint16_t free_space, const std::span<uint8_t> &buffer) {
    if (free_space < ITEM_HEADER_SIZE) {
        return std::nullopt;
    }
    ItemHeader header = { false, 0, 0 };
    storage.read_bytes(address, { reinterpret_cast<uint8_t *>(&header), ITEM_HEADER_SIZE });

    if (free_space < ITEM_HEADER_SIZE + header.len) {
        return std::nullopt;
    }

    auto item_data = buffer.subspan(0, header.len);
    storage.read_bytes(address + ITEM_HEADER_SIZE, item_data);

    return ItemLoadResult { .header = header, .data = item_data };
}
std::optional<Backend::CRCType> Backend::get_crc(const uint16_t address, const uint16_t free_space) {
    if (free_space < CRC_SIZE) {
        return std::nullopt;
    }
    CRCType crc;
    storage.read_bytes(address, { reinterpret_cast<uint8_t *>(&crc), CRC_SIZE });
    return crc;
}

std::optional<Backend::CRCType> Backend::get_crc(const std::span<uint8_t> data) {
    if (data.size() < CRC_SIZE) {
        return std::nullopt;
    }
    CRCType crc;
    memcpy(&crc, data.data(), CRC_SIZE);
    return crc;
}
size_t Backend::find_oldest_migration_index(std::span<const MigrationFunction> migration_functions) {
    size_t oldest_migration = migration_functions.size();

    auto callback = [&migration_functions, &oldest_migration](ItemHeader header, [[maybe_unused]] std::array<uint8_t, MAX_ITEM_SIZE> &buffer) -> void {
        for (size_t i = 0; i < oldest_migration; ++i) {
            if (std::ranges::any_of(
                    migration_functions[i].deprecated_ids,
                    [&header](const auto &elem) {
                        return elem == header.id;
                    })) {
                oldest_migration = i; // note: also ends loop
            }
        }
    };

    read_all_current_bank_items(callback);
    return oldest_migration;
}

bool Backend::generate_migration_intermediaries(std::span<const MigrationFunction> migration_functions) {
    if (migration_functions.size() < 1) {
        return false;
    }

    size_t oldest_migration = find_oldest_migration_index(migration_functions);

    if (oldest_migration < migration_functions.size()) { // we found a migration
        // Need to erase the next bank because the space is needed for storing intermediary transactions
        init_bank(get_next_bank(), current_bank_id - 1, true); // prepare the next bank for intermediaries (mark as older and reset current_next_addr)

        for (size_t i = oldest_migration; i < migration_functions.size(); ++i) {
            auto guard = migrating_transaction_guard(); // always start a migrating transaction, so that data goes into next bank. We can do this since if the function doesn't want to save anything, the transaction destructor does nothing
            migration_functions[i].migration_fn(*this);
        }
    }

    return oldest_migration != migration_functions.size();
}

// Load data, just like normal, except from the next bank
void Backend::load_migrated_data(const UpdateFunction &update_function) {
    // precondition: next bank contains 'migrated' intermediary data

    auto [state, num_of_transactions, end_of_last_transaction] = validate_transactions(get_next_bank_start_address() + BANK_HEADER_SIZE + CRC_SIZE);
    if (state == BankState::Corrupted) {
        bsod("Next bank data is corrupted"); // should never happen, but it's possible migration functions did something 'bad'
    }

    uint16_t next_bank_transactions_start_address = get_next_bank_start_address() + BANK_HEADER_SIZE + CRC_SIZE;
    uint16_t len_of_transactions = current_next_address - next_bank_transactions_start_address;

    load_items(next_bank_transactions_start_address, len_of_transactions, update_function);
}

void Backend::load_all(const UpdateFunction &update_function, std::span<const MigrationFunction> migration_functions) {
    auto l = lock();

    const auto [primary_bank, primary_address, secondary_header, secondary_address] = [this]() {
        const auto res = choose_bank();

        if (res.has_value()) {
            return res.value();
        }

        // no bank found, init first bank
        journal_state = JournalState::ColdStart;
        BankHeader primary_header { .sequence_id = 1, .version = CURRENT_VERSION };
        init_bank(BankSelector::First, primary_header.sequence_id);
        return BanksState { primary_header, get_bank_start_address(BankSelector::First), std::nullopt, std::nullopt };
    }();

    if (journal_state == JournalState::ColdStart) {
        return;
    }

    uint16_t current_bank_address = primary_address;
    current_bank_id = primary_bank.sequence_id;
    uint32_t current_bank_version = primary_bank.version;

    if (current_bank_version != CURRENT_VERSION) {
        // handle different version of bank than current
        // currently exists only one version of bank, will maybe be used in future
    }

    auto [state, num_of_transactions, end_of_last_transaction] = validate_transactions(current_bank_address + BANK_HEADER_SIZE + CRC_SIZE);
    current_address = end_of_last_transaction;

    if (state == BankState::Corrupted) {
        write_end_item(end_of_last_transaction); // "erase" this newer bank
        if (secondary_header.has_value()) {
            // attempt to read from the older bank
            current_bank_id = secondary_header->sequence_id;
            current_bank_id = secondary_header->version;

            current_bank_address = secondary_address.value();
        } else {
            journal_state = JournalState::CorruptedBank;
            return;
        }
        auto tmp = validate_transactions(current_bank_address + BANK_HEADER_SIZE + CRC_SIZE); // cannot do structured binding of a non-tuple to an existing variables

        current_address = tmp.end_of_last_transaction;

        if (tmp.state == BankState::Corrupted) { // older bank also corrupted
            write_end_item(tmp.end_of_last_transaction); // "erase" the older bank as well
            journal_state = JournalState::CorruptedBank;
            return;
        }
        // we can start loading from the older bank to save at least some data
        state = tmp.state;
        num_of_transactions = tmp.num_of_transactions;
        end_of_last_transaction = tmp.end_of_last_transaction;
    }

    if (state == BankState::MissingEndItem) { // intentionally not 'else if'
        write_end_item(current_address); // fix missing end item
        journal_state = JournalState::MissingEndItem;
    } else if (state == BankState::Valid) {
        journal_state = JournalState::ValidStart;
    }

    uint16_t current_bank_transactions_start_address = get_current_bank_start_address() + BANK_HEADER_SIZE + CRC_SIZE;
    uint16_t len_of_transactions = current_address - current_bank_transactions_start_address;

    // migrate from potentially older version and create migration transactions into the next bank
    bool migrated = generate_migration_intermediaries(migration_functions);
    load_items(current_bank_transactions_start_address, len_of_transactions, update_function);

    if (migrated) {
        load_migrated_data(update_function);
    }
    // load extra transactions that were a result of migration functions from the next bank

    if (migrated || num_of_transactions > 1) {
        migrate_bank();
    }
}
std::optional<Backend::BanksState> Backend::choose_bank() const {
    std::array<uint8_t, BANK_HEADER_SIZE + CRC_SIZE> bank_header_buffer {};

    storage.read_bytes(start_address, bank_header_buffer);
    auto bank1_header = validate_bank_header(bank_header_buffer);

    storage.read_bytes(start_address + bank_size, bank_header_buffer);
    auto bank2_header = validate_bank_header(bank_header_buffer);

    if (bank1_header.has_value() && bank2_header.has_value()) {
        bool bank1_is_newer = (bank1_header->sequence_id - bank2_header->sequence_id) < std::numeric_limits<uint32_t>::max() / 2;

        if (bank1_is_newer) {
            return BanksState { bank1_header.value(), start_address, bank2_header, start_address + bank_size };
        } else {
            uint16_t bank_address = start_address + bank_size;
            return BanksState { bank2_header.value(), bank_address, bank1_header, start_address };
        }

    } else if (bank1_header.has_value()) {
        return BanksState { bank1_header.value(), start_address, std::nullopt, std::nullopt };
    } else if (bank2_header.has_value()) {
        uint16_t address = start_address + bank_size;
        return BanksState { bank2_header.value(), address, std::nullopt, std::nullopt };
    } else {
        return std::nullopt;
    }
}

Backend::MultipleTransactionValidationResult Backend::validate_transactions(const Address address) {
    uint16_t num_of_transactions = 0;
    TransactionValidationResult prev_result = { 0, 0, 0 };

    uint16_t free_space = bank_size - BANK_HEADER_SIZE;
    uint16_t pos = address;

    auto val_res = get_next_transaction(pos, free_space);
    while (val_res.has_value()) {
        num_of_transactions++;
        prev_result = val_res.value();

        pos += val_res->transaction_len;
        free_space -= val_res->transaction_len;

        val_res = get_next_transaction(pos, free_space);
    }

    if (num_of_transactions == 0) {
        // no valid transaction found -> bank is corrupted, we should at least have ending item
        return MultipleTransactionValidationResult { .state = BankState::Corrupted, .num_of_transactions = num_of_transactions, .end_of_last_transaction = pos };
    }

    if (prev_result.num_of_items == 1) {
        std::array<uint8_t, MAX_ITEM_SIZE> buffer {};

        // check that transaction has end item
        auto item = load_item(prev_result.address, prev_result.transaction_len, buffer);
        if (!item.has_value()) {
            // should not happen, we have already validated the transaction
            bsod("This should not happen");
        }

        auto [item_header, item_data] = item.value();

        if (item_header.id == LAST_ITEM_STOP.id) {
            // if we have only one transaction and that transaction contains only the end item then this bank is valid
            // we want to overwrite the ending item
            return MultipleTransactionValidationResult { .state = BankState::Valid, .num_of_transactions = static_cast<uint16_t>(num_of_transactions - 1), .end_of_last_transaction = static_cast<uint16_t>(pos - item_data.size() - ITEM_HEADER_SIZE - CRC_SIZE) };
        } else {
            return MultipleTransactionValidationResult { .state = BankState::MissingEndItem, .num_of_transactions = num_of_transactions, .end_of_last_transaction = pos };
        }

    } else {
        // we have some transaction, but not end item -> report missing item
        return MultipleTransactionValidationResult { .state = BankState::MissingEndItem, .num_of_transactions = num_of_transactions, .end_of_last_transaction = pos };
    }
}

std::optional<Backend::BankHeader> Backend::validate_bank_header(const std::span<uint8_t> &data) {
    BankHeader header { 0, 0 };
    memcpy(&header, data.data(), BANK_HEADER_SIZE);
    auto crc_read = get_crc(data.subspan(BANK_HEADER_SIZE));
    if (!crc_read.has_value()) {
        return std::nullopt;
    }
    CRCType crc_computed = crc32_calc(reinterpret_cast<const uint8_t *>(&header), BANK_HEADER_SIZE);
    if (crc_read != crc_computed) {
        return std::nullopt;
    }
    return header;
}

void Backend::init_bank(const Backend::BankSelector selector, Backend::BankSequenceId id, bool is_next_bank) {
    Address address = get_bank_start_address(selector);
    BankHeader header { .sequence_id = id, .version = CURRENT_VERSION };
    CRCType crc = crc32_calc(reinterpret_cast<const uint8_t *>(&header), BANK_HEADER_SIZE);
    storage.write_bytes(address + BANK_HEADER_SIZE, { reinterpret_cast<uint8_t *>(&crc), CRC_SIZE });
    storage.write_bytes(address, { reinterpret_cast<uint8_t *>(&header), BANK_HEADER_SIZE });
    write_end_item(address + BANK_HEADER_SIZE + CRC_SIZE);

    if (is_next_bank) {
        current_next_address = address + BANK_HEADER_SIZE + CRC_SIZE;
    } else {
        current_address = address + BANK_HEADER_SIZE + CRC_SIZE;
        current_bank_id = header.sequence_id;
    }
}

auto Backend::get_journal_state() const -> JournalState {
    return journal_state;
}

void Backend::override_cold_start_state() {
    if (journal_state == JournalState::ColdStart) {
        journal_state = JournalState::ValidStart;
    }
}

void Backend::init(const DumpCallback &callback) {
    auto res = choose_bank();
    if (!res.has_value()) {
        init_bank(BankSelector::First, 1);
        journal_state = JournalState::ColdStart;
    }

    dump_callback = callback;
}

void Backend::load_items(uint16_t address, uint16_t len_of_transactions, const UpdateFunction &update_function) {
    read_all_items(address, len_of_transactions, [&update_function](ItemHeader header, std::array<uint8_t, MAX_ITEM_SIZE> &buffer) {
        update_function(header.id, { buffer.data(), header.len });
    });
}
uint16_t Backend::write_end_item(uint16_t address) {
    if (!fits_in_current_bank(ITEM_HEADER_SIZE + CRC_SIZE)) {
        migrate_bank();
    }

    CRCType crc = crc32_calc_ex(0, reinterpret_cast<const uint8_t *>(&Backend::LAST_ITEM_STOP), sizeof(Backend::LAST_ITEM_STOP));
    return write_item(address, LAST_ITEM_STOP, {}, crc);
}
void Backend::store_single_item(uint16_t id, const std::span<uint8_t> &data) {
    if (!fits_in_current_bank(ITEM_HEADER_SIZE + data.size() + CRC_SIZE + ITEM_HEADER_SIZE + CRC_SIZE)) {
        migrate_bank();
        return;
    }

    ItemHeader header { .last_item = true, .id = id, .len = static_cast<uint16_t>(data.size()) };

    CRCType crc = calculate_crc(header, data);

    current_address += write_item(current_address, header, data, crc);
    write_end_item(current_address);
}
Backend::Address Backend::get_next_bank_start_address() const {
    if (current_address > start_address && current_address < start_address + bank_size) {
        return start_address + bank_size;
    } else {
        return start_address;
    }
}
void Backend::migrate_bank() {
    current_bank_id++;
    init_bank(get_next_bank(), current_bank_id);

    {
        auto guard = migration_guard();
        dump_callback();
    }
}
void Backend::transaction_start() {
    if (transaction.has_value()) {
        bsod("Starting transaction while transaction is running");
    }
    transaction.emplace(Transaction::Type::transaction, *this);
}

void Backend::transaction_end() {
    if (!transaction.has_value()) {
        bsod("Transaction is not in progress");
    }
    transaction.reset();
}

auto Backend::transaction_guard() -> TransactionGuard {
    return TransactionGuard(*this);
}

void Backend::migrating_transaction_start() {
    if (transaction.has_value()) {
        bsod("Starting transaction while transaction is running");
    }
    transaction.emplace(Transaction::Type::migrating_transaction, *this);
}

void Backend::migrating_transaction_end() {
    transaction_end();
}

auto Backend::migrating_transaction_guard() -> MigratingTransactionGuard {
    return MigratingTransactionGuard(*this);
}

void Backend::erase_storage_area() {
    storage.erase_area(start_address, start_address + bank_size * 2);
}

bool Backend::fits_in_current_bank(uint16_t size) const {
    return get_free_space_in_current_bank() > size;
}
uint16_t Backend::get_free_space_in_current_bank() const {
    uint16_t used_space = current_address - get_current_bank_start_address();
    return bank_size - used_space;
}
uint16_t Backend::get_current_bank_start_address() const {
    if (current_address > start_address && current_address < start_address + bank_size) {
        return start_address;
    } else {
        return start_address + bank_size;
    }
}

uint16_t Backend::write_item(const uint16_t address, const Backend::ItemHeader &header, const std::span<uint8_t> &data, std::optional<CRCType> crc) {
    const uint16_t data_address = address + ITEM_HEADER_SIZE;
    uint16_t written = 0;

    if (crc.has_value()) {
        const uint16_t crc_address = address + ITEM_HEADER_SIZE + data.size();
        storage.write_bytes(crc_address, { reinterpret_cast<uint8_t *>(&crc.value()), CRC_SIZE });
        written += CRC_SIZE;
    }
    storage.write_bytes(data_address, data);
    written += data.size();
    storage.write_bytes(address, { reinterpret_cast<const uint8_t *>(&header), ITEM_HEADER_SIZE });
    return written + ITEM_HEADER_SIZE;
}

Backend::CRCType Backend::calculate_crc(const Backend::ItemHeader &header, const std::span<uint8_t> &data, CRCType crc) {
    crc = crc32_calc_ex(crc, reinterpret_cast<const uint8_t *>(&header), ITEM_HEADER_SIZE);
    crc = crc32_calc_ex(crc, data.data(), data.size());
    return crc;
}
void Backend::save(uint16_t id, std::span<uint8_t> data) {
    if (migration.has_value()) {
        migration->store_item(id, data);
    } else if (transaction.has_value()) {
        transaction->store_item(id, data);
    } else {
        store_single_item(id, data);
    }
}
Backend::Backend(uint16_t offset, uint16_t size, configuration_store::Storage &storage)
    : start_address(offset)
    , bank_size(size / 2)
    , storage(storage) {
    assert(bank_size > BANK_HEADER_SIZE + CRC_SIZE + ITEM_HEADER_SIZE + CRC_SIZE);
}
Backend::BankSelector Backend::get_next_bank() {
    if (current_address > start_address && current_address < start_address + bank_size) {
        return BankSelector::Second;
    } else {
        return BankSelector::First;
    }
}
Backend::Address Backend::get_bank_start_address(const Backend::BankSelector selector) {
    return selector == BankSelector::First ? start_address : start_address + bank_size;
}
void Backend::migration_start() {
    migration.emplace(Transaction::Type::migration, *this);
}
void Backend::migration_end() {
    if (!migration.has_value()) {
        bsod("Migration is not started");
    }
    if (transaction.has_value()) {
        transaction->cancel();
    }
    migration.reset();
}

auto Backend::migration_guard() -> MigrationGuard {
    return MigrationGuard(*this);
}

Backend::Transaction::Transaction(Transaction::Type type, Backend &backend)
    : backend(backend)
    , type(type) {
}

Backend::Transaction::~Transaction() {
    if (type == Type::transaction && !backend.fits_in_current_bank(CRC_SIZE + ITEM_HEADER_SIZE + CRC_SIZE)) {
        backend.migrate_bank();
        return;
    }

    if (item_count == 0) {
        return;
    }

    auto &current_address = type == Type::migrating_transaction ? backend.current_next_address : backend.current_address;

    backend.storage.write_bytes(current_address, { reinterpret_cast<uint8_t *>(&last_item_crc), CRC_SIZE });
    last_item_header.last_item = true;
    backend.storage.write_bytes(last_item_address, { reinterpret_cast<uint8_t *>(&last_item_header), ITEM_HEADER_SIZE });
    current_address += CRC_SIZE;

    backend.write_end_item(current_address);
}

void Backend::Transaction::calculate_crc(Backend::Id id, const std::span<uint8_t> &data) {
    ItemHeader header { .last_item = false, .id = id, .len = static_cast<uint16_t>(data.size()) };
    ItemHeader last_header { .last_item = true, .id = id, .len = static_cast<uint16_t>(data.size()) };

    last_item_crc = Backend::calculate_crc(last_header, data, crc);
    crc = Backend::calculate_crc(header, data, crc);
}

void Backend::Transaction::store_item(Backend::Id id, const std::span<uint8_t> &data) {
    if (type == Type::transaction && !backend.fits_in_current_bank(ITEM_HEADER_SIZE + data.size())) {
        backend.migrate_bank();
        return;
    }

    calculate_crc(id, data);
    item_count++;

    auto &current_address = type == Type::migrating_transaction ? backend.current_next_address : backend.current_address;

    ItemHeader header { .last_item = false, .id = id, .len = static_cast<uint16_t>(data.size()) };
    last_item_header = header;
    last_item_address = current_address;

    current_address += backend.write_item(current_address, header, data, std::nullopt);
}

void Backend::Transaction::cancel() {
    Backend &_backend = backend;
    Type _type = type;
    // using placement new, because we want to get default values without calling destructor
    new (this) Backend::Transaction(_type, _backend);
}
} // namespace journal
