#include "opt_nfcv.hpp"

#include <bitset>
#include <raii/scope_guard.hpp>
#include <bsod/bsod.h>
#include <utils/byte_utils.hpp>

using namespace openprinttag;

namespace {

OPTBackend::IOError to_backend_error(nfcv::Error error) {
    using E = nfcv::Error;
    using R = OPTBackend::IOError;

    switch (error) {

    case E::timeout:
    case E::invalid_chip:
    case E::bad_oscilator:
    case E::buffer_overflow:
    case E::invalid_crc:
    case E::device_hard_framing_error:
    case E::device_soft_framing_error:
    case E::device_parity_error:
    case E::device_crc_error:
    case E::no_response:
    case E::response_invalid_size:
    case E::response_format_invalid:
    case E::response_is_error:
    case E::bad_request:
    case E::other:
    case E::unknown:
        return R::other;

    case E::not_implemented:
        return R::not_implemented;
    }

    std::unreachable();
}

} // namespace

OPTBackend_NFCV::FieldGuard::FieldGuard(OPTBackend_NFCV &backend, ReaderAntenna antenna)
    : result(backend.config_.enable_radio ? backend.reader.field_up(antenna).transform_error(to_backend_error) : std::unexpected(IOError::radio_disabled)) {
}

OPTBackend_NFCV::FieldGuard::~FieldGuard() {
    // Do NOT put the field down.
    // Constant field switching introduces problems and RF noise.
    // Instead, we keep it on and lazily switch it in field_up
    // BFW-8285

    // if (result) {
    //    reader.field_down();
    // }
}

OPTBackend_NFCV::OPTBackend_NFCV(nfcv::ReaderWriterInterface &reader, const Config &initial_config)
    : reader(reader)
    , discoveries_limiter(initial_config.discovery_interval_ms) {
    debug_assert(reader.antenna_count() <= MAX_ANTENNA_COUNT);
    set_config(initial_config);
    reset_state();
}

void openprinttag::OPTBackend_NFCV::set_config(const Config &config) {
    if (!config.enable_radio) {
        // The field might be left up, shut it down
        reader.field_down();
    }

    OPTBackend::set_config(config);
    discoveries_limiter.set_min_delay(config.discovery_interval_ms);
}

OPTBackend::IOResult<void> OPTBackend_NFCV::io_op(TagID tag, PayloadPos start, size_t buffer_size, const stdext::inplace_function<IOOpFunc> &impl) {
    if (!is_valid(tag)) {
        return std::unexpected(IOError::invalid_id);
    }
    auto &tag_data = tags.at(tag);

    if (const auto end = start + buffer_size; end > tag_data.block_count * tag_data.block_size) {
        return std::unexpected(IOError::outside_of_bounds);
    }

    FieldGuard field_guard { *this, tag_data.antenna };
    if (!field_guard) {
        return std::unexpected(field_guard.error());
    }

    if (const auto r = impl(tag_data); !r.has_value()) {
        return std::unexpected(to_backend_error(r.error()));
    }

    return {};
}

OPTBackend::IOResult<void> OPTBackend_NFCV::read(TagID tag, PayloadPos start, const WritableBytes &buffer) {
    // Default lambda capture doesn't fit the stdext::inplace_function so we need a helper to pass the data correctly
    struct {
        PayloadPos start;
        const WritableBytes &buffer;
    } ref = {
        .start = start,
        .buffer = buffer,
    };
    return io_op(tag, start, buffer.size(), [this, &ref](const TagData &tag_data) {
        return read_impl(tag_data, ref.start, ref.buffer);
    });
}

nfcv::Result<void> OPTBackend_NFCV::read_impl(const TagData &tag_data, PayloadPos start, const WritableBytes &buffer) {
    auto block_index = start / tag_data.block_size;
    auto it = buffer.begin();
    int32_t in_block_byte_offset = start % tag_data.block_size;
    std::array<std::byte, nfcv::MAX_BLOCK_SIZE_IN_BYTES> tmp_buf {};
    for (; it != buffer.end(); ++block_index) {
        if (const auto res = reader.read_single_block(tag_data.uid, block_index, std::span { tmp_buf.data(), tag_data.block_size }); !res.has_value()) {
            return res;
        }

        const int32_t remaining_bytes = std::distance(it, buffer.end());
        const auto to_copy = std::min(tag_data.block_size - in_block_byte_offset, remaining_bytes);
        auto tmp_buf_it = tmp_buf.begin();
        it = std::copy_n(std::next(tmp_buf_it, in_block_byte_offset), to_copy, it);
        in_block_byte_offset = 0;
    }

    return {};
}

