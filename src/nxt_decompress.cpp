
#include "nxt_decompress.hpp"

#include <climits>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>

#include <lzma.h>

std::string nxt_decompress(const char* data, std::size_t size)
{
	constexpr int buffer_size = 256 * 1024;

	std::string output;
	std::unique_ptr<char[]> output_buffer(new char[buffer_size]);

	lzma_stream stream = LZMA_STREAM_INIT;

	lzma_ret result = lzma_alone_decoder(&stream, UINT64_MAX);

	if (result != LZMA_OK)
		throw std::runtime_error("LZMA decoder initialization failed");

	stream.next_in = reinterpret_cast<const std::uint8_t*>(data);
	stream.avail_in = size;

	while (stream.avail_in > 0)
	{
		stream.next_out = reinterpret_cast<std::uint8_t*>(output_buffer.get());
		stream.avail_out = buffer_size;

		result = lzma_code(&stream, LZMA_RUN);

		std::size_t bytes_decompressed = buffer_size - stream.avail_out;

		output.append(output_buffer.get(), bytes_decompressed);

		if (result != LZMA_OK)
		{
			if (output.size() == 0)
				throw std::runtime_error("LZMA decoder failed");

			// The files we get cause the decoder to return an error
			//   so just return and hope for the best!
			break;
		}
	}

	lzma_end(&stream);

	return output;
}
