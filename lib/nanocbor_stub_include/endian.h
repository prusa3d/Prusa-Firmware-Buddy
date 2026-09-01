#pragma once

#define be64toh(x) __builtin_bswap64(x)
#define htobe64(x) __builtin_bswap64(x)
#define be32toh(x) __builtin_bswap32(x)
#define htobe32(x) __builtin_bswap32(x)
#define be16toh(x) __builtin_bswap16(x)
#define htobe16(x) __builtin_bswap16(x)
