
#include "nxt_fifo.hpp"

#include <stdexcept>

#include <cerrno>
#include <cstring>

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

nxt_fifo::nxt_fifo(const char* filename)
	: m_filename(filename)
	, m_fd(-1)
	, m_exists(false)
{
	::unlink(filename);
}

nxt_fifo::nxt_fifo(nxt_fifo&& other)
	: m_filename(other.m_filename)
	, m_fd(other.m_fd)
	, m_exists(other.m_exists)
{
	other.m_filename = nullptr;
	other.m_fd = -1;
	other.m_exists = false;
}

nxt_fifo& nxt_fifo::operator=(nxt_fifo&& other)
{
	m_filename = other.m_filename;
	m_fd = other.m_fd;
	m_exists = other.m_exists;

	other.m_filename = nullptr;
	other.m_fd = -1;
	other.m_exists = false;

	return *this;
}

nxt_fifo::~nxt_fifo()
{
	close();
	unlink();
}

bool nxt_fifo::create(int file_perms)
{
	int result = ::mkfifo(m_filename, file_perms);
	bool success = (result == 0);

	if (success)
		m_exists = true;

	return success;
}

bool nxt_fifo::open(open_mode_t open_mode)
{
	int real_open_mode = 
		  (open_mode == mode_read)
		? O_RDONLY 
		: O_WRONLY;

	m_fd = ::open(m_filename, static_cast<mode_t>(real_open_mode));
	bool success = (m_fd != -1);

	return success;
}

bool nxt_fifo::read(char* data, std::size_t size)
{
	std::size_t bytes_read = 0;

	while (bytes_read < size)
	{
		ssize_t result = ::read(m_fd, data + bytes_read, size - bytes_read);

		if (result <= 0)
		{
			if (result != 0)
				throw std::runtime_error("Client communication failed: read failed: " + std::string(std::strerror(errno)));

			return false;
		}

		bytes_read += result;
	}

	return true;
}

bool nxt_fifo::write(const char* data, std::size_t size)
{
	std::size_t bytes_written = 0;

	while (bytes_written < size)
	{
		ssize_t result = ::write(m_fd, data + bytes_written, size - bytes_written);

		if (result <= 0)
		{
			if (result != 0)
				throw std::runtime_error("Client communication failed: write failed: " + std::string(std::strerror(errno)));

			return false;
		}

		bytes_written += result;
	}

	return true;
}
 
void nxt_fifo::close()
{
	if (m_fd != -1)
	{
		::close(m_fd);
		m_fd = -1;
	}
}

void nxt_fifo::unlink()
{
	if (m_exists)
	{
		::unlink(m_filename);
		m_exists = false;
	}
}

