#ifndef NXT_MESSAGE_HPP
#define NXT_MESSAGE_HPP

#include <algorithm>
#include <vector>

#include <cstdint>
#include <cstring>

class nxt_message
{
	private:
		std::vector<char> m_data;
		std::size_t m_pos;

		void overflow_check(std::size_t size);

	public:
		nxt_message(const char* data, std::size_t size);
		explicit nxt_message(int id, std::size_t reserve_size = 0);

		void add_raw(const char* data, std::size_t size);
		void add_byte(std::uint8_t b);
		void add_short(std::uint16_t s);
		void add_int(std::uint32_t i);
		void add_stringz(const char* c);

		void get_raw(char* data, std::size_t size);
		std::uint8_t get_byte();
		std::uint16_t get_short();
		std::uint32_t get_int();

		int id() const;
		std::size_t size() const { return m_data.size() - 2; }
		const char* data() const { return m_data.data() + 2; }
};

#endif // RUNEMESSAGE_HPP

