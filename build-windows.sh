#!/bin/bash
# Windows build script for Mandelbrot Explorer

set -e

echo "Building Mandelbrot Explorer for Windows..."
echo "============================================"

# Clean previous build
if [ -d "build-windows" ]; then
    echo "Cleaning previous build..."
    rm -rf build-windows
fi

# Create build directory
mkdir -p build-windows
cd build-windows

# Configure with CMake
echo "Configuring with CMake..."
cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain-mingw.cmake -DCMAKE_BUILD_TYPE=Release ..

# Build
echo "Building..."
make -j$(nproc)

# Check if executable was created
if [ -f "bin/MandelbrotExplorer.exe" ]; then
    echo ""
    echo "Build successful!"
    echo "================"
    ls -lh bin/MandelbrotExplorer.exe
    echo ""
    echo "Executable location: build-windows/bin/MandelbrotExplorer.exe"
    echo "Shaders location: build-windows/bin/shaders/"
else
    echo "ERROR: Build failed - executable not found"
    exit 1
fi
