#ifndef NXT_IPC_HPP
#define NXT_IPC_HPP

#include "nxt_fifo.hpp"
#include "nxt_message.hpp"

#include <functional>
#include <map>

// Creates and manages a pair of named pipes for communicating with the client
class nxt_ipc
{
	public:
		using handler_t = std::function<void(nxt_message&)>;

	private:
		nxt_fifo m_fifo_in;
		nxt_fifo m_fifo_out;

		std::map<int, handler_t> m_handlers;

		void handle(nxt_message& message);

	public:
		// Initializes the files for IPC
		nxt_ipc(const char* fifo_in_filename, const char* fifo_out_filename);

		// Not copyable
		nxt_ipc(const nxt_ipc&) = delete;
		nxt_ipc& operator=(const nxt_ipc&) = delete;

		// Moveable, but probably a bad idea if handlers reference the original
		nxt_ipc(nxt_ipc&&) = default;
		nxt_ipc& operator=(nxt_ipc&&) = default;

		// Registers a handler for a given message ID
		// Registering new handlers will overwrite old ones
		void register_handler(int message_id, handler_t handler);

		// Starts pumping messages and executing handlers
		// (expected to be called from a separate thread)
		void operator()();

		// Sends a message to the client
		void send(const nxt_message& msg);
};

#endif // NXT_IPC_HPP

