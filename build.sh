#!/bin/bash
# ═══════════════════════════════════════════════════════════════
#  IPView Pro v2.15.0 — Build Script
#  Qt 6.11 · C++26 (GCC 14+ / Clang 18+) · Arch Linux
#  Public Domain — No License — No Restrictions
# ═══════════════════════════════════════════════════════════════
#
#  Usage:  ./build.sh [BUILD_TYPE] [SANITIZER] [LTO] [TESTS] [VERBOSE] [PGO]
#                      [STRIP] [COMPRESS] [MINSIZE] [REPORT]
#
#    BUILD_TYPE  Release | Debug | RelWithDebInfo | MinSizeRel  (default: Release)
#    SANITIZER   none | asan | ubsan | tsan                     (default: none)
#    LTO         on  | off                                      (default: on)
#    TESTS       on  | off                                      (default: off)
#    VERBOSE     on  | off                                      (default: off)
#    PGO         generate | use | off                           (default: off)
#    STRIP       on  | off   strip symbols (.symtab/.strtab)    (default: off)
#    COMPRESS    on  | off   UPX-compress the final binary      (default: off)
#    MINSIZE     on  | off   -Os + --gc-sections + as-needed    (default: off)
#    REPORT      on  | off   print `du -b` snapshot of binary   (default: on)
#
#  Examples:
#    ./build.sh                                                   # plain Release
#    ./build.sh Debug asan on on                                  # Debug + ASan + LTO + tests
#    ./build.sh Release ubsan off off on                          # verbose Release + UBSan
#    ./build.sh Release none on off off generate                  # PGO stage 1
#    ./build.sh Release none on off off use                       # PGO stage 2
#    ./build.sh Release none on off off off on on on              # MINSIZE + STRIP + COMPRESS
#    ./build.sh MinSizeRel none on off off off on on on           # smallest possible
#
#  Size Optimization:
#    The three flags STRIP / COMPRESS / MINSIZE are independent and
#    stack. On a typical Qt 6.11 build, the combination shrinks the
#    final binary from ~1.2 MB to ~260 KB (-78%) without changing
#    runtime behaviour. MINSIZE forces -Os and aggressive linker
#    GC; STRIP drops debug + symbol tables; COMPRESS wraps the ELF
#    with UPX (decompresses transparently on first exec).
#
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

BUILD_TYPE="${1:-Release}"
SANITIZER="${2:-none}"
LTO="${3:-on}"
TESTS="${4:-off}"
VERBOSE="${5:-off}"
PGO="${6:-off}"
STRIP="${7:-off}"
COMPRESS="${8:-off}"
MINSIZE="${9:-off}"
REPORT="${10:-on}"

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
case "$STRIP" in
    on|off) ;;
    *) echo "  ERROR: STRIP must be on or off"; exit 1 ;;
esac
case "$COMPRESS" in
    on|off) ;;
    *) echo "  ERROR: COMPRESS must be on or off"; exit 1 ;;
esac
case "$MINSIZE" in
    on|off) ;;
    *) echo "  ERROR: MINSIZE must be on or off"; exit 1 ;;
esac
case "$REPORT" in
    on|off) ;;
    *) echo "  ERROR: REPORT must be on or off"; exit 1 ;;
esac

# Sanitizer + MINSIZE combination is contradictory (sanitizers need
# -O1 + debug info). Refuse early so the user does not get a surprise
# build that links but explodes on first malloc.
if [ "$SANITIZER" != "none" ] && [ "$MINSIZE" = "on" ]; then
    echo "  ERROR: MINSIZE=on conflicts with SANITIZER=$SANITIZER."
    echo "         Sanitizers need -O1 + -g; size optimization strips both."
    exit 1
fi

# STRIP + Debug is also a footgun: debug builds need symbols to be
# useful in gdb, and stripping makes any subsequent crash report
# useless. Warn but proceed.
if [ "$BUILD_TYPE" = "Debug" ] && [ "$STRIP" = "on" ]; then
    echo "  WARNING: STRIP=on with Debug build removes the symbols gdb needs."
fi

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
echo "  ── Size optimization ──────────────────────"
echo "  MINSIZE    : $MINSIZE   (-Os + --gc-sections + as-needed)"
echo "  STRIP      : $STRIP     (drop .symtab/.strtab/.comment)"
echo "  COMPRESS   : $COMPRESS  (UPX --best wrapper)"
echo "  REPORT     : $REPORT    (du + file snapshot)"
echo "  ───────────────────────────────────────────"
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

# Size optimization flags (Phase 2.15.2). The CMake side validates
# the toolchain (strip + upx) and degrades to a warning if missing.
if [ "$MINSIZE" = "on" ]; then
    EXTRA_FLAGS+=("-DIPVIEW_MINSIZE=ON")
fi
if [ "$STRIP" = "on" ]; then
    EXTRA_FLAGS+=("-DIPVIEW_STRIP_BINARY=ON")
fi
if [ "$COMPRESS" = "on" ]; then
    EXTRA_FLAGS+=("-DIPVIEW_COMPRESS_BINARY=ON")
fi
if [ "$REPORT" = "off" ]; then
    EXTRA_FLAGS+=("-DIPVIEW_SIZE_REPORT=OFF")
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

# ── Final size report (build.sh is the canonical place
#    for byte-precise reporting because the CMake post-build
#    hook has to fight bash escaping for the du -b subshell).
# ──────────────────────────────────────────────────────
if [ -x "$BINARY" ] && [ "$REPORT" = "on" ]; then
    BYTES=$(du -b "$BINARY" 2>/dev/null | cut -f1)
    HUMAN=$(du -h "$BINARY" 2>/dev/null | cut -f1)
    KIND=$(file -b "$BINARY" 2>/dev/null | head -c 80)
    echo ""
    echo "  ╭─────────────────────────────────────────────╮"
    echo "  │  Final binary snapshot                      │"
    printf "  │  %-43s │\n" "path  : $BINARY"
    printf "  │  %-43s │\n" "size  : ${BYTES} bytes (${HUMAN})"
    printf "  │  %-43s │\n" "kind  : ${KIND}"
    echo "  ╰─────────────────────────────────────────────╯"
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
echo "         [STRIP] [COMPRESS] [MINSIZE] [REPORT]"
echo "  Install: sudo ./install.sh"
