
#include "nxt_crc32.hpp"
#include "nxt_config.hpp"
#include "nxt_decompress.hpp"
#include "nxt_file.hpp"
#include "nxt_http.hpp"
#include "nxt_ipc.hpp"
#include "nxt_log.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

#include <cstdarg>
#include <cstdio>
#include <cstdlib>

#include <sys/types.h>
#include <dlfcn.h>
#include <unistd.h>

#include <getopt.h>

#ifdef ENABLE_X11
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#endif

// ---

#define NXTLAUNCHER_VERSION "1.0"

// 1 = 32-bit(?) Windows
// 2 = 64-bit(?) Windows
// 3 = OS X
// 4 = Linux
// 5 = 32-bit(?) Windows + DLLs
// 6 = 64-bit(?) Windows + DLLs
#define CLIENT_BINARY_TYPE "4"

// Default client language
// 0 = English
// 1 = German
// 2 = French
// 3 = Brazilian Portuguese
#define CLIENT_LANGUAGE "0"

// Default client width
#define CLIENT_WIDTH "1024"

// Default client height
#define CLIENT_HEIGHT "768"

// Default client configuration URI
#define CLIENT_CONFIG_URI "http://www.runescape.com/k=5/l=$(Language:0)/jav_config.ws"

// ---

// Query string appended to downloads
#define BINARY_TYPE_SUFFIX "?binaryType=" CLIENT_BINARY_TYPE

// Token in configuration URI to replace with the language number
#define LANGUAGE_TOKEN "$(Language:0)"

// Added by default to the end of the HOME environment variable to create the default preferences and binary path
#define LAUNCHER_PATH_SUFFIX "/Jagex/launcher"

// Name of the config file from the launcher path to load by default
#define PREFERENCES_SUFFIX "/preferences.cfg"

// Default location to save game configuration for --reuse
#define SAVED_CONFIG_LOCATION "/tmp/jav_config.cache"

// Used to create the FIFO launcher path: e.g. /tmp/RS2LauncherConnection_XXXX_i
#define FIFO_PREFIX "/tmp/RS2LauncherConnection_"
#define FIFO_IN_SUFFIX "_i"
#define FIFO_OUT_SUFFIX "_o"

// Parent window ID for the client to attach itself to (0 for root, aka no parent)
#define PARENT_WINDOW_ID 0

// ---

// int RunMainBootstrap(const char* params, int width, int height, int unk1, int unk2);
// Not sure if unk1 and unk2 are really parameters, but they do no harm on the Unix AMD64 ABI
using RunMainBootstrap_t = int(*)(const char*, int, int, int, int);

namespace
{
	const char* cmdline_path = nullptr;
	const char* cmdline_config = nullptr;
	const char* cmdline_configuri = nullptr;

	// Prints the help message seen when calling nxtlauncher --help
	void print_help(char* argv0)
	{
		std::printf("\
Usage: %1$s [OPTION]...\n\
Replacement launcher program for RuneScape NXT.\n\
\n\
  -q, --quiet       only output when something goes wrong\n\
\n\
  -v, --verbose     more detailed logging output\n\
                    specify twice (e.g. -v -v) for even more output\n\
\n\
  --path=<file>     use <file> as the launcher path\n\
                    downloaded client files are cached here\n\
                    if not specified, defaults to $HOME/Jagex/launcher\n\
\n\
  --config=<file>   load launcher preferences from <file>\n\
                    if not specified, defaults to " PREFERENCES_SUFFIX " in the launcher path\n\
\n\
  --configURI=<uri> download game configuration from <uri>\n\
\n\
  --reuse           reuse locally downloaded game configuration / session\n\
                    sessions are valid for 24 hours after download\n\
\n\
  --help            display this help and exit\n\
\n\
  --version         display version information and exit\n\
\n", argv0);
	}

	// Prints the version message seen when calling nxtlauncher --version
	void print_version()
	{
		std::puts("nxtlauncher " NXTLAUNCHER_VERSION);
		std::puts("  by SyldraTheCat");
	}