OPTBackend::IOResult<void> OPTBackend_NFCV::write(TagID tag, PayloadPos start, const Bytes &buffer) {
    // Default lambda capture doesn't fit the stdext::inplace_function so we need a helper to pass the data correctly
    struct {
        PayloadPos start;
        const Bytes &buffer;
    } ref = { .start = start, .buffer = buffer };
    return io_op(tag, start, buffer.size(), [this, &ref](const TagData &tag_data) { return write_impl(tag_data, ref.start, ref.buffer); });
}

nfcv::Result<void> OPTBackend_NFCV::write_impl(const TagData &tag_data, PayloadPos start, const Bytes &buffer) {
    std::array<std::byte, nfcv::MAX_BLOCK_SIZE_IN_BYTES> tmp_buf {};
    const WritableBytes tmp_buf_block_span { tmp_buf.data(), tag_data.block_size };
    auto source_it = buffer.begin();

    auto block_index = start / tag_data.block_size;
    const auto in_block_byte_offset = start % tag_data.block_size;

    // if we are not block alligned we need to read the first block - write some data at the end of it and then write this edited block
    if (in_block_byte_offset != 0) {
        if (const auto res = reader.read_single_block(tag_data.uid, block_index, tmp_buf_block_span); !res.has_value()) {
            return res;
        }

        const auto to_copy = std::min(static_cast<size_t>(tag_data.block_size - in_block_byte_offset), buffer.size());
        std::copy_n(source_it, to_copy, std::next(tmp_buf.begin(), in_block_byte_offset));
        std::advance(source_it, to_copy);

        if (const auto res = reader.write_single_block(tag_data.uid, block_index, tmp_buf_block_span); !res.has_value()) {
            return res;
        }
        ++block_index;
    }

    // let's write the aligned data
    // TODO: use write blocks command
    for (; std::distance(source_it, buffer.end()) >= tag_data.block_size; std::advance(source_it, tag_data.block_size), ++block_index) {
        auto block_write = buffer.subspan(std::distance(buffer.begin(), source_it), tag_data.block_size);

        if (const auto res = reader.write_single_block(tag_data.uid, block_index, block_write); !res.has_value()) {
            return res;
        }
    }

    // again if the data are not block aligned at the end we need to read the block, rewrite the start of it and then write the whole block
    if (source_it != buffer.end()) {
        if (const auto res = reader.read_single_block(tag_data.uid, block_index, tmp_buf_block_span); !res.has_value()) {
            return res;
        }

        std::copy(source_it, buffer.end(), tmp_buf_block_span.begin());

        if (const auto res = reader.write_single_block(tag_data.uid, block_index, tmp_buf_block_span); !res.has_value()) {
            return res;
        }
    }

    return {};
}

bool OPTBackend_NFCV::get_event(Event &e, uint32_t current_time_ms) {
    // check for events to report
    if (events.isEmpty()) {
        if (!discoveries_limiter.check(current_time_ms)) {
            return false;
        }
        // if we don't have any => run discovery to detect new tags
        run_next_discovery();

        // if still don't have any events => return false;
        if (events.isEmpty()) {
            return false;
        }
    }

    debug_assert(!events.isEmpty());

    e = events.dequeue();
    return true;
}

OPTBackend::IOResult<size_t> OPTBackend_NFCV::get_tag_uid(TagID tag, const WritableBytes &buffer) {
    if (!is_valid(tag)) {
        return std::unexpected(IOError::invalid_id);
    }

    const auto &uid = tags.at(tag).uid;
    if (buffer.size() < uid.size()) {
        return std::unexpected(IOError::data_too_big);
    }

    memcpy(buffer.data(), uid.data(), uid.size());
    return uid.size();
}

OPTBackend::IOResult<void> OPTBackend_NFCV::read_tag_info(TagID tag, TagInfo &target) {
    if (!is_valid(tag)) {
        return std::unexpected(IOError::invalid_id);
    }

    // TODO: Add support for CC8
    nfcv::CapabilityContainer4 cc;
    if (auto r = read(tag, 0, std::as_writable_bytes(std::span { &cc, 1 })); !r) {
        return std::unexpected(r.error());
    }

    if (cc.magic_number != cc.expected_magic_number) {
        return std::unexpected(IOError::tag_invalid);
    }

    target = {
        // TLV starts right after the CC and ends with the tag
        .tlv_span = PayloadSpan::from_offset_end(sizeof(cc), cc.memory_length_8 * 8),
    };

    return {};
}

