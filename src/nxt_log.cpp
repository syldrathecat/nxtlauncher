
#include "nxt_log.hpp"

#include <cstdarg>
#include <cstdio>

int log_level = LOG_LOG;

void nxt_log(int level, const char* format, ...)
{
	if (log_level < level)
		return;

	std::va_list args;
	va_start(args, format);
	std::fputs("[NXT] ", stderr);
	std::vfprintf(stderr, format, args);
	std::fputc('\n', stderr);
	va_end(args);
}
