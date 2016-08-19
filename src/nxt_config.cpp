
#include "nxt_config.hpp"

#include "nxt_log.hpp"

#include <algorithm>

#include <cstdio>

#include <unistd.h>

void parse_config(nxt_config_parser::config_t& config, const std::string& config_string)
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

		config.insert({std::string(it, eq_it), std::string(eq_it + 1, eol_it)});
	}
}
