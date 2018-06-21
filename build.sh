#!/usr/bin/env bash

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)

strip=false
_32bit=false

for i in "$@"; do
    case ${i} in
        -s|--strip) strip=true; shift ;;
        -32|--32bit) _32bit=true; shift ;;
        *) echo "Usage: $0 [-s|--strip] [-32|--32bit]"; exit 1 ;;
    esac
    shift
done

# Delete previous directory if it exists
if [ -e "/tmp/discord-rpc" ]; then
    rm -rf "/tmp/discord-rpc"
fi

# Create build directory and copy files into it
mkdir -p /tmp/discord-rpc
cp -r "$script_dir/src/." "/tmp/discord-rpc/"
cp "$script_dir/discord-rpc.spec" "/tmp/discord-rpc/discord_rpc.spec"

# Fix up file extension
# Winemaker doesn't detect C++ source and header files with .hh and .cc extension
for file in $(find /tmp/discord-rpc/ -name '*.cc' -or -name '*.hh'); do
    filename=$(basename -- "$file")
    extension="${filename##*.}"
    filename="${filename%.*}"

    if [ "$extension" == "cc" ]; then
        mv "$file" "$(dirname ${file})/`basename $file .cc`.cpp"

        # Fix includes
        sed -i 's/\.cc/.cpp/g' "$(dirname ${file})/$filename.cpp"
        sed -i 's/\.hh/.hpp/g' "$(dirname ${file})/$filename.cpp"
    fi

    if [ "$extension" == "hh" ]; then
        mv "$file" "$(dirname ${file})/`basename $file .hh`.hpp"

        # Fix includes
        sed -i 's/\.cc/.cpp/g' "$(dirname ${file})/$filename.hpp"
        sed -i 's/\.hh/.hpp/g' "$(dirname ${file})/$filename.hpp"
    fi
done

# Navigate into the build directory
cd /tmp/discord-rpc

# Create required build files
if [ ${_32bit} == "true" ]; then
    winemaker --dll --wine32 --nobanner --nomfc --nomsvcrt --nosource-fix -lpthread .

    # Workaround bug that "-m32" won't get correctly added to Makefile
    # even thought we passed "--wine32" with winemaker.
    cextra_line=$(grep -n -m 1 "CEXTRA" "/tmp/discord-rpc/Makefile" | sed 's/\([0-9]*\).*/\1/')
    cxxextra_line=$(grep -n -m 1 "CXXEXTRA" "/tmp/discord-rpc/Makefile" | sed 's/\([0-9]*\).*/\1/')
    ldflags_line=$(grep -n -m 1 "discord_rpc_dll_LDFLAGS" "/tmp/discord-rpc/Makefile" | sed 's/\([0-9]*\).*/\1/')
    ldflags_line=$((ldflags_line + 1))

    cextra_line="$cextra_line"s
    cxxextra_line="$cxxextra_line"s
    ldflags_line="$ldflags_line"s

    sed -i "$cextra_line/$/ -m32/" "/tmp/discord-rpc/Makefile"
    sed -i "$cxxextra_line/$/ -m32/" "/tmp/discord-rpc/Makefile"
    sed -i "$ldflags_line/$/ -m32/" "/tmp/discord-rpc/Makefile"
else
    winemaker --dll --nobanner --nomfc --nomsvcrt --nosource-fix -lpthread .
fi

# Exit out of the build process if it didn't work
if [ $? -ne 0 ]; then
    echo -e "\e[91mFailed to create required files to build the project. Is Winemaker available?\e[0m"
    exit $?
fi

# Build the project
make

# Exit out of the build process if it didn't work
if [ $? -ne 0 ]; then
    echo -e "\e[91mFailed to build the project. Did you install required dependencies?\e[0m"
    exit $?
fi

# Go back to top level directory
cd "$script_dir"

# Delete previous result binary if it exists
if [ -e "$script_dir/discord-rpc.dll.so" ]; then
    rm -rf "$script_dir/discord-rpc.dll.so"
fi

# Copy result file to script directory
cp "/tmp/discord-rpc/discord_rpc.dll.so" "$script_dir/discord-rpc.dll.so"

if [ ${strip} == "true" ]; then
    strip -s "$script_dir/discord-rpc.dll.so"
fi

# Success!
echo -e "\e[32mSuccessfully built RPC Wine. Put the discord-rpc.dll.so into your WINEDLLPATH environment variable.\e[0m"