	// Generates the full paths of files use for IPC with the client
	std::string build_fifo_filename(const char* pidcode, const char* suffix)
	{
		return std::string(FIFO_PREFIX) + pidcode + suffix;
	}

	// Generates the base launcher path from either the command line setting,
	//   environment variable or using a default
	std::string get_launcher_path()
	{
		if (cmdline_path)
			return cmdline_path;

		const char* env_nxtlauncher_path = std::getenv("NXTLAUNCHER_PATH");

		if (env_nxtlauncher_path)
			return std::string(env_nxtlauncher_path);

		const char* env_home = std::getenv("HOME");

		if (!env_home)
		{
			nxt_log(LOG_ERR, "HOME environment variable not found - Please set NXTLAUNCHER_PATH");
			return "/root" LAUNCHER_PATH_SUFFIX;
		}

		return std::string(env_home) + LAUNCHER_PATH_SUFFIX;
	}

	// Generates the path to the config file from either the command line setting,
	//   environment variable or using a default
	std::string get_config_filename()
	{
		if (cmdline_config)
			return cmdline_config;

		char* env_nxtlauncher_config = std::getenv("NXTLAUNCHER_CONFIG");

		if (env_nxtlauncher_config)
			return std::string(env_nxtlauncher_config);

		return get_launcher_path() + PREFERENCES_SUFFIX;
	}

	// Gets the configuration URI from either the command line setting or launcher config
	std::string get_config_uri(const nxt_config& config)
	{
		if (cmdline_configuri)
			return cmdline_configuri;

		return config.get_else("x_config_uri", CLIENT_CONFIG_URI);
	}

	// Replaces the $(Language:0) part of a configuration URI with the current language ID
	void replace_language_placeholder(std::string& uri, const char* language)
	{
		std::size_t token_pos = uri.find(LANGUAGE_TOKEN);
		std::size_t token_length = sizeof(LANGUAGE_TOKEN) - 1;

		if (token_pos != std::string::npos)
			uri.replace(token_pos, token_length, language);
	}

	// Pack the parameters from the game configuration in to a long string that's passed to the client library
	std::string build_launcher_params(const nxt_config& jav_config, const char* pidcode)
	{
		std::string params;

		params.reserve(1536);

		for (auto x : jav_config.data())
		{
			const std::string& config_key = x.first;
			const std::string& config_value = x.second;

			if (config_key.compare(0, 6, "param=") == 0)
				params = params + '"' + config_key.substr(6) + '"' + ' ' + '"' + config_value + '"' + ' ';
		}

		params += "launcher ";
		params += pidcode;

		return params;
	}
}

int main(int argc, char** argv) try
{
// --- Initialize things ---
	int mypid = static_cast<int>(::getpid()) & 0xFFFF;
	std::string pidcode;

	{
		char pidbuf[5];
		std::snprintf(pidbuf, sizeof(pidbuf), "%X", mypid);
		pidcode = pidbuf;
	}

	std::string fifo_in_filename = build_fifo_filename(pidcode.c_str(), FIFO_IN_SUFFIX);
	std::string fifo_out_filename = build_fifo_filename(pidcode.c_str(), FIFO_OUT_SUFFIX);

// --- Handle command line options ---
	bool reuse_files = false;

	{
		int result;
		int indexptr;

		const option longopts[] = {
			{ "quiet",     no_argument, nullptr, 'q' },
			{ "verbose",   no_argument, nullptr, 'v' },
			{ "path",      required_argument, nullptr, 'P' },
			{ "config",    required_argument, nullptr, 'C' },
			{ "configURI", required_argument, nullptr, 'U' },
			{ "reuse",     no_argument, nullptr, 'R' },
			{ "help",      no_argument, nullptr, 'H' },
			{ "version",   no_argument, nullptr, 'V' },
			{ nullptr,     no_argument, nullptr, 0 },
		};

		while ((result = getopt_long(argc, argv, "qv", longopts, &indexptr)) != -1)
		{
			switch (result)
			{
				case 'q': --log_level; break;
				case 'v': ++log_level; break;
				case 'P': cmdline_path = optarg; break;
				case 'C': cmdline_config = optarg; break;
				case 'U': cmdline_configuri = optarg; break;
				case 'R': reuse_files = true; break;
				case 'V': print_version(); return 0;
				case 'H': print_help(argv[0]); return 0;
				default: std::fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]); return 1;
			}
		}
	}

	std::string launcher_path = get_launcher_path();
	nxt_log(LOG_VERBOSE, "Launcher path is %s", launcher_path.c_str());

