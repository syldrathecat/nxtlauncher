#ifndef NXT_CONFIG_HPP
#define NXT_CONFIG_HPP

#include <string>
#include <unordered_map>

class nxt_config
{
	public:
		using data_t = std::unordered_map<std::string, std::string>;

	private:
		const char* m_id;
		data_t m_data;

	public:
		nxt_config(const char* id);

		void parse(const std::string& config_string);

		const std::string& get(const std::string& key) const;
		std::string get_else(const std::string& key, const char* fallback) const;

		      data_t& data()       { return m_data; }
		const data_t& data() const { return m_data; }
};

#endif // NXT_CONFIG_HPP