void OPTBackend_NFCV::forget_tag(TagID tag) {
    if (tag >= tags.size()) {
        return;
    }

    auto &tag_data = tags.at(tag);

    if (tag_data.state == TagData::State::free) {
        return;
    }

    known_tags_per_antenna[tag_data.antenna]--;
    tag_data.state = TagData::State::free;
}

void OPTBackend_NFCV::reset_state() {
    for (size_t i = 0; i < tags.size(); ++i) {
        forget_tag(i);
    }
    events.clear();
}

OPTBackend::IOResult<void> OPTBackend_NFCV::initialize_tag(TagID tag, const InitializeTagParams &params) {
    if (!is_valid(tag)) {
        return std::unexpected(IOError::invalid_id);
    }
    const TagData &tag_data = tags.at(tag);

    // This procedure is only implemented for our ICODE SLIX 2 tags
    if (tag_data.tag_type != TagType::slix2) {
        return std::unexpected(IOError::not_implemented);
    }

    const auto recoverable_op = [&](nfcv::Result<void> r, bool *clear_on_failure = nullptr) -> IOResult<void> {
        if (r) {
            return {};

        } else if (params.best_effort && r.error() == nfcv::Error::response_is_error) {
            // The chip will stop responding if we try to do a command with wrong password, we need to power cycle
            reader.field_down();

            if (auto r = reader.field_up(tag_data.antenna); !r) {
                return std::unexpected(to_backend_error(r.error()));
            }

            if (clear_on_failure) {
                *clear_on_failure = false;
            }

            return {};

        } else {
            return std::unexpected(to_backend_error(r.error()));
        }
    };

    // Reset the tag when we finish
    // We don't want to leave the tag with the correct password set
    ScopeGuard field_down_guard = [&] {
        reader.field_down();
    };

    FieldGuard field_guard { *this, tag_data.antenna };
    if (!field_guard) {
        return std::unexpected(field_guard.error());
    }

    // Set up various registers
    {
        // Set AFI to 0
        if (auto r = recoverable_op(reader.write_register(tag_data.uid, nfcv::ReaderWriterInterface::Register::afi, 0)); !r) {
            return r;
        }

        // Set DSFID to 0
        if (auto r = recoverable_op(reader.write_register(tag_data.uid, nfcv::ReaderWriterInterface::Register::dsfid, 0)); !r) {
            return r;
        }

        // Disable EAS
        if (auto r = recoverable_op(reader.write_register(tag_data.uid, nfcv::ReaderWriterInterface::Register::eas, 0)); !r) {
            return r;
        }
    }

    using Policy = InitializeTagParams::ProtectionPolicy;
    using LockMode = nfcv::ReaderWriterInterface::LockMode;
    using Register = nfcv::ReaderWriterInterface::Register;
    switch (params.protection_policy) {

    case Policy::none:
        // All done!
        return {};

    case Policy::write_password: {
        const auto tag_info = reader.get_system_info(tag_data.uid);
        if (!tag_info) {
            return std::unexpected(to_backend_error(tag_info.error()));
        }
        const auto block_size = tag_info->mem_size.value_or(nfcv::TagInfo::MemorySize {}).block_size;
        if (block_size == 0) {
            return std::unexpected(to_backend_error(nfcv::Error::bad_request));
        }

        // Set up the passwords
        for (auto reg : { Register::read_password, Register::write_password, Register::eas_afi_password, Register::destroy_password, Register::privacy_password }) {
            // "Log in" with the default passwords
            const auto default_pwd = (reg == Register::privacy_password || reg == Register::destroy_password) ? 0x0F0F0F0F : 0x00000000;
            bool password_set = true;
            if (auto r = recoverable_op(reader.set_password(tag_data.uid, reg, default_pwd), &password_set); !r) {
                return r;
            }

            if (!password_set) {
                continue;
            }

            // Write our new password
            if (auto r = recoverable_op(reader.write_register(tag_data.uid, reg, params.password)); !r) {
                return r;
            }
        }

        // Password protect memory
        {
            bool both_passwords_set = true;

            for (auto reg : { Register::read_password, Register::write_password }) {
                if (auto r = recoverable_op(reader.set_password(tag_data.uid, reg, params.password), &both_passwords_set); !r) {
                    return r;
                }
            }

            if (both_passwords_set) {
                const nfcv::command::ProtectPage cmd { {
                    .uid = tag_data.uid,
                    .boundary_block_address = static_cast<uint8_t>(params.protect_first_num_bytes / block_size),
                    .l_page_protection = nfcv::SLIX2PageProtection::write,
                    .h_page_protection = nfcv::SLIX2PageProtection::none,
                } };
                if (auto r = recoverable_op(reader.nfcv_command(cmd)); !r) {
                    return r;
                }
            }
        }

        // !!! IRREVERSIBLE OPERATIONS
        // NOTE: The if is there for easy disable during debugging
        if (1) {
            bool eas_afi_password_set = true;
            if (auto r = recoverable_op(reader.set_password(tag_data.uid, Register::eas_afi_password, params.password), &eas_afi_password_set); !r) {
                return r;
            }

            if (eas_afi_password_set) {
                // Password protect EAS
                if (auto r = recoverable_op(reader.lock_register(tag_data.uid, nfcv::ReaderWriterInterface::Register::eas, LockMode::password_protect)); !r) {
                    return r;
                }

                // Password protect AFI
                if (auto r = recoverable_op(reader.lock_register(tag_data.uid, nfcv::ReaderWriterInterface::Register::afi, LockMode::password_protect)); !r) {
                    return r;
                }
            }

            // Lock DSFID - cannot be password protected :(
            if (auto r = recoverable_op(reader.lock_register(tag_data.uid, nfcv::ReaderWriterInterface::Register::dsfid, LockMode::hard_lock)); !r) {
                return r;
            }
        }

        return {};
    }

    case Policy::lock:
        return std::unexpected(IOError::not_implemented);

    case Policy::_cnt:
        // Fallback to unreachable
        break;
    }

    std::unreachable();
}

