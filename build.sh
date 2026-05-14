#!/bin/bash
# ═══════════════════════════════════════════════════════════════
#  IPView Pro v2.7.0 – Build Script
#  Qt 6.11 · C++26 (GCC 14+ / Clang 18+) · Arch Linux
#  Public Domain — No License — No Restrictions
# ═══════════════════════════════════════════════════════════════

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_TYPE="${1:-Release}"
BUILD_DIR="$SCRIPT_DIR/build"
BINARY="$BUILD_DIR/IPView"

echo "╔═══════════════════════════════════════════════╗"
echo "║      IPView Pro v2.5.0 — C++26 · Qt 6.11     ║"
echo "╚═══════════════════════════════════════════════╝"
echo ""
echo "  Build type : $BUILD_TYPE"
echo "  Source dir : $SCRIPT_DIR"
echo "  Build  dir : $BUILD_DIR"
echo ""

# ── Prüfen auf benötigte Tools ────────────────────────
MISSING=""
command -v cmake >/dev/null 2>&1 || MISSING+=" cmake"
if [ -n "$MISSING" ]; then
    echo "  ERROR: Missing required tools:$MISSING"
    echo "  Install: sudo pacman -S cmake qt6-base qt6-svg"
    exit 1
fi

# ── CMake konfigurieren ───────────────────────────────
echo "  Configuring with CMake ($BUILD_TYPE)..."
mkdir -p "$BUILD_DIR"
cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE"

# ── Bauen ─────────────────────────────────────────────
echo ""
echo "  Building..."
cmake --build "$BUILD_DIR" -j"$(nproc)"

# ── Fertig ────────────────────────────────────────────
echo ""
echo "╔═══════════════════════════════════════════════╗"
echo "║      Build complete!                         ║"
echo "╚═══════════════════════════════════════════════╝"
echo "  Binary: $BINARY"
echo "  Run  : $BINARY"
echo ""
echo   "  Usage: $0 [Release|Debug]"
  echo "  Install: sudo ./install.sh"