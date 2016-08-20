#ifndef NXT_CRC32_HPP_INCLUDED
#define NXT_CRC32_HPP_INCLUDED

#include <cstddef>
#include <cstdint>

std::uint32_t nxt_crc32(const char* data, std::size_t size, std::uint32_t hash = 0x00000000);

#endif // NXT_CRC32_HPP_INCLUDED
