#include <prusa3d/nfc/util/ReaderError_1_0.h>
#include <prusa3d/nfc/request/InitializeTag_1_0.h>
#include <openprinttag/opt_reader.hpp>

using namespace openprinttag;

// We have an enum specified in src/can/data_types/prusa3d/nfc/util/ReaderError.1.0.dsdl that needs to match 1:1 OPTReader::Error enum
// Here we are checking that this is really the case

static_assert(prusa3d_nfc_util_ReaderError_1_0_FIELD_NOT_PRESENT == std::to_underlying(OPTReader::Error::field_not_present));
static_assert(prusa3d_nfc_util_ReaderError_1_0_WRONG_FIELD_TYPE == std::to_underlying(OPTReader::Error::wrong_field_type));
static_assert(prusa3d_nfc_util_ReaderError_1_0_WRITE_PROTECTED == std::to_underlying(OPTReader::Error::write_protected));
static_assert(prusa3d_nfc_util_ReaderError_1_0_REGION_CORRUPT == std::to_underlying(OPTReader::Error::region_corrupt));
static_assert(prusa3d_nfc_util_ReaderError_1_0_TAG_INVALID == std::to_underlying(OPTReader::Error::tag_invalid));
static_assert(prusa3d_nfc_util_ReaderError_1_0_DATA_TOO_BIG == std::to_underlying(OPTReader::Error::data_too_big));
static_assert(prusa3d_nfc_util_ReaderError_1_0_OTHER == std::to_underlying(OPTReader::Error::other));
static_assert(prusa3d_nfc_util_ReaderError_1_0_NOT_IMPLEMENTED == std::to_underlying(OPTReader::Error::not_implemented));
static_assert(prusa3d_nfc_util_ReaderError_1_0_RADIO_DISABLED == std::to_underlying(OPTReader::Error::radio_disabled));
static_assert(prusa3d_nfc_util_ReaderError_1_0_FIELD_COLLISION == std::to_underlying(OPTReader::Error::field_collision));
static_assert(std::to_underlying(OPTReader::Error::_cnt) == 10);

static_assert(prusa3d_nfc_request_InitializeTag_1_0_PROTECTION_POLICY_NONE == std::to_underlying(OPTBackend::InitializeTagParams::ProtectionPolicy::none));
static_assert(prusa3d_nfc_request_InitializeTag_1_0_PROTECTION_POLICY_LOCK == std::to_underlying(OPTBackend::InitializeTagParams::ProtectionPolicy::lock));
static_assert(prusa3d_nfc_request_InitializeTag_1_0_PROTECTION_POLICY_WRITE_PASSWORD == std::to_underlying(OPTBackend::InitializeTagParams::ProtectionPolicy::write_password));
static_assert(std::to_underlying(OPTBackend::InitializeTagParams::ProtectionPolicy::_cnt) == 3);
