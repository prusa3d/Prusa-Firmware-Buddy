/**
 * \brief Utilities for generating headers.
 *
 * This is where the low-level routines for generating headers are stored. Note
 * that many handlers already integrate these.
 */
#pragma once

#include <http/types.h>

#include <cstdint>
#include <optional>

struct stat;

namespace nhttp {

/**
 * \brief Lookup of the text corresponding for a status code.
 */
struct StatusText {
    /// The status.
    http::Status status;
    /// Text acompanied for the status.
    const char *text;

    /**
     * \brief Performs a lookup of the text.
     *
     * Note that this always returns _something_ even if the status is not
     * known. Such behaviour is valid, as in HTTP the text is not significant,
     * client is supposed to use the numeric code, but as it provides some
     * guidance to humans, it is still better to extend the lookup table.
     */
    static const StatusText &find(http::Status status);
};

/**
 * \brief Renders the response headers into a buffer.
 *
 * This renders the header part of a HTTP response (including the status line).
 * An array of extra headers that are simply copied into the output can be
 * provided. Such array shall be terminated by a null.
 *
 * This assumes the buffer is large enough to accomodate the output. In case it
 * isn't, the size constraint will still be respected (it won't overwrite
 * memory past the buffer), but the output is silently truncated.
 *
 * The body separator (two lines) is included.
 */
size_t write_headers(uint8_t *buffer, size_t buffer_len, http::Status status, http::ContentType content_type, http::ConnectionHandling handling, std::optional<uint64_t> content_length = std::nullopt, std::optional<uint32_t> etag = std::nullopt, const char *const *extra_hdrs = nullptr);

/**
 * \brief Makes a guess about a content type based on a file extension.
 *
 * Given a filename (not just the extension), this'll make a best-effort guess
 * about the content type.
 *
 * If it is not recognized, application/octet-stream is used as fallback.
 */
http::ContentType guess_content_by_ext(const char *fname);

/**
 * \brief Compute an etag for the given file.
 */
uint32_t compute_etag(const struct stat &stat);

} // namespace nhttp