OPTBackend::IOResult<void> OPTBackend_NFCV::unlock_tag(TagID tag, uint32_t password) {
    if (!is_valid(tag)) {
        return std::unexpected(IOError::invalid_id);
    }
    const TagData &tag_data = tags.at(tag);

    // This procedure is only implemented for our ICODE SLIX 2 tags
    if (tag_data.tag_type != TagType::slix2) {
        return std::unexpected(IOError::not_implemented);
    }

    // Reset the tag when we finish
    // We don't want to leave the tag with the correct password set
    ScopeGuard field_down_guard = [&] {
        reader.field_down();
    };

    FieldGuard field_guard { *this, tag_data.antenna };
    if (!field_guard) {
        return std::unexpected(field_guard.error());
    }

    for (const auto reg : { nfcv::ReaderWriterInterface::Register::read_password, nfcv::ReaderWriterInterface::Register::write_password }) {
        if (auto r = reader.set_password(tag_data.uid, reg, password); !r) {
            return std::unexpected(to_backend_error(r.error()));
        }
    }

    // Unprotect the memory
    {
        const nfcv::command::ProtectPage cmd { {
            .uid = tag_data.uid,
            .boundary_block_address = 0,
            .l_page_protection = nfcv::SLIX2PageProtection::none,
            .h_page_protection = nfcv::SLIX2PageProtection::none,
        } };
        if (auto r = reader.nfcv_command(cmd); !r) {
            return std::unexpected(to_backend_error(r.error()));
        }
    }

    return {};
}

bool OPTBackend_NFCV::is_valid(TagID tag_id) {
    static_assert(!std::is_signed_v<decltype(tag_id)>);
    if (tag_id >= tags.size()) {
        return false;
    }

    return tags.at(tag_id).state != TagData::State::free;
}

