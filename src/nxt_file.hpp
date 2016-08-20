#ifndef NXT_FILE_HPP
#define NXT_FILE_HPP

#include <string>

#include <cstddef>

// Local file reader - just made to look similar to nxt_http
class nxt_file
{
	public:
		enum file_mode_t
		{
			mode_binary,
			mode_text
		};

		// Reads a local file's contents in to a string
		std::string get(const char* path, file_mode_t file_mode = mode_binary);

		// Writes a string to a local file
		void put(const char* path, const char* data, std::size_t size, file_mode_t file_mode = mode_binary);
};

#endif // NXT_FILE_HPP

