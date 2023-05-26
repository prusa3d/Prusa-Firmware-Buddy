#include "backend.hpp"
#include "timing.h"
#include "assert.h"
#include <memory>
#include <utility>

namespace Journal {
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
Backend::LoadAction Backend::handle_valid(const UpdateFunction &update_function, uint16_t num_of_transactions) {
    // load data to configuration store and then dump them to other bank
    journal_state = JournalState::ValidStart;
    uint16_t address = get_current_bank_start_address() + BANK_HEADER_SIZE + CRC_SIZE;
    uint16_t size = current_address - address;
    bool migrated = load_items(address, size, update_function);
    if (num_of_transactions > 1 || migrated) {
        return LoadAction::MigrateBank;
    }
    return LoadAction::Nothing;
}
void Backend::load_all(const UpdateFunction &update_function) {
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

    while (true) {
        auto const [state, num_of_transactions, end_of_last_transaction] = validate_transactions(current_bank_address + BANK_HEADER_SIZE + CRC_SIZE);

        current_address = end_of_last_transaction;
        LoadAction action = LoadAction::Nothing;

        switch (state) {
        case BankState::Valid:
            action = handle_valid(update_function, num_of_transactions);
            break;
        case BankState::MissingEndItem:
            action = handle_missing_end_item(update_function, num_of_transactions);
            journal_state = JournalState::MissingEndItem;
            break;
        case BankState::Corrupted:
            action = LoadAction::ChangeBank;
            journal_state = JournalState::CorruptedBank;
            break;
        }

        switch (action) {
        case LoadAction::Nothing:
            return;
        case LoadAction::ChangeBank:
            write_end_item(current_address);
            if (secondary_header.has_value()) {
                current_bank_id = secondary_header->sequence_id;
                current_bank_id = secondary_header->version;

                current_bank_address = secondary_address.value();
                continue;
            } else {
                return;
            }
        case LoadAction::MigrateBank:
            migrate_bank();
            return;
        case LoadAction::FixEndItem:
            write_end_item(current_address);
            return;
        case LoadAction::FixEndItemAndMigrate:
            write_end_item(current_address);
            migrate_bank();
            return;
        }
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

void Backend::init_bank(const Backend::BankSelector selector, Backend::BankSequenceId id) {
    Address address = get_bank_start_address(selector);
    BankHeader header { .sequence_id = id, .version = CURRENT_VERSION };
    CRCType crc = crc32_calc(reinterpret_cast<const uint8_t *>(&header), BANK_HEADER_SIZE);
    storage.write_bytes(address + BANK_HEADER_SIZE, { reinterpret_cast<uint8_t *>(&crc), CRC_SIZE });
    storage.write_bytes(address, { reinterpret_cast<uint8_t *>(&header), BANK_HEADER_SIZE });
    write_end_item(address + BANK_HEADER_SIZE + CRC_SIZE);

    current_address = address + BANK_HEADER_SIZE + CRC_SIZE;
    current_bank_id = header.sequence_id;
}

void Backend::init(const DumpCallback &callback) {
    auto res = choose_bank();
    if (!res.has_value()) {
        init_bank(BankSelector::First, 1);
        journal_state = JournalState::ColdStart;
    }

    dump_callback = std::move(callback);
}
bool Backend::load_items(uint16_t address, uint16_t len_of_transactions, const UpdateFunction &update_function) {
    bool last_item = false;
    bool migrated = false;

    const auto callback = [&update_function, &last_item, &migrated](ItemHeader header, std::array<uint8_t, MAX_ITEM_SIZE> &buffer) -> void {
        if (header.id == LAST_ITEM_STOP.id) {
            last_item = true;
            return;
        }
        if (update_function(header.id, buffer, header.len)) {
            migrated = true;
        }
    };
    while (!last_item && len_of_transactions > 0) {
        auto res = map_over_transaction_unchecked(address, len_of_transactions, callback);
        if (!res.has_value()) {
            // should not happen, already checked validity
            bsod("Error while loading items");
        }
        address += res.value();
        len_of_transactions -= res.value();
    };
    return migrated;
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

    migration_start();
    dump_callback();
    migration_end();
}
void Backend::transaction_start() {
    if (transaction.has_value()) {
        bsod("Starting transaction while transaction is running");
    }
    transaction.emplace(true, *this);
}

void Backend::transaction_end() {
    if (!transaction.has_value()) {
        bsod("Transaction is not in progress");
    }
    transaction.reset();
}
bool Backend::fits_in_current_bank(uint16_t size) const {
    return get_free_space_in_current_bank() > size;
}
uint16_t Backend::get_free_space_in_current_bank() const {
    uint16_t used_space = current_address - get_current_bank_start_address();
    return bank_size - used_space;
}
Backend::LoadAction Backend::handle_missing_end_item(const UpdateFunction &update_function, uint16_t num_of_transactions) {
    // we have initial data -> we have lost new data, which are not preset in the other bank -> write end item and continue normally
    Address address = get_current_bank_start_address() + BANK_HEADER_SIZE + CRC_SIZE;
    uint16_t transaction_len = current_address - address;
    // migrate to other bank if items are migrated
    bool migrated = load_items(address, transaction_len, update_function);
    if (migrated || num_of_transactions > 1) {
        return LoadAction::FixEndItemAndMigrate;
    }
    return LoadAction::FixEndItem;
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
Backend::Backend(uint16_t offset, uint16_t size, Storage &storage)
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
    migration.emplace(false, *this);
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

Backend::Transaction::Transaction(bool check_size, Backend &backend)
    : backend(backend)
    , check_size(check_size) {
}

Backend::Transaction::~Transaction() {
    if (check_size && !backend.fits_in_current_bank(CRC_SIZE + ITEM_HEADER_SIZE + CRC_SIZE)) {
        backend.migrate_bank();
        return;
    }

    if (item_count == 0) {
        return;
    }

    backend.storage.write_bytes(backend.current_address, { reinterpret_cast<uint8_t *>(&last_item_crc), CRC_SIZE });
    last_item_header.last_item = true;
    backend.storage.write_bytes(last_item_address, { reinterpret_cast<uint8_t *>(&last_item_header), ITEM_HEADER_SIZE });
    backend.current_address += CRC_SIZE;

    backend.write_end_item(backend.current_address);
}

void Backend::Transaction::calculate_crc(Backend::Id id, const std::span<uint8_t> &data) {
    ItemHeader header { .last_item = false, .id = id, .len = static_cast<uint16_t>(data.size()) };
    ItemHeader last_header { .last_item = true, .id = id, .len = static_cast<uint16_t>(data.size()) };

    last_item_crc = Backend::calculate_crc(last_header, data, crc);
    crc = Backend::calculate_crc(header, data, crc);
}

void Backend::Transaction::store_item(Backend::Id id, const std::span<uint8_t> &data) {
    if (check_size && !backend.fits_in_current_bank(ITEM_HEADER_SIZE + data.size())) {
        backend.migrate_bank();
        return;
    }

    calculate_crc(id, data);
    item_count++;

    ItemHeader header { .last_item = false, .id = id, .len = static_cast<uint16_t>(data.size()) };
    last_item_header = header;
    last_item_address = backend.current_address;

    backend.current_address += backend.write_item(backend.current_address, header, data, std::nullopt);
}

void Backend::Transaction::cancel() {
    Backend &_backend = backend;
    bool _check_size = check_size;
    // using placement new, because we want to get default values without calling destructor
    new (this) Backend::Transaction(_check_size, _backend);
}
}