// --- Parse preferences ---
	nxt_config config("preferences.cfg");
	nxt_file file;

	std::string preferences_path = get_config_filename();
	nxt_log(LOG_VERBOSE, "Config filename is %s", preferences_path.c_str());
	std::string preferences_data = file.get(preferences_path.c_str(), nxt_file::mode_text);
	config.parse(preferences_data.c_str());

	std::string language     = config.get_else("Language", CLIENT_LANGUAGE),
	            cache_folder = config.get("cache_folder"),
	            user_folder  = config.get("user_folder"),
	            saved_config = config.get_else("x_saved_config_path", SAVED_CONFIG_LOCATION);

	int client_w = std::atoi(config.get_else("x_client_width", CLIENT_WIDTH).c_str()),
	    client_h = std::atoi(config.get_else("x_client_height", CLIENT_HEIGHT).c_str());

// --- Download game configuration ---
	nxt_http http;
	nxt_config jav_config("jav_config");

	if (reuse_files)
	{
		nxt_log(LOG_LOG, "Reusing locally downloaded configuration (%s)...", saved_config.c_str());
		std::string jav_config_data = file.get(saved_config.c_str());
		jav_config.parse(jav_config_data.c_str());
	}
	else
	{
		std::string config_uri = get_config_uri(config) + BINARY_TYPE_SUFFIX;
		replace_language_placeholder(config_uri, language.c_str());
		std::string jav_config_data = http.get(config_uri.c_str());
		jav_config.parse(jav_config_data.c_str());

		if (saved_config.size() > 0)
		{
			nxt_log(LOG_VERBOSE, "Saving config to %s for --reuse", saved_config.c_str());

			try
			{
				file.put(saved_config.c_str(), jav_config_data.data(), jav_config_data.size());
			}
			catch (std::exception& e)
			{
				nxt_log(LOG_ERR, "%s", e.what());
			}
		}
	}

// --- Download client files ---
	if (!reuse_files)
	{
		std::string codebase = jav_config.get("codebase");
		int binary_count = std::atoi(jav_config.get("binary_count").c_str());

		if (binary_count < 1)
			nxt_log(LOG_ERR, "Configuration file has no binaries to download!");

		for (int i = 0; i < binary_count; ++i)
		{
			std::string is = std::to_string(i);

			std::string name = jav_config.get("download_name_" + is);
			std::string filename = launcher_path + '/' + name;
			unsigned long crc = std::atol(jav_config.get("download_crc_" + is).c_str());

			nxt_log(LOG_VERBOSE, "Checking binary %s", filename.c_str());
			std::string filedata;

			try
			{
				filedata = file.get(filename.c_str());
			}
			catch (...)
			{
				// The file probably didn't exist and we need to create it
			}

			unsigned long local_crc = nxt_crc32(filedata.data(), filedata.size());

			if (crc != local_crc)
			{
				nxt_log(LOG_VERBOSE, "%s updated: old=%ul, new=%ul", name.c_str(), local_crc, crc);

				std::string download_uri = codebase + "client" + BINARY_TYPE_SUFFIX
				                         + "&fileName=" + name + "&crc=" + std::to_string(crc);

				std::string download_compressed = http.get(download_uri.c_str(), true);
				std::string download = nxt_decompress(download_compressed.data(), download_compressed.size());

				local_crc = nxt_crc32(download.data(), download.size());

				if (local_crc != crc)
					nxt_log(LOG_ERR, "Downloaded file %s seems corrupt. (crc: %ul, expected: %ul)", name.c_str(), local_crc, crc);

				nxt_log(LOG_VERBOSE, "Saving downloaded file %s", filename.c_str());
				file.put(filename.c_str(), download.data(), download.size());
			}
		}
	}

	std::string title       = jav_config.get("title"),
	            binary_name = jav_config.get("binary_name");

