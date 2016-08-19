#ifndef NXT_LOG_HPP
#define NXT_LOG_HPP

constexpr int LOG_ERR = 0;
constexpr int LOG_LOG = 1;
constexpr int LOG_VERBOSE = 2;
constexpr int LOG_VERY_VERBOSE = 3;

extern int log_level;

void nxt_log(int level, const char* format, ...);

#endif // NXT_LOG_HPP
