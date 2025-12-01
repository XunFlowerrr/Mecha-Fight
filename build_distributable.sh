#!/bin/bash

# ===========================================
# Build Distributable Package Script
# ===========================================
# This script creates a portable build of mecha_fight
# that can be distributed and run on any machine.
#
# Usage:
#   ./build_distributable.sh [platform]
#
# Platforms:
#   macos   - Build for macOS (default on Mac)
#   windows - Build for Windows (requires cross-compilation or Windows machine)
#   linux   - Build for Linux
#
# Output:
#   dist/mecha_fight/  - Complete distributable folder
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR"
BUILD_DIR="$PROJECT_ROOT/build"
DIST_DIR="$PROJECT_ROOT/dist/mecha_fight"

echo "============================================"
echo "Building Mecha Fight - Distributable Package"
echo "============================================"

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with distributable option
echo ""
echo "[1/4] Configuring CMake with BUILD_DISTRIBUTABLE=ON..."
cmake -DBUILD_DISTRIBUTABLE=ON -DCMAKE_BUILD_TYPE=Release ..

# Build the executable
echo ""
echo "[2/4] Building mecha_fight..."
make mecha_fight -j$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)

# Create distribution directory
echo ""
echo "[3/4] Creating distribution package..."
rm -rf "$DIST_DIR"
mkdir -p "$DIST_DIR"

# Copy executable
cp "$PROJECT_ROOT/bin/mecha_fight/mecha_fight" "$DIST_DIR/"

# Copy shaders (as actual files, not symlinks)
echo "  - Copying shaders..."
for shader in "$PROJECT_ROOT/src/mecha_fight/shaders"/*.{vs,fs,gs,tcs,tes,cs}; do
    if [ -f "$shader" ]; then
        cp "$shader" "$DIST_DIR/"
    fi
done 2>/dev/null || true

# Copy resources
echo "  - Copying resources..."
cp -R "$PROJECT_ROOT/resources" "$DIST_DIR/"

# On macOS, we might need to fix library paths for distribution
if [[ "$OSTYPE" == "darwin"* ]]; then
    echo ""
    echo "[4/4] Checking library dependencies..."
    echo "  Note: For full distribution, you may need to bundle dylibs."
    echo "  Run 'otool -L $DIST_DIR/mecha_fight' to see dependencies."
fi

echo ""
echo "============================================"
echo "Distribution package created at:"
echo "  $DIST_DIR"
echo ""
echo "Contents:"
ls -la "$DIST_DIR"
echo ""
echo "To run:"
echo "  cd $DIST_DIR && ./mecha_fight"
echo "============================================"
