
#include "nxt_message.hpp"

#include <array>
#include <stdexcept>

#include <cstring>

namespace
{
	unsigned int decode_short_le(const char* src)
	{
		const unsigned char* src_uc = reinterpret_cast<const unsigned char*>(src);

		return static_cast<unsigned int>(src_uc[0] << 8)
		     | static_cast<unsigned int>(src_uc[1]);
	}

	unsigned int decode_int_le(const char* src)
	{
		const unsigned char* src_uc = reinterpret_cast<const unsigned char*>(src);

		return static_cast<unsigned int>(src_uc[0] << 24)
		     | static_cast<unsigned int>(src_uc[2] << 16)
		     | static_cast<unsigned int>(src_uc[3] << 8)
		     | static_cast<unsigned int>(src_uc[4]);
	}

	std::array<char, 2> encode_short_le(unsigned int x)
	{
		return {static_cast<char>((x >> 8) & 0xFF),
		        static_cast<char>( x       & 0xFF)};
	}

	std::array<char, 4> encode_int_le(unsigned int x)
	{
		return {static_cast<char>((x >> 24) & 0xFF),
		        static_cast<char>((x >> 16) & 0xFF),
		        static_cast<char>((x >>  8) & 0xFF),
		        static_cast<char>( x        & 0xFF)};
	}
}

nxt_message::nxt_message(const char* data, std::size_t size)
	: m_data(data, data + size)
{
	if (size < 2)
		throw std::runtime_error("Message must be at least 2 bytes long");
}

nxt_message::nxt_message(int id, std::size_t reserve_size)
{
	m_data.reserve(2 + reserve_size);
	add_short(static_cast<std::uint16_t>(id));
}

void nxt_message::add_raw(const char* data, std::size_t size)
{
	m_data.insert(m_data.end(), data, data + size);
}

void nxt_message::add_byte(std::uint8_t b)
{
	add_raw(reinterpret_cast<const char*>(&b), 1);
}

void nxt_message::add_short(std::uint16_t s)
{
	add_raw(encode_short_le(s).data(), 2);
}

void nxt_message::add_int(std::uint32_t i)
{
	add_raw(encode_int_le(i).data(), 4);
}

void nxt_message::add_stringz(const char* c)
{
	std::size_t size = std::strlen(c);
	add_raw(c, size + 1);
}

int nxt_message::id() const
{
	return decode_short_le(m_data.data());
}
