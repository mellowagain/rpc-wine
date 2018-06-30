#!/usr/bin/env bash

# Exit as soon as a command fails
set -e

# Discover directory where this script is located
script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)

# CLI options
skip_32bit=false
skip_64bit=false

# Parse them
positional=()

while [[ $# -gt 0 ]]; do
    key="$1"

    case "$key" in
        -s32|--skip32|--skip32bit|--skip32-bit|--skip-32-bit) echo "Skipping 32-bit..."; skip_32bit=true; shift ;;
        -s64|--skip64|--skip64bit|--skip64-bit|--skip-64-bit) echo "Skipping 64-bit..."; skip_64bit=true; shift ;;
    esac
done

# Restore positional parameters
set -- "${positional[@]}"

# .cc/.hh -> .cpp/.hpp file extension converter
fix_file_extension() {
    for file in $(find $1 -name '*.cc' -or -name '*.hh'); do
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
}

# Builds RPC Wine as 32-bit shared library
build_32bit() {
    # Delete previous directory if it exists
    if [ -e "/tmp/discord-rpc/32bit/discord-rpc" ]; then
        rm -rf "/tmp/discord-rpc/32bit/discord-rpc"
    fi

    # Create build directory and copy files into it
    mkdir -p "/tmp/discord-rpc/32bit/discord-rpc"
    cp -r "$script_dir/src/." "/tmp/discord-rpc/32bit/discord-rpc/"
    cp "$script_dir/discord-rpc.spec" "/tmp/discord-rpc/32bit/discord-rpc/discord_rpc.spec"

    # Fix up file extension
    # Winemaker doesn't detect C++ source and header files with .hh and .cc extension
    fix_file_extension "/tmp/discord-rpc/32bit/discord-rpc/"

    # Navigate into build directory
    cd "/tmp/discord-rpc/32bit/discord-rpc/"

    # Create Makefile and stuff with Winemaker
    winemaker --dll --wine32 --nobanner --nomfc --nomsvcrt --nosource-fix -lpthread .

    # Workaround bug that "-m32" won't get correctly added to Makefile
    # even thought we passed "--wine32" with winemaker.
    if ! grep -q "m32" "/tmp/discord-rpc/32bit/discord-rpc/Makefile"; then
        echo "Detected Wine failing to add 32-bit flags, fixing..."

        cextra_line=$(grep -n -m 1 "CEXTRA" "/tmp/discord-rpc/32bit/discord-rpc/Makefile" | sed 's/\([0-9]*\).*/\1/')s
        cxxextra_line=$(grep -n -m 1 "CXXEXTRA" "/tmp/discord-rpc/32bit/discord-rpc/Makefile" | sed 's/\([0-9]*\).*/\1/')s
        ldflags_line=$(grep -n -m 1 "LDFLAGS" "/tmp/discord-rpc/32bit/discord-rpc/Makefile" | sed 's/\([0-9]*\).*/\1/')
        ldflags_line=$((ldflags_line + 1))s

        sed -i "$cextra_line/$/ -m32/" "/tmp/discord-rpc/32bit/discord-rpc/Makefile"
        sed -i "$cxxextra_line/$/ -m32/" "/tmp/discord-rpc/32bit/discord-rpc/Makefile"
        sed -i "$ldflags_line/$/ -m32/" "/tmp/discord-rpc/32bit/discord-rpc/Makefile"
    fi

    # The command below will fail so don't exit out if that happens
    set +e

    # Workaround for Ubuntu bug that Wine would use 64-bit libraries for linking against 32-bit targets
    winegcc_output=$(winegcc -v -m32 /dev/zero 2>&1)

    # Restore exiting out as soon as a command fails
    set -e

    # Check if WineGCC has 64-bit default libraries while building RPC Wine in 32-bit
    if [[ "$winegcc_output" == *"lib64/wine/libwinecrt0.a"* ]]; then
        echo "Detected Wine failure, trying to fix it up..."

        fixed_string="-nodefaultlibs /usr/lib/i386-linux-gnu/wine/libwinecrt0.a -luser32 -lkernel32 -lgdi32"

        sed -i "${cextra_line::-1} $fixed_string" "/tmp/discord-rpc/32bit/discord-rpc/Makefile"
        sed -i "${cxxextra_line::-1} $fixed_string" "/tmp/discord-rpc/32bit/discord-rpc/Makefile"
    fi

    # Build the project
    make

    # Delete previous result directory if it exists
    if [ -e "$script_dir/bin32" ]; then
        rm -rf "$script_dir/bin32"
    fi

    # Create build result directory
    mkdir -p "$script_dir/bin32"

    # Copy result file to script directory
    cp "/tmp/discord-rpc/32bit/discord-rpc/discord_rpc.dll.so" "$script_dir/bin32/discord-rpc.dll.so"
}

# Builds RPC Wine as 64-bit shared library
build_64bit() {
    # Delete previous directory if it exists
    if [ -e "/tmp/discord-rpc/64bit/discord-rpc" ]; then
        rm -rf "/tmp/discord-rpc/64bit/discord-rpc"
    fi

    # Create build directory and copy files into it
    mkdir -p "/tmp/discord-rpc/64bit/discord-rpc"
    cp -r "$script_dir/src/." "/tmp/discord-rpc/64bit/discord-rpc/"
    cp "$script_dir/discord-rpc.spec" "/tmp/discord-rpc/64bit/discord-rpc/discord_rpc.spec"

    # Fix up file extension
    # Winemaker doesn't detect C++ source and header files with .hh and .cc extension
    fix_file_extension "/tmp/discord-rpc/64bit/discord-rpc/"

    # Navigate into build directory
    cd "/tmp/discord-rpc/64bit/discord-rpc/"

    # Create Makefile and stuff with Winemaker
    winemaker --dll --nobanner --nomfc --nomsvcrt --nosource-fix -lpthread .

    # Build the project
    make

    # Delete previous result directory if it exists
    if [ -e "$script_dir/bin64" ]; then
        rm -rf "$script_dir/bin64"
    fi

    # Create build result directory
    mkdir -p "$script_dir/bin64"

    # Copy result file to script directory
    cp "/tmp/discord-rpc/64bit/discord-rpc/discord_rpc.dll.so" "$script_dir/bin64/discord-rpc.dll.so"
}

# Build 32-bit library if we don't skip
if [[ "$skip_32bit" != "true" ]]; then
    build_32bit
fi

# Build 64-bit library if this is a 64-bit machine
if [[ "$skip_64bit" != "true" ]] || [[ "`uname -a`" == "x86_64" ]]; then
    build_64bit
fi
