#include "backend.hpp"
#include <crc32.hpp>
#include <algorithm>
#include <bsod/bsod.h>

#ifndef UNITTESTS
    #include <metric.h>

/// Emitted for each item write (outside of migrations), value is the hash ID of the written item
METRIC_DEF(metric_store_id, "store_id", METRIC_VALUE_INTEGER, 0, METRIC_ENABLED);

/// Emitted for ecah migration, value is the number of bytes written due to the migration
METRIC_DEF(metric_store_migration, "store_migration", METRIC_VALUE_INTEGER, 0, METRIC_ENABLED);

/// Emitted for each migration, value is the duration of the migration in ms
METRIC_DEF(metric_store_migration_ms, "store_migration_ms", METRIC_VALUE_INTEGER, 0, METRIC_ENABLED);
#endif

namespace journal {
std::unique_lock<freertos::Mutex> Backend::lock() {
    return std::unique_lock<freertos::Mutex>(mutex);
}
std::optional<uint16_t> Backend::map_over_transaction(Backend::Address address, Backend::Offset free_space, CallbackFunction fnc) {
    CRCType crc_comp = 0;

    auto transaction_len = map_over_transaction_unchecked(address, free_space,
        [&crc_comp, &fnc](ItemHeader header, std::array<std::byte, MAX_ITEM_SIZE> &buffer) {
            crc_comp = crc32(crc_comp, trivial_as_bytes(header));
            crc_comp = crc32(crc_comp, { buffer.data(), header.len });

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
    Address start_address = get_current_bank_start_address() + BANK_HEADER_SIZE_WITH_CRC;
    read_all_items(start_address, current_address - start_address, callback);
}

void Backend::read_items_for_migrations(const CallbackFunction &callback) {
    // precondition: current_next_address is set and next bank has valid data (nothing or migrated intermediary transactions)
    read_all_current_bank_items(callback);

    Address start_address = get_next_bank_start_address() + BANK_HEADER_SIZE_WITH_CRC;
    read_all_items(start_address, current_next_address - start_address, callback);
}

void Backend::read_all_items(Address address, Offset len_of_transactions, const CallbackFunction &fnc) {
    bool last_item = false;

    len_of_transactions = std::min(len_of_transactions, static_cast<Offset>(bank_size - BANK_HEADER_SIZE_WITH_CRC));

    auto last_item_clb_wrapper = [&last_item, &fnc](journal::Backend::ItemHeader header, std::array<std::byte, journal::Backend::MAX_ITEM_SIZE> &buffer) -> void {
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
    std::array<std::byte, MAX_ITEM_SIZE> buffer {};

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

    auto fnc = [&num_of_items]([[maybe_unused]] ItemHeader header, [[maybe_unused]] std::array<std::byte, MAX_ITEM_SIZE> &buffer) {
        num_of_items++;
    };

    auto transaction_len = map_over_transaction(address, free_space, fnc);
    if (!transaction_len.has_value()) {
        return std::nullopt;
    }

    return TransactionValidationResult { .address = address, .transaction_len = transaction_len.value(), .num_of_items = num_of_items };
}

std::optional<Backend::ItemLoadResult> Backend::load_item(uint16_t address, uint16_t free_space, const WritableBytes &buffer) {
    if (free_space < ITEM_HEADER_SIZE) {
        return std::nullopt;
    }
    ItemHeader header = { false, 0, 0 };
    storage.read_bytes(address, trivial_as_writable_bytes(header));

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
    storage.read_bytes(address, trivial_as_writable_bytes(crc));
    return crc;
}

std::optional<Backend::CRCType> Backend::get_crc(Bytes data) {
    if (data.size() < CRC_SIZE) {
        return std::nullopt;
    }
    CRCType crc;
    memcpy(&crc, data.data(), CRC_SIZE);
    return crc;
}
size_t Backend::find_oldest_version_migration_index(std::span<const MigrationFunction> migration_functions) {
    size_t oldest_migration = migration_functions.size();

    auto callback = [&migration_functions, &oldest_migration](ItemHeader header, [[maybe_unused]] std::array<std::byte, MAX_ITEM_SIZE> &buffer) -> void {
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

bool Backend::generate_version_migration_intermediaries(std::span<const MigrationFunction> migration_functions) {
    if (migration_functions.size() < 1) {
        return false;
    }

    size_t oldest_migration = find_oldest_version_migration_index(migration_functions);

    if (oldest_migration < migration_functions.size()) { // we found a migration
        // Need to erase the next bank because the space is needed for storing intermediary transactions
        init_bank(get_next_bank(), current_bank_id - 1, true); // prepare the next bank for intermediaries (mark as older and reset current_next_addr)

        for (size_t i = oldest_migration; i < migration_functions.size(); ++i) {
            auto guard = version_migration_guard(); // always start a migrating transaction, so that data goes into next bank. We can do this since if the function doesn't want to save anything, the transaction destructor does nothing
            migration_functions[i].migration_fn(*this);
        }
    }

    return oldest_migration != migration_functions.size();
}

// Load data, just like normal, except from the next bank
void Backend::load_version_migrated_data(const UpdateFunction &update_function) {
    // precondition: next bank contains 'migrated' intermediary data

    auto [state, num_of_transactions, end_of_last_transaction] = validate_transactions(get_next_bank_start_address() + BANK_HEADER_SIZE_WITH_CRC);
    if (state == BankState::Corrupted) {
        bsod("Next bank data is corrupted"); // should never happen, but it's possible migration functions did something 'bad'
    }

    uint16_t next_bank_transactions_start_address = get_next_bank_start_address() + BANK_HEADER_SIZE_WITH_CRC;
    uint16_t len_of_transactions = current_next_address - next_bank_transactions_start_address;

    load_items(next_bank_transactions_start_address, len_of_transactions, update_function);
}

void Backend::load_all(const UpdateFunction &update_function, const std::span<const MigrationFunction> &migration_functions) {
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

    auto [state, num_of_transactions, end_of_last_transaction] = validate_transactions(current_bank_address + BANK_HEADER_SIZE_WITH_CRC);
    current_address = end_of_last_transaction;

    if (state == BankState::Corrupted) {
        write_end_item(end_of_last_transaction); // "erase" this newer bank
        if (secondary_header.has_value()) {
            // attempt to read from the older bank
            current_bank_id = secondary_header->sequence_id;
            current_bank_version = secondary_header->version;
            current_bank_address = secondary_address.value();
        } else {
            journal_state = JournalState::CorruptedBank;
            return;
        }
        auto tmp = validate_transactions(current_bank_address + BANK_HEADER_SIZE_WITH_CRC); // cannot do structured binding of a non-tuple to an existing variables

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

    uint16_t current_bank_transactions_start_address = get_current_bank_start_address() + BANK_HEADER_SIZE_WITH_CRC;
    uint16_t len_of_transactions = current_address - current_bank_transactions_start_address;

    // migrate from potentially older version and create migration transactions into the next bank
    bool migrated = generate_version_migration_intermediaries(migration_functions);
    load_items(current_bank_transactions_start_address, len_of_transactions, update_function);

    if (migrated) {
        load_version_migrated_data(update_function);
    }
    // load extra transactions that were a result of migration functions from the next bank

    if (migrated || num_of_transactions > 1) {
        migrate_bank();
    }
}
std::optional<Backend::BanksState> Backend::choose_bank() const {
    std::array<std::byte, BANK_HEADER_SIZE_WITH_CRC> bank_header_buffer {};

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
    TransactionValidationResult last_transaction = { 0, 0, 0 };

    uint16_t free_space = bank_size - BANK_HEADER_SIZE;
    uint16_t pos = address;

    while (auto val_res = get_next_transaction(pos, free_space)) {
        last_transaction = val_res.value();
        num_of_transactions++;
        pos += val_res->transaction_len;
        free_space -= val_res->transaction_len;
    }

    if (num_of_transactions == 0) {
        // no valid transaction found -> bank is corrupted, we should at least have ending item
        return MultipleTransactionValidationResult { .state = BankState::Corrupted, .num_of_transactions = num_of_transactions, .end_of_last_transaction = pos };
    }

    if (last_transaction.num_of_items == 1) {
        std::array<std::byte, MAX_ITEM_SIZE> buffer {};

        // check that transaction has end item
        auto item = load_item(last_transaction.address, last_transaction.transaction_len, buffer);
        if (!item.has_value()) {
            // should not happen, we have already validated the transaction
            bsod("This should not happen");
        }

        auto [item_header, item_data] = item.value();

        if (item_header.id == LAST_ITEM_STOP.id) {
            // if we have only one transaction and that transaction contains only the end item then this bank is valid
            // we want to overwrite the ending item
            return MultipleTransactionValidationResult { .state = BankState::Valid, .num_of_transactions = static_cast<uint16_t>(num_of_transactions - 1), .end_of_last_transaction = static_cast<uint16_t>(pos - item_data.size() - END_ITEM_SIZE_WITH_CRC) };
        } else {
            return MultipleTransactionValidationResult { .state = BankState::MissingEndItem, .num_of_transactions = num_of_transactions, .end_of_last_transaction = pos };
        }

    } else {
        // we have some transaction, but not end item -> report missing item
        return MultipleTransactionValidationResult { .state = BankState::MissingEndItem, .num_of_transactions = num_of_transactions, .end_of_last_transaction = pos };
    }
}

std::optional<Backend::BankHeader> Backend::validate_bank_header(const Bytes &data) {
    BankHeader header { 0, 0 };
    memcpy(&header, data.data(), BANK_HEADER_SIZE);
    auto crc_read = get_crc(data.subspan(BANK_HEADER_SIZE));
    if (!crc_read.has_value()) {
        return std::nullopt;
    }
    CRCType crc_computed = crc32(0, trivial_as_bytes(header));
    if (crc_read != crc_computed) {
        return std::nullopt;
    }
    return header;
}

void Backend::prepare_bank(const Backend::BankSelector selector, Backend::BankSequenceId id, bool is_next_bank) {
    const Address address = get_bank_start_address(selector);

    if (is_next_bank) {
        current_next_address = address + BANK_HEADER_SIZE_WITH_CRC;
    } else {
        current_address = address + BANK_HEADER_SIZE_WITH_CRC;
        current_bank_id = id;
    }

    write_end_item(address + BANK_HEADER_SIZE_WITH_CRC);
}

void Backend::write_bank_header(const Backend::BankSelector selector, Backend::BankSequenceId id) {
    const Address address = get_bank_start_address(selector);
    const BankHeader header { .sequence_id = id, .version = CURRENT_VERSION };
    const CRCType crc = crc32(0, trivial_as_bytes(header));
    storage.write_bytes(address + BANK_HEADER_SIZE, trivial_as_bytes(crc));
    storage.write_bytes(address, trivial_as_bytes(header));
    storage.flush();
}

void Backend::init_bank(const Backend::BankSelector selector, Backend::BankSequenceId id, bool is_next_bank) {
    prepare_bank(selector, id, is_next_bank);
    write_bank_header(selector, id);
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
    read_all_items(address, len_of_transactions, [&update_function](ItemHeader header, std::array<std::byte, MAX_ITEM_SIZE> &buffer) {
        update_function(header.id, { buffer.data(), header.len });
    });
}
uint16_t Backend::write_end_item(uint16_t address) {
    if (get_free_space_in_bank(address) < END_ITEM_SIZE_WITH_CRC) {
        return 0;
    }

    CRCType crc = crc32(0, trivial_as_bytes(LAST_ITEM_STOP));
    const uint16_t written = write_item(address, LAST_ITEM_STOP, {}, crc);
    storage.flush();
    return written;
}
void Backend::store_single_item(uint16_t id, const Bytes &data) {
    if (!fits_in_current_bank(ITEM_HEADER_SIZE + data.size() + CRC_SIZE + END_ITEM_SIZE_WITH_CRC)) {
        migrate_bank();
        return;
    }

    ItemHeader header { .last_item = true, .id = id, .len = static_cast<uint16_t>(data.size()) };

    const CRCType crc = calculate_crc(header, data);

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
#ifndef UNITTESTS
    const auto start_bytes_written = storage.bytes_written();
    const auto start_ms = ticks_ms();
#endif

    current_bank_id++;
    bank_migration_count_.fetch_add(1, std::memory_order_relaxed);

    const BankSelector target = get_next_bank();
    prepare_bank(target, current_bank_id);

    {
        auto guard = bank_migration_guard();
        dump_callback();
    }

    // bank is not valid until this point
    write_bank_header(target, current_bank_id);

#ifndef UNITTESTS
    metric_record_integer(&metric_store_migration, storage.bytes_written() - start_bytes_written);
    metric_record_integer(&metric_store_migration_ms, ticks_diff(ticks_ms(), start_ms));
#endif
}
void Backend::transaction_start() {
    auto lock_guard = lock();
    if (transaction.has_value()) {
        if (transaction->type == Transaction::Type::transaction) {
            transaction->ref_count++;
        } else {
            bsod("Starting transaction while other-type transaction is running");
        }
    } else {
        transaction.emplace(Transaction::Type::transaction, *this);
    }
}

void Backend::transaction_end() {
    auto lock_guard = lock();
    if (!transaction.has_value() || transaction->type != Transaction::Type::transaction) {
        bsod("This transaction is not in progress");
    }
    debug_assert(transaction->ref_count > 0);
    transaction->ref_count--;
    if (transaction->ref_count == 0) {
        transaction.reset();
    }
}

auto Backend::transaction_guard() -> TransactionGuard {
    return TransactionGuard(*this);
}

void Backend::trigger_bank_migration() {
    auto lock_guard = lock();
    migrate_bank();
}

void Backend::version_migration_start() {
    if (transaction.has_value()) {
        bsod("Starting transaction while transaction is running");
    }
    transaction.emplace(Transaction::Type::version_migration, *this);
}

void Backend::version_migration_end() {
    if (!transaction.has_value()) {
        bsod("Transaction is not in progress");
    }
    transaction.reset();
}

auto Backend::version_migration_guard() -> VersionMigratingTransactionGuard {
    return VersionMigratingTransactionGuard(*this);
}

bool Backend::fits_in_current_bank(uint16_t size) const {
    return get_free_space_in_current_bank() >= size;
}
uint16_t Backend::get_free_space_in_bank(Address address_in_bank) const {
    uint16_t used_space = current_address - get_bank_start_address(address_in_bank);
    return bank_size - used_space - 1; // 1 to prevent current_address going into next bank when you fit the item size just right
}
uint16_t Backend::get_bank_start_address(Address address_in_bank) const {
    return start_address + (address_in_bank < start_address + bank_size ? 0 : bank_size);
}

uint16_t Backend::write_item(Address address, Backend::ItemHeader header, const Bytes &data, std::optional<CRCType> crc) {
    const uint16_t data_address = address + ITEM_HEADER_SIZE;
    uint16_t written = 0;

    if (crc.has_value()) {
        const uint16_t crc_address = address + ITEM_HEADER_SIZE + data.size();
        storage.write_bytes(crc_address, trivial_as_bytes(crc.value()));
        written += CRC_SIZE;
    }
    storage.write_bytes(data_address, data);
    written += data.size();
    storage.write_bytes(address, trivial_as_bytes(header));

    if (!bank_migration.has_value() && header.id != LAST_ITEM_ID) {
#ifndef UNITTESTS
        metric_record_integer(&metric_store_id, header.id);
#endif

        item_write_count_.fetch_add(1, std::memory_order_relaxed);
    }

    return written + ITEM_HEADER_SIZE;
}

Backend::CRCType Backend::calculate_crc(const Backend::ItemHeader &header, const Bytes &data, CRCType crc) {
    crc = crc32(crc, trivial_as_bytes(header));
    crc = crc32(crc, data);
    return crc;
}
void Backend::save(uint16_t id, const Bytes &data) {
    if (bank_migration) {
        bank_migration->calculate_crc(id, data);
        bank_migration->item_count++;
        bank_migration->last_item_header = { .last_item = false, .id = id, .len = static_cast<uint16_t>(data.size()) };
        bank_migration->last_item_address = current_address;
        current_address += storage.write_bytes(current_address, trivial_as_bytes(bank_migration->last_item_header));
        current_address += storage.write_bytes(current_address, data);
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
    debug_assert(bank_size > BANK_HEADER_SIZE_WITH_CRC + END_ITEM_SIZE_WITH_CRC);
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
void Backend::bank_migration_start() {
    bank_migration.emplace(Transaction::Type::bank_migration, *this);
}
void Backend::bank_migration_end() {
    if (!bank_migration.has_value()) {
        bsod("Migration is not started");
    }
    if (transaction.has_value()) {
        // Reinitialize the transaction.
        //
        // Doing so "from the outside", because it is questionable if an object
        // can replace _itself_ (it probably can, but that raised a lot of
        // questions).
        Backend &_backend = transaction->backend;
        const Transaction::Type _type = transaction->type;
        const uint16_t _ref_count = transaction->ref_count;
        // Launder probably isn't strictly necessary either, but better safe than sorry.
        Transaction *t = std::launder(&*transaction);
        // using placement new, because we want to get default values without calling destructor
        new (t) Backend::Transaction(_type, _backend);
        t->ref_count = _ref_count;
    }
    bank_migration.reset();
}

auto Backend::bank_migration_guard() -> BankMigrationGuard {
    return BankMigrationGuard(*this);
}

Backend::Transaction::Transaction(Transaction::Type type, Backend &backend)
    : backend(backend)
    , type(type)
    , last_item_address(type == Type::version_migration ? backend.current_next_address : backend.current_address) {
}

Backend::Transaction::~Transaction() {
    if (type == Type::transaction && !backend.fits_in_current_bank(CRC_SIZE + END_ITEM_SIZE_WITH_CRC)) {
        // After bank migration, all items will be stored with new values, we don't have to do anything
        backend.migrate_bank();
        return;
    }

    if (item_count == 0) {
        return;
    }

    auto &current_address = type == Type::version_migration ? backend.current_next_address : backend.current_address;

    // Append CRC
    backend.storage.write_bytes(current_address, trivial_as_bytes(last_item_crc));

    // Overwrite last item header to mark it ast last item
    last_item_header.last_item = true;
    backend.storage.write_bytes(last_item_address, trivial_as_bytes(last_item_header));
    current_address += CRC_SIZE;

    backend.write_end_item(current_address);
}

void Backend::Transaction::calculate_crc(Backend::Id id, const Bytes &data) {
    const auto prev_crc = crc;

    ItemHeader header { .last_item = false, .id = id, .len = static_cast<uint16_t>(data.size()) };
    crc = Backend::calculate_crc(header, data, prev_crc);

    // If this item ends up being the last item, we need to calculate a different CRC for that
    header.last_item = true;
    last_item_crc = Backend::calculate_crc(header, data, prev_crc);
}

void Backend::Transaction::store_item(Backend::Id id, const Bytes &data) {
    if (type == Type::transaction && !backend.fits_in_current_bank(ITEM_HEADER_SIZE + data.size())) {
        backend.migrate_bank();
        return;
    }

    calculate_crc(id, data);
    item_count++;

    auto &current_address = type == Type::version_migration ? backend.current_next_address : backend.current_address;

    ItemHeader header { .last_item = false, .id = id, .len = static_cast<uint16_t>(data.size()) };
    last_item_header = header;
    last_item_address = current_address;

    current_address += backend.write_item(current_address, header, data, std::nullopt);
}
} // namespace journal
