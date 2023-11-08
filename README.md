# RPC Wine

`discord-rpc.dll` implementation for Wine allowing your Wine games to interact
with your native Discord instance.

## Installation

Some users have provided distro packages for this library:

* Arch Linux: [`discord-rpc-wine-git`](https://aur.archlinux.org/packages/discord-rpc-wine-git/)

If your distro isn't listed above, the library can still
be installed with a few simple steps.

#### Dependencies

Install the following dependencies, they are required for either building
or running RPC Wine:

* make
* wine-devel (including `winegcc` and headers)
* RapidJson

#### Building

After installing dependencies and cloning the repository, the `build.sh`
can be used to build both a 32-bit aswell as a 64-bit version of this library.
The resulting binaries can be found in the `bin32` respectively the `bin64` directory
in the source tree.

#### Setup in Wineprefix

Append to the `WINEDLLPATH` environment variable both the `bin32` and
`bin64` directory containing the built library.

Then configure your Wineprefix (`winecfg`) to add `discord-rpc`
(without `.dll` extension) to the DLL overrides list:

![winecfg](https://wontfix.club/i/UUusAV3s.png)

and mark it to prefer the Builtin version over the native one:

![winecfg edit override](https://wontfix.club/i/ihlrxiAp.png)

## Restrictions

RPC Wine currently has not all Discord-RPC methods implemented. This will
be solved with future updates. However, there are other problems with RPC Wine:

RPC Wine is **not** able to run by design:

* Games which don't use the native `discord-rpc.dll` library but their own or a third-party wrapper.
* Games with statically linked `discord-rpc.dll`
