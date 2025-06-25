#!/bin/bash

# HybridCAD Build Script
# Usage: ./build.sh [clean|release|debug]

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Default build type
BUILD_TYPE="Release"
BUILD_DIR="build"

# Parse command line arguments
case "$1" in
    clean)
        echo -e "${YELLOW}Cleaning build directory...${NC}"
        rm -rf "$BUILD_DIR"
        echo -e "${GREEN}Clean complete.${NC}"
        exit 0
        ;;
    debug)
        BUILD_TYPE="Debug"
        BUILD_DIR="build-debug"
        ;;
    release)
        BUILD_TYPE="Release"
        BUILD_DIR="build-release"
        ;;
    "")
        # Default to release
        ;;
    *)
        echo -e "${RED}Usage: $0 [clean|release|debug]${NC}"
        exit 1
        ;;
esac

echo -e "${BLUE}HybridCAD Build Script${NC}"
echo -e "${BLUE}=====================${NC}"
echo -e "Build type: ${GREEN}$BUILD_TYPE${NC}"
echo -e "Build directory: ${GREEN}$BUILD_DIR${NC}"
echo ""

# Check for required tools
echo -e "${YELLOW}Checking dependencies...${NC}"

if ! command -v cmake &> /dev/null; then
    echo -e "${RED}Error: CMake is not installed${NC}"
    exit 1
fi

if ! command -v make &> /dev/null && ! command -v ninja &> /dev/null; then
    echo -e "${RED}Error: Neither make nor ninja is available${NC}"
    exit 1
fi

# Check for Qt6
echo "Checking for Qt6..."
if ! pkg-config --exists Qt6Core Qt6Widgets Qt6OpenGL Qt6OpenGLWidgets 2>/dev/null; then
    echo -e "${YELLOW}Warning: Qt6 not found via pkg-config, CMake will try to find it${NC}"
fi

# Check for OpenGL
echo "Checking for OpenGL..."
if ! pkg-config --exists gl 2>/dev/null; then
    echo -e "${YELLOW}Warning: OpenGL development libraries may not be installed${NC}"
fi

# Optional: Check for OpenCASCADE
echo "Checking for OpenCASCADE (optional)..."
if pkg-config --exists opencascade 2>/dev/null; then
    echo -e "${GREEN}OpenCASCADE found - advanced CAD features will be available${NC}"
else
    echo -e "${YELLOW}OpenCASCADE not found - some advanced CAD features will be disabled${NC}"
fi

echo ""

# Create build directory
echo -e "${YELLOW}Creating build directory...${NC}"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake
echo -e "${YELLOW}Configuring with CMake...${NC}"
cmake .. \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Build
echo -e "${YELLOW}Building HybridCAD...${NC}"
CORES=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
echo "Using $CORES parallel jobs"

if command -v ninja &> /dev/null && [ -f build.ninja ]; then
    ninja
else
    make -j"$CORES"
fi

echo ""
echo -e "${GREEN}Build completed successfully!${NC}"
echo -e "Executable location: ${BLUE}$(pwd)/HybridCAD${NC}"
echo ""
echo -e "${YELLOW}To run HybridCAD:${NC}"
echo -e "  cd $BUILD_DIR && ./HybridCAD"
echo ""
echo -e "${YELLOW}To install system-wide:${NC}"
echo -e "  cd $BUILD_DIR && sudo cmake --install ." 