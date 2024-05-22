#include <span>
#include <cstdint>

/**
 * Generic helper function that formats unix timestamp as time duration into given buffer
 *
 * Formats given Unix timestamp as duration. So the user will see eather of these:
 *  * %is
 *  * %im %is
 *  * %ih %im
 *  * %id %ih
 * Where s is seconds, m minutes, h hours and d days.
 */
void format_duration(std::span<char> buffer, std::uint32_t duration, bool display_seconds = true);