// --- Load and run game + set up IPC ---
	nxt_log(LOG_LOG, "Loading %s (%s)...", title.c_str(), binary_name.c_str());

	void* librs2client = ::dlopen((launcher_path + '/' + binary_name).c_str(), RTLD_LAZY);

	if (!librs2client)
	{
		nxt_log(LOG_ERR, "dlopen(%s) failed: %s", binary_name.c_str(), dlerror());
		return 1;
	}

	nxt_ipc ipc(fifo_in_filename.c_str(), fifo_out_filename.c_str());

	// First message sent by client to launcher
	// Received data is always: 00 01 00 02
	ipc.register_handler(0x0000, [&](nxt_message&)
	{
		std::size_t message_size = 15 + cache_folder.size() + user_folder.size();

		nxt_log(LOG_VERBOSE, "Sending init string to client...");
		nxt_log(LOG_VERBOSE, "cache_folder = %s", cache_folder.c_str());
		nxt_log(LOG_VERBOSE, "user_folder = %s", user_folder.c_str());
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
	ipc.register_handler(0x0002, [&](nxt_message& msg)
	{
		if (msg.size() == 10)
		{
#ifdef ENABLE_X11
			msg.get_short();
			msg.get_int();
			unsigned int window_id = msg.get_int();

			nxt_log(LOG_VERBOSE, "Setting title of window id 0x%x...", window_id);

			Display* display = XOpenDisplay(nullptr);

			if (!display)
			{
				nxt_log(LOG_ERR, "Could not open display. Window title will not be set");
			}
			else
			{
				XChangeProperty(display, static_cast<Window>(window_id), XA_WM_NAME, XA_STRING, CHAR_BIT,
					PropModeReplace, reinterpret_cast<const unsigned char*>(title.data()), static_cast<int>(title.size()));

				XCloseDisplay(display);
			}
#else
			nxt_log(LOG_LOG, "Not compiled with X11 support. Window title will not be set");
#endif
		}

		nxt_log(LOG_LOG, "Loading...");
	});

	// Received several times while loading before login appears
	// Data structure: { short a; int b; }
	// - a always 00 01
	// - b is an increasing number between around 0x40000000 and 0x42xxxxxx
	ipc.register_handler(0x000D, [&](nxt_message&)
	{
		static bool first = true;

		if (log_level < LOG_LOG || log_level >= LOG_VERY_VERBOSE)
			return;

		// Avoid printing a "." before "Loading..."
		if (first)
		{
			first = false;
			return;
		}

		std::putc('.', stdout);
		std::fflush(stdout);
	});

	// 0x0003:
	// Received when then login screen appears:
	// Data always 00 01
	ipc.register_handler(0x0003, [&](nxt_message&)
	{
		static bool first = true;

		if (log_level < LOG_LOG || log_level >= LOG_VERY_VERBOSE)
			return;

		// Prints a line break after our .... progress bar
		if (first)
		{
			std::puts(" Ready!");
			first = false;
			return;
		}
	});

	// Final message received when closing:
	// Data always 00 01
	ipc.register_handler(0x000C, [&](nxt_message&)
	{
		nxt_log(LOG_LOG, "Closing...");
	});

	std::thread fifo_thread([&ipc]() { ipc(); });

	RunMainBootstrap_t RunMainBootstrap = reinterpret_cast<RunMainBootstrap_t>(::dlsym(librs2client, "RunMainBootstrap"));

	if (!RunMainBootstrap)
	{
		nxt_log(LOG_ERR, "dlsym(RunMainBootstrap) failed: %s", dlerror());
		return 1;
	}

	std::string params = build_launcher_params(jav_config, pidcode.c_str());

	nxt_log(LOG_VERY_VERBOSE, "params: %s", params.c_str());

	nxt_log(LOG_VERBOSE, "Running client (%ix%i)...", client_w, client_h);
	int result = RunMainBootstrap(params.c_str(), client_w, client_h, 1, 0);

	nxt_log(LOG_VERY_VERBOSE, "result: %i", result);

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

