#!/bin/bash

set -e  # Exit on error

echo "Building MouseFlip with MinGW-w64..."

# MinGW-w64 cross-compiler tools
WINDRES="x86_64-w64-mingw32-windres"
GCC="x86_64-w64-mingw32-g++"

# Check if MinGW-w64 is installed
if ! command -v $GCC &> /dev/null; then
    echo "Error: MinGW-w64 not found!"
    echo "Install it with: sudo apt install mingw-w64"
    exit 1
fi

# Compile resources
echo "Compiling resources..."
$WINDRES resources/mouseflip.rc -O coff -o resources/mouseflip.res

# Compile and link
echo "Compiling application..."
$GCC -std=c++11 -Wall -Wextra -Wno-unused-parameter -DUNICODE -D_UNICODE \
     -mwindows -municode \
     src/mouseflip.cpp \
     resources/mouseflip.res \
     -o mouseflip.exe \
     -luser32 -lshell32 -static-libgcc -static-libstdc++

echo "Build successful! Output: mouseflip.exe"
