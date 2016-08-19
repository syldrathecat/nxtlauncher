
#include "nxt_config.hpp"
#include "nxt_ipc.hpp"
#include "nxt_log.hpp"

#include <algorithm>
#include <stdexcept>
#include <thread>

#include <cstdarg>
#include <cstdio>
#include <cstdlib>

#include <sys/types.h>
#include <dlfcn.h>
#include <unistd.h>

const char* jav_config_data = R"xxx(
<<<config goes here>>>
)xxx";

// int RunMainBootstrap(const char* params, int width, int height, int unk1, int unk2);
// Not sure if unk1 and unk2 are really parameters, but they do no harm on the Unix AMD64 ABI
using RunMainBootstrap_t = int(*)(const char*, int, int, int, int);

// Added by default to the end of the HOME environment variable to create the default preferences and binary path
#define LAUNCHER_PATH_SUFFIX "/Jagex/launcher"

// Name of the config file from the launcher path to load by default
#define PREFERENCES_NAME "/preferences.cfg"

// Name of the binary from the launcher path to load by default
#define CLIENT_BINARY_NAME "/librs2client.so"

// Used to create the FIFO launcher path: e.g. /tmp/RS2LauncherConnection_XXXX_i
#define FIFO_PREFIX "/tmp/RS2LauncherConnection_"
#define FIFO_IN_SUFFIX "_i"
#define FIFO_OUT_SUFFIX "_o"

// Default configuration URL with %i to be replaced by the language ID
#define DEFAULT_CONFIG_URL "http://www.runescape.com/k=5/l=%i/jav_config.ws?binaryType=4"

// Parent window ID for the client to attach itself to (0 for root, aka no parent)
#define PARENT_WINDOW_ID 0

namespace
{
	nxt_config_parser::config_t config;

	std::string build_launcher_params(const char* pidcode)
	{
		std::string params;

		params.reserve(1536);

		for (auto x : config)
		{
			const std::string& config_key = x.first;
			const std::string& config_value = x.second;

			if (config_key.compare(0, 6, "param=") == 0)
				params = params + '"' + config_key.substr(6) + '"' + ' ' + '"' + config_value + '"' + ' ';
		}

		params += " launcher ";
		params += pidcode;

		return params;
	}

	std::string build_fifo_filename(const char* pidcode, const char* suffix)
	{
		return std::string(FIFO_PREFIX) + pidcode + suffix;
	}

	std::string get_launcher_path()
	{
		char* env_nxtlauncher_path = std::getenv("NXTLAUNCHER_PATH");

		if (env_nxtlauncher_path)
			return std::string(env_nxtlauncher_path);

		char* env_home = std::getenv("HOME");

		if (!env_home)
		{
			nxt_log(LOG_ERR, "HOME environment variable not found - Please set NXTLAUNCHER_PATH");
			return "/root/" LAUNCHER_PATH_SUFFIX;
		}

		return std::string(env_home) + LAUNCHER_PATH_SUFFIX;
	}
/*
	std::string get_config_filename()
	{
		char* env_nxtlauncher_config = std::getenv("NXTLAUNCHER_CONFIG");

		if (env_nxtlauncher_config)
			return std::string(env_nxtlauncher_config);

		return get_launcher_path() + PREFERENCES_NAME;
	}

	std::string get_config_else(const std::string& key, const char* fallback)
	{
		auto it = config.find(key);

		if (it != config.end())
			return it->second;
		else
			return fallback;
	}
*/
}

