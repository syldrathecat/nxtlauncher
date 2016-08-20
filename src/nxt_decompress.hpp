#ifndef NXT_DECOMPRESS_HPP
#define NXT_DECOMPRESS_HPP

#include <cstddef>
#include <string>

// Decompresses an LZMA-compressed string of data
std::string nxt_decompress(const char* data, std::size_t size);

#endif // NXT_DECOMPRESS_HPP
