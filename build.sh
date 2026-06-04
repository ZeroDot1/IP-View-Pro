#!/bin/bash
# ═══════════════════════════════════════════════════════════════
#  IPView Pro v2.15.0 — Build Script
#  Qt 6.11 · C++26 (GCC 14+ / Clang 18+) · Arch Linux
#  Public Domain — No License — No Restrictions
# ═══════════════════════════════════════════════════════════════
#
#  Usage:  ./build.sh [BUILD_TYPE] [SANITIZER] [LTO] [TESTS] [VERBOSE] [PGO]
#
#    BUILD_TYPE  Release | Debug | RelWithDebInfo | MinSizeRel  (default: Release)
#    SANITIZER   none | asan | ubsan | tsan                     (default: none)
#    LTO         on  | off                                      (default: on)
#    TESTS       on  | off                                      (default: off)
#    VERBOSE     on  | off                                      (default: off)
#    PGO         generate | use | off                            (default: off)
#
#  Examples:
#    ./build.sh                                # Release, no sanitizer
#    ./build.sh Debug asan on on               # Debug + ASan + LTO + tests
#    ./build.sh Release ubsan off off on       # Release + UBSan, no LTO, no tests, verbose
#    ./build.sh Release none on off off generate   # Stage-1 PGO instrumented build
#    ./build.sh Release none on off off use        # Stage-2 PGO optimised build
#
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

BUILD_TYPE="${1:-Release}"
SANITIZER="${2:-none}"
LTO="${3:-on}"
TESTS="${4:-off}"
VERBOSE="${5:-off}"
PGO="${6:-off}"

# Validate inputs
case "$BUILD_TYPE" in
    Release|Debug|RelWithDebInfo|MinSizeRel) ;;
    *) echo "  ERROR: Unknown BUILD_TYPE '$BUILD_TYPE'"; exit 1 ;;
esac
case "$SANITIZER" in
    none|asan|ubsan|tsan) ;;
    *) echo "  ERROR: Unknown SANITIZER '$SANITIZER'"; exit 1 ;;
esac
case "$LTO" in
    on|off) ;;
    *) echo "  ERROR: LTO must be on or off"; exit 1 ;;
esac
case "$TESTS" in
    on|off) ;;
    *) echo "  ERROR: TESTS must be on or off"; exit 1 ;;
esac
case "$VERBOSE" in
    on|off) ;;
    *) echo "  ERROR: VERBOSE must be on or off"; exit 1 ;;
esac
case "$PGO" in
    generate|use|off) ;;
    *) echo "  ERROR: PGO must be generate, use, or off"; exit 1 ;;
esac

BUILD_DIR="$SCRIPT_DIR/build"
BINARY="$BUILD_DIR/IPView"
JOBS="$(nproc)"

echo "╔═══════════════════════════════════════════════╗"
echo "║      IPView Pro v2.15.0 — C++26 · Qt 6.11    ║"
echo "╚═══════════════════════════════════════════════╝"
echo ""
echo "  Build type : $BUILD_TYPE"
echo "  Sanitizer  : $SANITIZER"
echo "  LTO        : $LTO"
echo "  Tests      : $TESTS"
echo "  Verbose    : $VERBOSE"
echo "  PGO        : $PGO"
echo "  Source dir : $SCRIPT_DIR"
echo "  Build  dir : $BUILD_DIR"
echo "  Jobs       : $JOBS"
echo ""

# ── Check for required tools ────────────────────────
MISSING=""
command -v cmake >/dev/null 2>&1 || MISSING+=" cmake"
if [ -n "$MISSING" ]; then
    echo "  ERROR: Missing required tools:$MISSING"
    echo "  Install: sudo pacman -S cmake qt6-base qt6-svg base-devel"
    exit 1
fi

# ── Build extra CMake flags ─────────────────────────
EXTRA_FLAGS=()

case "$SANITIZER" in
    asan)
        EXTRA_FLAGS+=("-DSANITIZE=ON")
        EXTRA_FLAGS+=("-DSANITIZER_TYPE=address")
        ;;
    ubsan)
        EXTRA_FLAGS+=("-DSANITIZE=ON")
        EXTRA_FLAGS+=("-DSANITIZER_TYPE=undefined")
        ;;
    tsan)
        EXTRA_FLAGS+=("-DSANITIZE=ON")
        EXTRA_FLAGS+=("-DSANITIZER_TYPE=thread")
        ;;
esac

if [ "$LTO" = "off" ]; then
    EXTRA_FLAGS+=("-DENABLE_LTO=OFF")
else
    EXTRA_FLAGS+=("-DENABLE_LTO=ON")
fi

if [ "$TESTS" = "on" ]; then
    EXTRA_FLAGS+=("-DBUILD_TESTING=ON")
else
    EXTRA_FLAGS+=("-DBUILD_TESTING=OFF")
fi

# PGO: stage 1 (generate) instruments with -fprofile-generate;
# stage 2 (use) feeds the captured .gcda profiles back to the
# compiler with -fprofile-use. The toolchain merges all files
# matching "$PROFILE_DIR/*.gcda" into the final binary.
PROFILE_DIR="$SCRIPT_DIR/build/pgo-profiles"
case "$PGO" in
    generate)
        EXTRA_FLAGS+=("-DCMAKE_CXX_FLAGS=-fprofile-generate=$PROFILE_DIR")
        EXTRA_FLAGS+=("-DCMAKE_C_FLAGS=-fprofile-generate=$PROFILE_DIR")
        mkdir -p "$PROFILE_DIR"
        echo "  PGO stage 1 — instrumented build, profiles in $PROFILE_DIR"
        echo "  Run the binary, then re-run with PGO=use for the optimised build."
        ;;
    use)
        EXTRA_FLAGS+=("-DCMAKE_CXX_FLAGS=-fprofile-use=$PROFILE_DIR -fprofile-correction")
        EXTRA_FLAGS+=("-DCMAKE_C_FLAGS=-fprofile-use=$PROFILE_DIR -fprofile-correction")
        echo "  PGO stage 2 — optimised build using profiles from $PROFILE_DIR"
        ;;
esac

# ── Configure with CMake ──────────────────────────────
echo "  Configuring with CMake ($BUILD_TYPE)..."
mkdir -p "$BUILD_DIR"
cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR" \
      -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
      "${EXTRA_FLAGS[@]}" 2>&1 | sed 's/^/    /'

# ── Build ─────────────────────────────────────────────
echo ""
echo "  Building..."
BUILD_FLAGS=(--build "$BUILD_DIR" -j"$JOBS")
if [ "$VERBOSE" = "on" ]; then
    BUILD_FLAGS+=(--verbose)
fi
cmake "${BUILD_FLAGS[@]}" 2>&1 | sed 's/^/    /'

# ── Run tests if requested ───────────────────────────
if [ "$TESTS" = "on" ]; then
    echo ""
    echo "  Running tests..."
    (cd "$BUILD_DIR" && ctest --output-on-failure) || {
        echo "  ERROR: Tests failed"; exit 1;
    }
fi

# ── Done ──────────────────────────────────────────────
echo ""
echo "╔═══════════════════════════════════════════════╗"
echo "║      Build complete!                         ║"
echo "╚═══════════════════════════════════════════════╝"
echo "  Binary: $BINARY"
echo "  Run  : $BINARY"
echo ""
echo "  Usage: $0 [BUILD_TYPE] [SANITIZER] [LTO] [TESTS] [VERBOSE] [PGO]"
echo "  Install: sudo ./install.sh"