int main(/*int argc, char** argv*/) try
{
	int mypid = static_cast<int>(::getpid()) & 0xFFFF;
	char pidcode[5];

	std::snprintf(pidcode, sizeof(pidcode), "%04X", mypid);

	std::string fifo_in_filename = build_fifo_filename(pidcode, FIFO_IN_SUFFIX);
	std::string fifo_out_filename = build_fifo_filename(pidcode, FIFO_OUT_SUFFIX);

	std::string launcher_path = get_launcher_path();
	std::string client_binary = launcher_path + CLIENT_BINARY_NAME;

	// TODO : Get from config
	std::string cache_folder = "/home/child/Jagex";
	std::string user_folder = "/home/child/Jagex";

	nxt_log(LOG_VERBOSE, "fifo_in_filename = %s", fifo_in_filename.c_str());
	nxt_log(LOG_VERBOSE, "fifo_out_filename = %s", fifo_out_filename.c_str());
	nxt_log(LOG_VERBOSE, "launcher_path = %s", launcher_path.c_str());
	nxt_log(LOG_VERBOSE, "client_binary = %s", client_binary.c_str());
	nxt_log(LOG_VERBOSE, "cache_folder = %s", cache_folder.c_str());
	nxt_log(LOG_VERBOSE, "user_folder = %s", user_folder.c_str());

	//nxt_config_parser(config).parse("test");
	parse_config(config, jav_config_data);

	int client_w = 1024;
	int client_h = 768;

	void* librs2client = ::dlopen(client_binary.c_str(), RTLD_LAZY);

	if (!librs2client)
	{
		nxt_log(LOG_ERR, "dlopen(%s) failed: %s", client_binary.c_str(), dlerror());
		return 1;
	}

	nxt_ipc ipc(fifo_in_filename.c_str(), fifo_out_filename.c_str());

	// First message sent by client to launcher
	// Received data is always: 00 01 00 02
	ipc.register_handler(0x0000, [&](nxt_message&)
	{
		std::size_t message_size = 15 + cache_folder.size() + user_folder.size();

		nxt_log(LOG_LOG, "Sending init reply...");
		nxt_message reply(0x0001, message_size);
		reply.add_raw("\x00\x02\x00\x00\x00\x00", 6);
		reply.add_int(PARENT_WINDOW_ID);
		reply.add_stringz(cache_folder.c_str());
		reply.add_stringz(user_folder.c_str());
		reply.add_raw("\x00\x03\x00", 3);
		ipc.send(reply);
	});

	// Received after first 0x000D message (During loading)
	// Data always 00 01 00 00 00 00 XX XX XX XX
	// where XX XX XX XX is the game's window ID
	ipc.register_handler(0x0002, [&](nxt_message&/* msg*/)
	{
		/*
		unsigned short unk1 = msg.get_short();
		unsigned int unk2 = msg_get_int();
		unsigned int window_id = msg.get_int();
		*/

		nxt_log(LOG_LOG, "Loading...");
	});

	// Final message received when closing:
	// Data always 00 01
	ipc.register_handler(0x000C, [&](nxt_message&)
	{
		nxt_log(LOG_LOG, "Closing...");
	});

	// --- Other seen messages ---
	// 0x0003:
	// Received when then login screen appears:
	// Data always 00 01
	//
	// 0x0004:
	// Received after logging in to lobby or a world (2 of 2)
	// Data always 00 01
	//
	// 0x0005:
	// Received after logging out of lobby or game
	// Data always 00 01
	//
	// 0x0006:
	// Received after logging in to lobby or a world (1 of 2)
	// Data always 00 01
	//
	// 0x0008:
	// Received a couple of times during loading
	// and every approx. 30 seconds while running:
	// Data always 00 01
	//
	// 0x000D:
	// Received several times while loading before login appears
	// Data structure: { short a; int b; }
	// - a always 00 01
	// - b is an increasing number between around 0x40000000 and 0x42xxxxxx
	//
	// 0x0016:
	// Received several times while closing:
	// Data structure: { short a; byte b; }
	// - a always 0x0001
	// - b ranges from 0x00 to 0x1D

	std::thread fifo_thread([&]() { ipc(); });

	RunMainBootstrap_t RunMainBootstrap = reinterpret_cast<RunMainBootstrap_t>(::dlsym(librs2client, "RunMainBootstrap"));

	if (!RunMainBootstrap)
	{
		nxt_log(LOG_ERR, "dlsym(RunMainBootstrap) failed: %s", dlerror());
		return 2;
	}

	std::string params = build_launcher_params(pidcode);

	nxt_log(LOG_VERBOSE, "params: %s", params.c_str());
	nxt_log(LOG_VERBOSE, "RunMainBootstrap...");
	int result = RunMainBootstrap(params.c_str(), client_w, client_h, 1, 0);

	nxt_log(LOG_LOG, "Result = %i", result);

	//fifo_thread.join();

	::dlclose(librs2client);

	return result;
}
catch (std::exception& e)
{
	nxt_log(LOG_ERR, "%s", e.what());
	throw;
}
catch (...)
{
	throw;
}
