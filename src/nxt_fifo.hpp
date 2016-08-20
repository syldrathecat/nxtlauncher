#ifndef NXT_FIFO_HPP
#define NXT_FIFO_HPP

#include <cstddef>

// Manages operations on named pipes
class nxt_fifo
{
	public:
		// File open mode constants for `open()`
		enum open_mode_t
		{
			mode_read,
			mode_write
		};

	private:
		const char* m_filename;
		int m_fd;
		bool m_exists;

	public:
		// Sets the filename to be used but does not create or open it
		// `filename` is assumed to point to a string that will outlive nxt_fifo
		// Deletes the file if it already exists!
		nxt_fifo(const char* filename);

		// Not copyable
		nxt_fifo(const nxt_fifo&) = delete;
		nxt_fifo& operator=(const nxt_fifo&) = delete;

		// Moveable
		nxt_fifo(nxt_fifo&&);
		nxt_fifo& operator=(nxt_fifo&&);
		
		// Automatically closes and deletes the files
		~nxt_fifo();

		// Creates the FIFO file with the given file permission mask
		// Returns false if the file could not be created
		bool create(int file_perms);

		// Opens the FIFO file with the given mode (mode_read or mode_write)
		// Returns false if the file could not be opened
		bool open(open_mode_t open_mode);

		// Reads data from an open pipe in read mode
		// Always 
		// Returns false if the read fails at any point
		bool read(char* data, std::size_t size);

		// Writes data to an open pipe in write mode
		// Returns false if the write fails at any point
		bool write(const char* data, std::size_t size);

		// Closes the file if it is open
		void close();

		// Deletes the file if it has been created by `create()`
		void unlink();

		const char* filename() const { return m_filename; }
};

#endif // NXT_FIFO_HPP

