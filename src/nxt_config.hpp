#ifndef NXT_CONFIG_HPP
#define NXT_CONFIG_HPP

#include <string>
#include <unordered_map>

class nxt_config_parser
{
	public:
		using config_t = std::unordered_map<std::string, std::string>;

	private:
		config_t& config;

	public:
		nxt_config_parser(config_t& config);

		void parse(const std::string& config_string);
};

void parse_config(nxt_config_parser::config_t& config, const std::string& config_string);

#endif // NXT_CONFIG_HPP
