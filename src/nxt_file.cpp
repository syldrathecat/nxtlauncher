
#include "nxt_file.hpp"

#include <stdexcept>

#include <climits>
#include <cstdio>
#include <cstring>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void nxt_file::mkdir(const char* path, bool is_dir)
{
	std::size_t length = std::strlen(path);

	if (length >= PATH_MAX)
		throw std::runtime_error(std::string("Path length too long: ") + path);
	else if (length == 0)
		return;

	char tmp[PATH_MAX];

	std::strcpy(tmp, path);

	auto try_mkdir = [](const char* path)
	{
		// Path already exists
		if (::access(path, F_OK) == 0)
			return;

		int result = ::mkdir(path, S_IRWXU | S_IRWXG | S_IRWXO);

		if (result != 0)
			throw std::runtime_error(std::string("Could not create directory: ") + path);
	};

	for (char* p = tmp + 1; *p != '\0'; ++p)
	{
		if (*p == '/')
		{
			*p = '\0';
			try_mkdir(tmp);
			*p = '/';
		}
	}

	if (is_dir)
		try_mkdir(tmp);
}

std::string nxt_file::get(const char* path, file_mode_t file_mode)
{
	const char* mode_str =
		  (file_mode == mode_text)
		? "rt"
		: "rb";

	mkdir(path);

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

	mkdir(path);

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

