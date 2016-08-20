
#include "nxt_http.hpp"

#include <stdexcept>

#include <cstddef>

#include "nxt_log.hpp"

namespace
{
	std::size_t nxt_http_writefunction(char* ptr, std::size_t size, std::size_t nmemb, void* userdata)
	{
		std::string& buffer = *static_cast<std::string*>(userdata);
		std::size_t num_bytes = size * nmemb;

		buffer.append(ptr, num_bytes);

		return num_bytes;
	}
}

nxt_http::nxt_http()
	: m_curl(curl_easy_init())
{
	if (!m_curl)
		throw std::runtime_error("Could not initialize curl: curl_easy_init failed");

	curl_easy_setopt(m_curl, CURLOPT_FOLLOWLOCATION, 1);

}

nxt_http::~nxt_http()
{
	if (m_curl)
		curl_easy_cleanup(m_curl);
}

nxt_http::nxt_http(nxt_http&& other)
	: m_curl(other.m_curl)
{
	other.m_curl = nullptr;
}

nxt_http& nxt_http::operator=(nxt_http&& other)
{
	m_curl = other.m_curl;
	other.m_curl = nullptr;

	return *this;
}

std::string nxt_http::get(const char* url, bool progress)
{
	CURLcode result;
	std::string buffer;

	progress = (log_level >= LOG_VERBOSE) && progress;

	curl_easy_setopt(m_curl, CURLOPT_URL, url);
	curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, reinterpret_cast<void*>(nxt_http_writefunction));
	curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, static_cast<void*>(&buffer));

	curl_easy_setopt(m_curl, CURLOPT_NOPROGRESS, !progress);

	nxt_log(LOG_VERBOSE, "HTTP GET %s...", url);

	result = curl_easy_perform(m_curl);

	if (result != CURLE_OK)
		throw std::runtime_error(std::string("Request for ") + url + " failed:\n\t" + curl_easy_strerror(result));

	return buffer;
}

