#include "opt_stub.hpp"

#include <cstring>
#include <cstddef>
#include <utils/byte_utils.hpp>

using namespace openprinttag;

/*
 * regions:
 *   meta:
 *     payload_offset: 0
 *     absolute_offset: 42
 *     size: 4
 *     used_size: 4
 *   main:
 *     payload_offset: 4
 *     absolute_offset: 46
 *     size: 222
 *     used_size: 83
 *   aux:
 *     payload_offset: 226
 *     absolute_offset: 268
 *     size: 35
 *     used_size: 5
 * root:
 *   data_size: 304
 *   payload_size: 261
 *   overhead: 43
 *   payload_used_size: 92
 *   total_used_size: 135
 * data:
 *   meta:
 *     aux_region_offset: 226
 *   main:
 *     material_class: FFF
 *     material_type: ASA
 *     brand_name: Prusament
 *     material_name: PLA Prusa Galaxy Black
 *     nominal_netto_full_weight: 8589934591
 *     transmission_distance: 0.3
 *     tags:
 *     - glitter
 *     - abrasive
 *     brand_specific_instance_id: '1'
 *     material_uuid: 43dab9d7-326c-4c53-9520-eb36a8fa8315
 *   aux:
 *     consumed_weight: 100
 * raw_data:
 *   meta: a10218e2
 *   main: bf080009040b6950727573616d656e740a76504c412050727573612047616c61787920426c61636b101b00000001ffffffff181bf934cd181c9f1704ff056131025043dab9d7326c4c539520eb36a8fa8315ff00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
 *   aux: bf001864ff000000000000000000000000000000000000000000000000000000000000
 * uri: null
 *
 */
static constexpr const std::byte default_tag_data[] {
    std::byte(225), std::byte(64), std::byte(38), std::byte(1), std::byte(3), std::byte(255), std::byte(1), std::byte(39), std::byte(194), std::byte(28), std::byte(0), std::byte(0), std::byte(1), std::byte(5), std::byte(97), std::byte(112), std::byte(112), std::byte(108), std::byte(105), std::byte(99), std::byte(97), std::byte(116), std::byte(105), std::byte(111), std::byte(110), std::byte(47), std::byte(118), std::byte(110), std::byte(100), std::byte(46), std::byte(111), std::byte(112), std::byte(101), std::byte(110), std::byte(112), std::byte(114), std::byte(105), std::byte(110), std::byte(116), std::byte(116), std::byte(97), std::byte(103), std::byte(161), std::byte(2), std::byte(24), std::byte(226), std::byte(191), std::byte(8), std::byte(0), std::byte(9), std::byte(4), std::byte(11), std::byte(105), std::byte(80), std::byte(114), std::byte(117), std::byte(115), std::byte(97), std::byte(109), std::byte(101), std::byte(110), std::byte(116), std::byte(10), std::byte(118), std::byte(80), std::byte(76), std::byte(65), std::byte(32), std::byte(80), std::byte(114), std::byte(117), std::byte(115), std::byte(97), std::byte(32), std::byte(71), std::byte(97), std::byte(108), std::byte(97), std::byte(120), std::byte(121), std::byte(32), std::byte(66), std::byte(108), std::byte(97), std::byte(99), std::byte(107), std::byte(16), std::byte(27), std::byte(0), std::byte(0), std::byte(0), std::byte(1), std::byte(255), std::byte(255), std::byte(255), std::byte(255), std::byte(24), std::byte(27), std::byte(249), std::byte(52), std::byte(205), std::byte(24), std::byte(28), std::byte(159), std::byte(23), std::byte(4), std::byte(255), std::byte(5), std::byte(97), std::byte(49), std::byte(2), std::byte(80), std::byte(67), std::byte(218), std::byte(185), std::byte(215), std::byte(50), std::byte(108), std::byte(76), std::byte(83), std::byte(149), std::byte(32), std::byte(235), std::byte(54), std::byte(168), std::byte(250), std::byte(131), std::byte(21), std::byte(255), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(191), std::byte(0), std::byte(24), std::byte(100), std::byte(255), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(0), std::byte(254)
};

OPTBackend_Stub::OPTBackend_Stub() {
    memcpy(tag_data_, default_tag_data, sizeof(default_tag_data));
}

OPTBackend::IOResult<void> OPTBackend_Stub::read(TagID tag, PayloadPos start, const WritableBytes &buffer) {
    if (tag != 0) {
        return std::unexpected(IOError::invalid_id);
    }

    if (start + buffer.size() > tag_size_) {
        return std::unexpected(IOError::outside_of_bounds);
    }

    memcpy(buffer.data(), tag_data_ + start, buffer.size());
    return {};
}

OPTBackend::IOResult<void> OPTBackend_Stub::write(TagID tag, PayloadPos start, const Bytes &buffer) {
    if (tag != 0) {
        return std::unexpected(IOError::invalid_id);
    }

    if (start + buffer.size() > tag_size_) {
        return std::unexpected(IOError::outside_of_bounds);
    }

    memcpy(tag_data_ + start, buffer.data(), buffer.size());
    return {};
}

bool OPTBackend_Stub::get_event(Event &e, [[maybe_unused]] uint32_t current_time_ms) {
    if (!tag_detected_reported_) {
        tag_detected_reported_ = true;
        e = TagDetectedEvent { .tag = 0, .antenna = 1 };
        return true;
    }

    return false;
}

OPTBackend::IOResult<size_t> OPTBackend_Stub::get_tag_uid(TagID tag, const WritableBytes &buffer) {
    static constexpr std::array uid { std::byte { 0xBC }, std::byte { 0x6F }, std::byte { 0x2F }, std::byte { 0x66 }, std::byte { 0x08 }, std::byte { 0x01 }, std::byte { 0x04 }, std::byte { 0xE0 } };
    if (tag != 0) {
        return std::unexpected(IOError::invalid_id);
    }

    if (buffer.size() < uid.size()) {
        return std::unexpected(IOError::data_too_big);
    }

    memcpy(buffer.data(), uid.data(), uid.size());
    return uid.size();
}

OPTBackend::IOResult<void> OPTBackend_Stub::read_tag_info(TagID tag, TagInfo &target) {
    if (tag != 0) {
        return std::unexpected(IOError::invalid_id);
    }

    target = TagInfo {
        .tlv_span = PayloadSpan::from_offset_end(4, tag_size_),
    };
    return {};
}

void OPTBackend_Stub::forget_tag(TagID tag) {
    if (tag == 0) {
        tag_detected_reported_ = false;
    }
}

void OPTBackend_Stub::reset_state() {
    tag_detected_reported_ = false;
}