void OPTBackend_NFCV::run_next_discovery() {
    if (config_.enforced_antenna != OPTBackend::no_antenna_enforce) {
        discovery_antenna = config_.enforced_antenna;
    }

    /// Force reset the field. If the current field was already active, some tags might be still quieted from the previous discovery.
    reader.field_down();

    FieldGuard field_guard { *this, discovery_antenna };
    if (!field_guard) {
        return;
    }

    // list of found uids in this procedure, we compare them in the end with known uids to detect lost tags
    std::bitset<MAX_KNOWN_TAGS> found_tags {};

    // let's do the procedure until all the tags answer to Inventory command
    while (const auto inv_res = reader.inventory()) {
        const auto uid = inv_res.value();
        debug_assert(uid[nfcv::UID_MSB_INDEX] == nfcv::UID_MSB);

        // Prevent the tag to repeatedly answering to inventory (it will still answer to addressed commands (the rest of them))
        if (!reader.stay_quiet(uid).has_value()) {
            // The tag is probably gone - just skip it for now
            continue;
        }

        // Check if we already detected the tag
        // We don't need to match the antenna, since we don't want to report the same tag twice
        // If the tag moves from one antenna to another we need to first emit tag missing event and then we can detect it on new antenna again
        auto tag_data = std::ranges::find_if(tags, [&](const auto &tag) { return tag.state != TagData::State::free && tag.uid == uid; });
        if (tag_data != tags.end()) {
            // But mark it as found
            if (tag_data->antenna == discovery_antenna) {
                found_tags.set(std::distance(tags.begin(), tag_data));
                tag_data->missed_discovery_count = 0;
            }
            // We already know the tag, we don't need to register it
            continue;
        }

        // Don't track more tags than the limit per antenna
        if (known_tags_per_antenna[discovery_antenna] >= config_.max_known_tags_per_antenna) {
            continue;
        }

        // We found unknown tag, lets ask for more information about it
        const auto tag_info = reader.get_system_info(uid);
        if (!tag_info.has_value()) {
            // Again the tag is probably gone - just skip it for now
            continue;
        }

        // Lets find a free space for the tag info to live
        tag_data = std::ranges::find_if(tags, [&](const auto &tag) { return tag.state == TagData::State::free; });
        if (tag_data == tags.end()) {
            // If we can't fit a new tag into the storage we should skip reporting it for a time.
            // Probably we are waiting for forget tag command - until then we can't report new tags since we need free
            // valid tag_id
            continue;
        }
        TagID tag_id = std::distance(tags.begin(), tag_data);

        // enqueue event to send
        if (!events.enqueue(OPTBackend::TagDetectedEvent { .tag = tag_id, .antenna = discovery_antenna })) {
            // we have too many events - this should never happend but lets be sure
            // we can't report the tag => so we don't want to store it
            // if this ultimately happens increase the events queue by little bit
            continue;
        }

        found_tags.set(tag_id);

        // Lets store all the information available
        static constexpr nfcv::TagInfo::MemorySize def_mem_size { .block_size = 0, .block_count = 0 };
        *tag_data = TagData {
            .uid = uid,
            .antenna = discovery_antenna,
            .block_size = tag_info->mem_size.value_or(def_mem_size).block_size,
            .block_count = tag_info->mem_size.value_or(def_mem_size).block_count,
            .state = TagData::State::known,
            .tag_type = TagType::unknown,
        };
        known_tags_per_antenna[discovery_antenna]++;

        // Determine tag type
        {
            constexpr std::byte uid_7_nfcv { 0xE0 }; // Specified by ISO 15693-3
            constexpr std::byte uid_6_nxp { 0x04 }; // NXP Semiconductors
            constexpr std::byte uid_5_icode { 0x01 }; //  ICODE SLIX 2

            constexpr std::byte uid_4_icode_mask { 0b11000 };
            constexpr std::byte uid_4_icode_slix2 { 0b01000 }; // Differentiate between SLI, SLIX, SLIX2)

            if (true
                && uid[7] == uid_7_nfcv
                && uid[6] == uid_6_nxp
                && uid[5] == uid_5_icode
                && (uid[4] & uid_4_icode_mask) == uid_4_icode_slix2) {
                tag_data->tag_type = TagType::slix2;
            }
        }
    }

    // Lets find a uid that we lost
    for (TagID tag_id = 0; tag_id < tags.size(); ++tag_id) {
        if (!is_valid(tag_id)) {
            continue;
        }

        auto &tag_data = tags.at(tag_id);

        // check if the tag was found during this procedure and check if originaly the it was found on current antenna
        if (tag_data.antenna != discovery_antenna || found_tags.test(tag_id)) {
            continue;
        }

        if (tag_data.missed_discovery_count < config_.tag_max_missed_discoveries) {
            tag_data.missed_discovery_count++;
            continue;
        }

        try_report_tag_lost(tag_id);
    }

    // Cycle antennas between discoveries
    discovery_antenna = (discovery_antenna + 1) % reader.antenna_count();
}

void OPTBackend_NFCV::try_report_tag_lost(TagID tag_id) {
    if (!is_valid(tag_id)) {
        return;
    }

    auto &tag_data = tags.at(tag_id);

    if (tag_data.state != TagData::State::known) {
        return;
    }

    // Detected lost tag - lets mark it and enqueue an event
    if (!events.enqueue(OPTBackend::TagLostEvent { .tag = tag_id })) {
        // we have too many events - this should never happend but lets be sure
        // we can't report the lost tag => so we don't want to mark it as lost
        // if this ultimately happens increase the events queue by little bit
        return;
    }

    if (config_.auto_forget_tag) {
        forget_tag(tag_id);
    } else {
        tag_data.state = TagData::State::lost;
    }
}
