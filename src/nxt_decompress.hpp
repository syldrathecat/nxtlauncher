#ifndef NXT_DECOMPRESS_HPP
#define NXT_DECOMPRESS_HPP

#include <string>

#include <cstddef>

// Decompresses an LZMA-compressed string of data
std::string nxt_decompress(const char* data, std::size_t size);

#endif // NXT_DECOMPRESS_HPP

