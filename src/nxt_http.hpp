#ifndef NXT_HTTP_HPP
#define NXT_HTTP_HPP

#include <string>

#include <curl/curl.h>

class nxt_http
{
	private:
		CURL* m_curl;

	public:
		nxt_http();

		// Not copyable
		nxt_http(const nxt_http&) = delete;
		nxt_http& operator=(const nxt_http&) = delete;

		// Moveable
		nxt_http(nxt_http&&);
		nxt_http& operator=(nxt_http&&);

		~nxt_http();

		std::string get(const char* url, bool progress = false);
};

#endif // NXT_HTTP_HPP
