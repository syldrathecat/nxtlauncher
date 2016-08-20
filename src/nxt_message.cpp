
#include "nxt_message.hpp"

#include <algorithm>
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
		     | static_cast<unsigned int>(src_uc[1] << 16)
		     | static_cast<unsigned int>(src_uc[2] << 8)
		     | static_cast<unsigned int>(src_uc[3]);
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
	, m_pos(2)
{
	if (size < 2)
		throw std::runtime_error("Message must be at least 2 bytes long");
}

nxt_message::nxt_message(int id, std::size_t reserve_size)
	: m_pos(2)
{
	m_data.reserve(2 + reserve_size);
	add_short(static_cast<std::uint16_t>(id));
}

void nxt_message::overflow_check(std::size_t size)
{
	if (m_pos + size > m_data.size())
		throw std::runtime_error("Overflow while reading message");
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

void nxt_message::get_raw(char* data, std::size_t size)
{
	overflow_check(size);

	std::copy(m_data.begin(), m_data.begin() + size, data);
	m_pos += size;
}

std::uint8_t nxt_message::get_byte()
{
	overflow_check(1);

	std::uint8_t result = static_cast<std::uint8_t>(m_data[m_pos]);
	m_pos += 1;

	return result;
}

std::uint16_t nxt_message::get_short()
{
	overflow_check(2);

	std::uint16_t result = static_cast<std::uint16_t>(decode_short_le(&m_data[m_pos]));
	m_pos += 2;

	return result;
}

std::uint32_t nxt_message::get_int()
{
	overflow_check(4);

	std::uint32_t result = static_cast<std::uint32_t>(decode_int_le(&m_data[m_pos]));
	m_pos += 4;

	return result;
}

int nxt_message::id() const
{
	return decode_short_le(m_data.data());
}
