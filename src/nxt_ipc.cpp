
#include "nxt_ipc.hpp"

#include "nxt_log.hpp"
#include "nxt_message.hpp"

#include <array>
#include <memory>
#include <stdexcept>

#include <cerrno>
#include <cstdio>
#include <cstring>

namespace
{
	unsigned int decode_short_be(const char* src)
	{
		const unsigned char* src_uc = reinterpret_cast<const unsigned char*>(src);

		return static_cast<unsigned int>(src_uc[0])
		     | static_cast<unsigned int>(src_uc[1] << 8);
	}

	std::array<char, 2> encode_short_be(unsigned int x)
	{
		return {{static_cast<char>( x       & 0xFF),
		         static_cast<char>((x >> 8) & 0xFF)}};
	}

	void log_message(const char* desc, const nxt_message& msg)
	{
		nxt_log(LOG_VERY_VERBOSE, "%s [id=0x%04X,length=%i]:", desc, msg.id(), static_cast<int>(msg.size()));
		std::putc('\t', stdout);

		for (std::size_t i = 0; i < msg.size(); ++i)
		{
			unsigned char c = msg.data()[i];

			std::printf("%02x ", c);
		}

		std::putc('\n', stdout);
	}
}

void nxt_ipc::handle(nxt_message& message)
{
	auto it = m_handlers.find(message.id());

	if (it != m_handlers.end())
		it->second(message);
}

nxt_ipc::nxt_ipc(const char* fifo_in_filename, const char* fifo_out_filename)
	: m_fifo_in(fifo_in_filename)
	, m_fifo_out(fifo_out_filename)
{
	nxt_fifo fifo_in(fifo_in_filename);
	nxt_fifo fifo_out(fifo_out_filename);

	if (!m_fifo_in.create(0600))
		throw std::runtime_error(std::string("Could not create FIFO ") + fifo_in_filename + ": mkfifo failed: " + std::strerror(errno));

	if (!m_fifo_out.create(0600))
		throw std::runtime_error(std::string("Could not create FIFO ") + fifo_out_filename + ": mkfifo failed: " + std::strerror(errno));
}

void nxt_ipc::register_handler(int message_id, handler_t handler)
{
	m_handlers.insert({message_id, handler});
}

void nxt_ipc::operator()()
{
	if (!m_fifo_in.open(nxt_fifo::mode_read))
		throw std::runtime_error(std::string("Could not open FIFO ") + m_fifo_in.filename() + ": open failed: " + std::strerror(errno));

	if (!m_fifo_out.open(nxt_fifo::mode_write))
		throw std::runtime_error(std::string("Could not open FIFO ") + m_fifo_out.filename() + ": open failed: " + std::strerror(errno));

	while (true)
	{
		char buf[2];

		if (!m_fifo_in.read(buf, 2))
		{
			nxt_log(LOG_LOG, "Client communication ended");
			break;
		}

		std::size_t length = decode_short_be(buf);

		std::unique_ptr<char[]> msg_buf(new char[length]);

		if (!m_fifo_in.read(msg_buf.get(), length))
			break;

		if (length < 2)
		{
			nxt_log(LOG_ERR, "Received message too short to be valid");
			continue;
		}

		nxt_message msg(msg_buf.get(), length);

		if (log_level >= LOG_VERY_VERBOSE)
			log_message("Received IPC message", msg);

		handle(msg);
	}
}

void nxt_ipc::send(const nxt_message& msg)
{
	// Uses magic knowledge that nxt_message stores the already-encoded messsage ID
	unsigned int size = static_cast<unsigned int>(msg.size() + 2);

	if (log_level >= LOG_VERY_VERBOSE)
		log_message("Sending IPC message", msg);

	m_fifo_out.write(encode_short_be(size).data(), 2);
	m_fifo_out.write(msg.data() - 2, size);
}

