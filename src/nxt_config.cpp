
#include "nxt_config.hpp"

#include <algorithm>

nxt_config::nxt_config(const char* id)
	: m_id(id)
{ }

void nxt_config::parse(const std::string& config_string)
{
	auto it = config_string.begin();
	auto eol_it = it;

	for (; eol_it != config_string.end(); it = eol_it + 1)
	{
		eol_it = std::find(it, config_string.end(), '\n');
		auto eq_it = std::find(it, eol_it, '=');

		if (eq_it == eol_it)
			continue;

		if (std::equal(it, eq_it, "param", "param" + 5)
		 || std::equal(it, eq_it, "msg", "msg" + 3))
		{
			eq_it = std::find(eq_it + 1, eol_it, '=');

			if (eq_it == eol_it)
				continue;
		}

		m_data.insert({std::string(it, eq_it), std::string(eq_it + 1, eol_it)});
	}
}

const std::string& nxt_config::get(const std::string& key) const
{
	auto it = m_data.find(key);

	if (it != m_data.end())
		return it->second;
	else
		throw std::runtime_error(std::string(m_id) + " missing configuration value: " + key);
}

std::string nxt_config::get_else(const std::string& key, const char* fallback) const
{
	auto it = m_data.find(key);

	if (it != m_data.end())
		return it->second;
	else
		return fallback;
}

