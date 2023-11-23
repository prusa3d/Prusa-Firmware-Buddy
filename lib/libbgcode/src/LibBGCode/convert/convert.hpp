#ifndef _BGCODE_CONVERT_HPP_
#define _BGCODE_CONVERT_HPP_

#include "convert/export.h"
#include "binarize/binarize.hpp"

namespace bgcode { namespace convert {

// Converts the gcode file contained into src_file from ascii (using the parameters specified with the given config) to binary format
// and save the results into dst_file,
extern BGCODE_CONVERT_EXPORT core::EResult from_ascii_to_binary(FILE& src_file, FILE& dst_file, const binarize::BinarizerConfig& config);

// Converts the gcode file contained into src_file from binary to ascii format and save the results into dst_file
extern BGCODE_CONVERT_EXPORT core::EResult from_binary_to_ascii(FILE& src_file, FILE& dst_file, bool verify_checksum);

}} // bgcode::core

#endif // _BGCODE_CONVERT_HPP_
