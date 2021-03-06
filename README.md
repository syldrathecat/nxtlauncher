# nxtlauncher

**nxtlauncher** is a third-party launcher for RuneScape NXT on Linux.

Current version is **2.1**, compatible with NXT **2.2.4**.

**Warning: Using this probably violates RuneScape's terms of service and might stop working one day.**

## What?

The launcher is the program you install locally that downloads the latest RuneScape client and runs it.

Instructions for installing the official Jagex launcher on Ubuntu are available here:

[https://www.runescape.com/download](https://www.runescape.com/download)

An unofficial re-distribution of the official launcher for Arch Linux is available here:

[https://aur.archlinux.org/packages/runescape-launcher/](https://aur.archlinux.org/packages/runescape-launcher/)

## Why?

The official Linux RuneScape launcher has consistently been released with annoying bugs that **nxtlauncher** avoids such as keyboard focus issues and the splash loading screen being stuck open.

It also requires far less disk space to install than the official launcher (which depends on WebKitGTK+) and can integrate more naturally on non-Ubuntu platforms.

Note that you still need to ensure you install the dependencies for the game client itself manually. Check for missing dependencies by using the command `ldd ~/Jagex/launcher/rs2client | grep "not found"` if the game does not load.

## Installation

**nxtlauncher** is written in C++ and can be built and installed using CMake.

### Dependencies

**nxtlauncher** depends on getopt (part of glibc), libcurl, and liblzma.

### Build + Install Steps

```
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release .
make
sudo make install
```

This will install both `nxtlauncher` and `runescape` to `/usr/local/bin/`.

Please see the CMake documentation for more info.

## Usage

An included script `runescape` wraps the launcher the same way the official `runescape-launcher` script does. Executing `nxtlauncher` directly will not set the required environment variables for the game to work correctly (resulting in, for example, no 3D graphics appearing for some graphics drivers).

**nxtlauncher** supports overriding the launcher path and configuration files through environment variables or command line options.

It also supports re-using downloaded game configurations / sessions (valid for 24 hours), and certain extended launcher preferences such as configuration URI and client window dimensions.

### Environment Variables

* `NXTLAUNCHER_PATH` - Sets the launcher path: location for downloaded binaries and preferences.cfg. If not set defaults to `$HOME/Jagex/launcher`.

* `NXTLAUNCHER_CONFIG` - Overrides the launcher config filename so downloaded binaries can be stored separately from the config file. If not set defaults to `$NXTLAUNCHER_PATH/preferences.cfg`.

### Command Line Options

* `-q`, `--quiet` - Only generates output when something goes wrong.
* `-v`, `--verbose` - Enables more detailed output. Specify twice (e.g. `-v -v`) for even more output.
* `--path=<file>` - Uses `<file>` as the launcher path. Overrides `NXTLAUNCHER_PATH`.
* `--config=<file>` - Use `<file>` as the launcher config file. Overrides `NXTLAUNCHER_CONFIG`.
* `--configURI=<uri>` - Use `<uri>` as the game configuration download URI. Overrides the `x_config_uri` config option.
* `--reuse` - Re-uses locally downloaded game configuration. Sessions are valid for 24 hours after downloading.
* `--help`
* `--version`

### Config Settings

**nxtlauncher** will read certain additional settings from the launcher config file (`NXTLAUNCHER_CONFIG` or *preferences.cfg*).

**Note:** Spaces should not appear around the equals sign (e.g. `x_config_uri=https://...`).

* `x_config_uri` - Overrides the game configuration URI. Defaults to *https://www.runescape.com/k=5/l=$(Language:0)/jav_config.ws*.
* `x_saved_config_path` - Path to save downloaded game configuration for `--reuse`. Make blank to not save configs. Defaults to */tmp/jav_config.cache*.
* `x_allow_insecure_dl` - Allows binary downloads over plain-text HTTP, in case Jagex stops offering HTTPS downloads some day.

## Wishlist

Features from the official launcher missing in **nxtlauncher**:

* Window position/size data is not saved.
* No GUI loading screen.
* No prompt when closing the game while logged in.

