
#include "nxt_ipc.hpp"

#include "nxt_log.hpp"
#include "nxt_message.hpp"

#include <array>
#include <memory>

#include <cerrno>
#include <cstdio>

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
		return {static_cast<char>( x       & 0xFF),
		        static_cast<char>((x >> 8) & 0xFF)};
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
		nxt_log(LOG_ERR, "Could not create FIFO %s: mkfifo failed [errno=%i]", m_fifo_in.filename(), errno);

	if (!m_fifo_out.create(0600))
		nxt_log(LOG_ERR, "Could not create FIFO %s: mkfifo failed [errno=%i]", m_fifo_out.filename(), errno);
}

void nxt_ipc::register_handler(int message_id, handler_t handler)
{
	m_handlers.insert({message_id, handler});
}

void nxt_ipc::operator()()
{
	if (!m_fifo_in.open(nxt_fifo::mode_read))
		nxt_log(LOG_ERR, "Could not open FIFO %s: open failed [errno=%i]", m_fifo_in.filename(), errno);

	if (!m_fifo_out.open(nxt_fifo::mode_write))
		nxt_log(LOG_ERR, "Could not open FIFO %s: open failed [errno=%i]", m_fifo_out.filename(), errno);

	while (true)
	{
		char buf[2];

		if (!m_fifo_in.read(buf, 2))
		{
			nxt_log(LOG_LOG, "librs2client FIFO communication ended");
			break;
		}

		std::size_t length = decode_short_be(buf);

		std::unique_ptr<char[]> msg_buf(new char[length]);

		if (!m_fifo_in.read(msg_buf.get(), length))
			break;

		if (length < 2)
		{
			nxt_log(LOG_VERBOSE, "Received message too short to be valid");
			continue;
		}

		nxt_message msg(msg_buf.get(), length);

		handle(msg);
	}
}

void nxt_ipc::send(const nxt_message& msg)
{
	// Uses magic knowledge that nxt_message stores the already-encoded messsage ID
	unsigned int size = static_cast<unsigned int>(msg.size() + 2);

	m_fifo_out.write(encode_short_be(size).data(), 2);
	m_fifo_out.write(msg.data() - 2, size);
}
