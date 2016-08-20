
#include "nxt_file.hpp"

#include <stdexcept>

#include <cstdio>

std::string nxt_file::get(const char* path, file_mode_t file_mode)
{
	const char* mode_str =
		  (file_mode == mode_text)
		? "rt"
		: "rb";

	FILE* fh = std::fopen(path, mode_str);
	std::string buffer;

	if (!fh)
		throw std::runtime_error(std::string("Could not open file: ") + path);

	while (!std::feof(fh))
	{
		char buf[1024];

		std::size_t result = std::fread(buf, 1, sizeof(buf), fh);

		if (std::ferror(fh))
		{
			std::fclose(fh);
			throw std::runtime_error(std::string("Failed while reading file: ") + path);
		}

		buffer.append(buf, result);
	}

	std::fclose(fh);

	return buffer;
}

void nxt_file::put(const char* path, const char* data, std::size_t size, file_mode_t file_mode)
{
	const char* mode_str =
		  (file_mode == mode_text)
		? "wt"
		: "wb";

	FILE* fh = std::fopen(path, mode_str);

	if (!fh)
		throw std::runtime_error(std::string("Could not open file: ") + path);

	std::size_t written = 0;

	while (written < size)
	{
		written += std::fwrite(data + written, 1, size - written, fh);

		if (std::ferror(fh))
		{
			std::fclose(fh);
			throw std::runtime_error(std::string("Failed while writing file: ") + path);
		}
	}

	std::fclose(fh);
}
