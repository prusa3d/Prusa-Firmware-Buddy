#include <cstdio>
#include <string>
#include <utility>

#include <emscripten/emscripten.h>
#include <emscripten/bind.h>

#include "convert/convert.hpp"

emscripten::val ascii2bgcode_cfg(std::string in, bgcode::binarize::BinarizerConfig config)
{
    emscripten::val ret = emscripten::val::null();

    FILE * fin = fmemopen(in.data(), in.size(), "r");

    char *outbuf = nullptr;
    size_t outsz = 0;

    FILE * fout = open_memstream(&outbuf, &outsz);

    if (fout == nullptr) {
        std::string ferrstr = std::string("console.log('Can not open writable memory stream!')");
        emscripten_run_script(ferrstr.c_str());
        return ret;
    }

    bgcode::core::EResult result = bgcode::convert::from_ascii_to_binary(*fin, *fout, config);
    if (result != bgcode::core::EResult::Success) {
        std::string astr = std::string("console.error('Error when translating gcode: ");
        astr += translate_result(result);
        astr += "')";
        emscripten_run_script(astr.c_str());
    }

    fclose(fin);
    fclose(fout);

    ret = emscripten::val::array(outbuf, outbuf + outsz);

    free(outbuf);

    return ret;
}

bgcode::binarize::BinarizerConfig get_config()
{
    bgcode::binarize::BinarizerConfig config;
    config.checksum = bgcode::core::EChecksumType::CRC32;
    config.compression.file_metadata = bgcode::core::ECompressionType::None;
    config.compression.print_metadata = bgcode::core::ECompressionType::None;
    config.compression.printer_metadata = bgcode::core::ECompressionType::None;
    config.compression.slicer_metadata = bgcode::core::ECompressionType::Deflate;
    config.compression.gcode = bgcode::core::ECompressionType::Heatshrink_12_4;
    config.gcode_encoding = bgcode::core::EGCodeEncodingType::MeatPackComments;
    config.metadata_encoding = bgcode::core::EMetadataEncodingType::INI;

    return config;
}

emscripten::val ascii2bgcode(std::string in)
{
    return ascii2bgcode_cfg(std::move(in), get_config());
}

std::string bgcode2ascii_vf(std::string in, bool verify)
{
    std::string ret;

    FILE * fin = fmemopen(in.data(), in.size(), "rb");

    char *outbuf = nullptr;
    size_t outsz = 0;

    FILE * fout = open_memstream(&outbuf, &outsz);
    if (fout == nullptr) {
        std::string ferrstr = std::string("console.log('Can not open writable memory stream!')");
        emscripten_run_script(ferrstr.c_str());
        return ret;
    }

    bgcode::core::EResult result = bgcode::convert::from_binary_to_ascii(*fin, *fout, verify);
    if (result != bgcode::core::EResult::Success) {
        std::string astr = std::string("console.error('Error when translating gcode: ");
        astr += translate_result(result);
        astr += "')";
        emscripten_run_script(astr.c_str());
    }

    fclose(fin);
    fclose(fout);

    ret.reserve(outsz);
    std::copy(outbuf, outbuf + outsz, std::back_inserter(ret));
    free(outbuf);

    return ret;
}

std::string bgcode2ascii_and_verify(std::string in)
{
    bool verify_checksum = true;
    return bgcode2ascii_vf(std::move(in), verify_checksum);
}

std::string bgcode2ascii(std::string in)
{
    bool verify_checksum = false;
    return bgcode2ascii_vf(std::move(in), verify_checksum);
}

EMSCRIPTEN_BINDINGS(module) {
    emscripten::function("ascii2bgcode", &ascii2bgcode);
    emscripten::function("ascii2bgcode_cfg", &ascii2bgcode_cfg);
    emscripten::function("bgcode2ascii", &bgcode2ascii);
    emscripten::function("bgcode2ascii_and_verify", &bgcode2ascii_and_verify);

    emscripten::enum_<bgcode::core::ECompressionType>("BGCode_CompressionType")
        .value("None", bgcode::core::ECompressionType::None)
        .value("Deflate", bgcode::core::ECompressionType::Deflate)
        .value("Heatshrink_11_4", bgcode::core::ECompressionType::Heatshrink_11_4)
        .value("Heatshrink_12_4", bgcode::core::ECompressionType::Heatshrink_12_4);
    emscripten::enum_<bgcode::core::EGCodeEncodingType>("BGCode_GCodeEncodingType")
        .value("None", bgcode::core::EGCodeEncodingType::None)
        .value("MeatPack", bgcode::core::EGCodeEncodingType::MeatPack)
        .value("MeatPackComments", bgcode::core::EGCodeEncodingType::MeatPackComments);
    emscripten::enum_<bgcode::core::EMetadataEncodingType>("BGCode_MetadataEncodingType")
        .value("INI", bgcode::core::EMetadataEncodingType::INI);
    emscripten::enum_<bgcode::core::EChecksumType>("BGCode_ChecksumType")
        .value("None", bgcode::core::EChecksumType::None)
        .value("CRC32", bgcode::core::EChecksumType::CRC32);
    emscripten::value_object<bgcode::binarize::BinarizerConfig::Compression>("BGCode_BinarizerCompression")
        .field("file_metadata", &bgcode::binarize::BinarizerConfig::Compression::file_metadata)
        .field("printer_metadata", &bgcode::binarize::BinarizerConfig::Compression::printer_metadata)
        .field("print_metadata", &bgcode::binarize::BinarizerConfig::Compression::print_metadata)
        .field("slicer_metadata", &bgcode::binarize::BinarizerConfig::Compression::slicer_metadata)
        .field("gcode", &bgcode::binarize::BinarizerConfig::Compression::gcode);
    emscripten::value_object<bgcode::binarize::BinarizerConfig>("BGCode_BinarizerConfig")
        .field("compression", &bgcode::binarize::BinarizerConfig::compression)
        .field("gcode_encoding", &bgcode::binarize::BinarizerConfig::gcode_encoding)
        .field("metadata_encoding", &bgcode::binarize::BinarizerConfig::metadata_encoding)
        .field("checksum", &bgcode::binarize::BinarizerConfig::checksum);

    emscripten::function("get_config", &get_config);
}
