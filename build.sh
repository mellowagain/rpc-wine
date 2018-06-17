#!/usr/bin/env bash

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)

# Delete previous directory if it exists
if [ -e "/tmp/discord-rpc" ]; then
    rm -rf "/tmp/discord-rpc"
fi

# Create build directory and copy files into it
mkdir -p /tmp/discord-rpc
cp -a "$script_dir/src/." "/tmp/discord-rpc/"
cp "$script_dir/discord-rpc.spec" "/tmp/discord-rpc/discord_rpc.spec"

# Fix up file extension
# Winemaker doesn't detect C++ source and header files with .hh and .cc extension
for file in /tmp/discord-rpc/*.cc /tmp/discord-rpc/*.hh; do
    filename=$(basename -- "$file")
    extension="${filename##*.}"
    filename="${filename%.*}"

    if [ "$extension" == "cc" ]; then
        mv "$file" "/tmp/discord-rpc/`basename $file .cc`.cpp"
        sed -i -e 's/.cc/.cpp/g' "/tmp/discord-rpc/$filename.cpp"
        sed -i -e 's/.hh/.hpp/g' "/tmp/discord-rpc/$filename.cpp"
    fi

    if [ "$extension" == "hh" ]; then
        mv "$file" "/tmp/discord-rpc/`basename $file .hh`.hpp"
        sed -i -e 's/.cc/.cpp/g' "/tmp/discord-rpc/$filename.hpp"
        sed -i -e 's/.hh/.hpp/g' "/tmp/discord-rpc/$filename.hpp"
    fi
done

# Navigate into the build directory
cd /tmp/discord-rpc

# Create required build files
winemaker --dll --wine32 --nomfc --nomsvcrt --nosource-fix -lpthread .

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

# Success!
echo -e "\e[32mSuccessfully built RPC Wine. Put the discord-rpc.dll.so into your WINEDLLPATH environment variable.\e[0m"
