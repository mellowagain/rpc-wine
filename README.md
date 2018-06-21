# RPC Wine

Making Discord RPC actually _rich_ by adding support for Wine.
This is achieved by creating a Wine DLL which overrides the original
`discord-rpc.dll` file.

## Dependencies

* [RapidJson](http://rapidjson.org/)

## Installation

A pre-compiled binary can be downloaded from the [Releases](https://github.com/Marc3842h/rpc-wine/releases)
tab. If you prefer building from source, the `build.sh` script can be used
to build the library. You'll end up in both cases with a `discord-rpc.dll.so` file.

Install the library into a path that Wine can find. The
`WINEDLLPATH` environment is used for setting dll override paths.
Append to that environment variable your directory containing
the `discord-rpc.dll.so` file.

Then configure a Wine DLL override in `winecfg`:

![img](https://wontfix.club/i/WAXlzUuh.png)

After that, start the game that has Discord RPC support
and Wine will automatically redirect all Discord RPC calls
to our `discord-rpc.dll.so`, which re-implements a few functions
from the original Discord RPC library.

## Limitations

This Wine DLL override only overrides [`discord-rpc.dll`](https://github.com/discordapp/discord-rpc). If your game
uses a [different implementation](https://github.com/discordapp/discord-rpc#wrappers-and-implementations)
this library will not work.

Currently also only a limited subset of Discord RPC functions
are implemented.
