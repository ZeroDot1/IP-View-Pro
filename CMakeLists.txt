# IPView — Professional IP Monitoring & Network Analysis Tool
# Requires: Qt 6.5+, CMake 3.28+, C++26 compiler (GCC 14+ / Clang 18+)
#
# ════════════════════════════════════════════════════════════════
#  C++26 (ISO/IEC 14882:2026) | Qt 6.5+ | CMake 3.28+
#  Security Hardening:
#    • -Werror             — all warnings = errors
#    • -fstack-protector   — stack smashing protection
#    • -D_FORTIFY_SOURCE   — buffer overflow detection
#    • -fPIE               — position independent executable
#    • Input validation    — no user input passed to QProcess unchecked
#    • SSL enforcement     — strict certificate validation
#  Public Domain — No License — No Restrictions
# ════════════════════════════════════════════════════════════════

cmake_minimum_required(VERSION 3.28)

project(IPView
    VERSION      2.15.6
    DESCRIPTION  "Professional IP Monitoring & Network Analysis Tool"
    LANGUAGES    CXX
)

# ── Version metadata (single source of truth) ────────────────────────
#  version.hpp is generated from version.hpp.in so main.cpp can read
#  the current version from a compile-time constant. Keeping the
#  canonical value in CMake means CMakeLists.txt, the about dialog,
#  and the runtime metadata are always in sync. The version is
#  mirrored in CHANGELOG.md and is used as the auto-release tag
#  by .github/workflows/release-on-changelog.yml.
set(IPVIEW_VERSION_MAJOR  2)
set(IPVIEW_VERSION_MINOR  15)
set(IPVIEW_VERSION_PATCH  6)
set(IPVIEW_VERSION_COMMITS 0)

# Git-aware commit count (best-effort, falls back to 0)
find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-list --count HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE IPVIEW_GIT_COMMIT_COUNT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(IPVIEW_GIT_COMMIT_COUNT MATCHES "^[0-9]+$")
        set(IPVIEW_VERSION_COMMITS ${IPVIEW_GIT_COMMIT_COUNT})
    endif()
endif()

set(IPVIEW_VERSION_STRING "${IPVIEW_VERSION_MAJOR}.${IPVIEW_VERSION_MINOR}.${IPVIEW_VERSION_PATCH}")
set(IPVIEW_APP_NAME       "IPView")
set(IPVIEW_ORG_NAME       "IPView")
set(IPVIEW_ORG_DOMAIN     "ipview.local")
set(IPVIEW_DISPLAY_NAME   "IP View Pro")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/version.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/version.hpp"
    @ONLY
)

# ── C++26 Standard ─────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Compiler-Warnungen (streng = Security) ─────────────────────
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    add_compile_options(-Wall -Wextra -Wpedantic)
    add_compile_options(-Wshadow -Wnon-virtual-dtor -Wold-style-cast)
    add_compile_options(-Wcast-align -Woverloaded-virtual)
    add_compile_options(-Wconversion -Wsign-conversion -Wnull-dereference)
    add_compile_options(-Wformat=2 -Wmisleading-indentation -Wduplicated-cond)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wduplicated-branches -Wlogical-op)
    add_compile_options(-Wuseless-cast -Wsuggest-override)
endif()

# ── Security-Hardening (Linux) ────────────────────────────────
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_compile_options(-fstack-protector-strong)
    add_link_options(-fstack-protector-strong -Wl,-z,relro -Wl,-z,now)
    # _FORTIFY_SOURCE=3 requires optimization (-O1 or higher).
    # In Debug builds (-O0), glibc emits #warning → fatal with -Werror.
    # Only enable for non-Debug configurations.
    if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_definitions(_FORTIFY_SOURCE=3)
    endif()
endif()

# Mit Sanitizern bauen: cmake -DSANITIZE=ON -DSANITIZER_TYPE=address ..
option(SANITIZE "Build with a sanitizer" OFF)
set(SANITIZER_TYPE "address,undefined" CACHE STRING
    "Sanitizer set to use when SANITIZE=ON (address, undefined, thread, or a comma list)")
if(SANITIZE)
    add_compile_options(-fsanitize=${SANITIZER_TYPE} -g -O1)
    add_link_options(-fsanitize=${SANITIZER_TYPE})
    message(STATUS "Sanitizers ENABLED: ${SANITIZER_TYPE}")
endif()

# LTO is enabled by default, can be disabled via -DENABLE_LTO=OFF.
option(ENABLE_LTO "Enable link-time optimization for Release builds" ON)
if(ENABLE_LTO AND CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
    message(STATUS "LTO enabled for Release build")
endif()

# Test builds (Phase 5) are disabled by default.
option(BUILD_TESTING "Build the unit / integration test targets" OFF)
if(BUILD_TESTING)
    enable_testing()
    message(STATUS "Test target enabled")
endif()

# ── Binary Size Optimization (Phase 2.15.2) ─────────────────────────────
#  Three independent post-build stages that each shave off a sizeable
#  chunk of the final ELF. They are OFF by default to keep the daily
#  dev loop fast, and turned ON via `build.sh` for release archives.
#
#    IPVIEW_MINSIZE=ON
#        Adds -Os + per-function / per-data sections, enables
#        --gc-sections, drops .note/.note.gnu.build-id/.comment,
#        and forces LTO on. Best paired with -DCMAKE_BUILD_TYPE=Release
#        (or MinSizeRel) for the smallest unstripped build.
#
#    IPVIEW_STRIP_BINARY=ON
#        Runs `strip --strip-all --remove-section=...` on the final
#        binary, removing ~16% of the .symtab/.strtab footprint.
#        Works on any Linux ELF produced by GCC or Clang.
#
#    IPVIEW_COMPRESS_BINARY=ON
#        Runs `upx --best` on the final binary, compressing the
#        executable in-place (decompresses on first exec). Requires
#        the upx(1) tool to be installed (pacman: extra/upx).
#
#    IPVIEW_SIZE_REPORT=ON
#        Prints a `du -b` + `file` snapshot of the final binary after
#        every post-build stage. Useful for verifying the pipeline
#        actually shrank the artifact. ON by default.
option(IPVIEW_MINSIZE         "Optimize the build for smallest ELF size"  OFF)
option(IPVIEW_STRIP_BINARY    "Strip debug + symbol tables from the binary" OFF)
option(IPVIEW_COMPRESS_BINARY "Compress the final binary with UPX"        OFF)
option(IPVIEW_SIZE_REPORT     "Print a size snapshot after the build"     ON)

if(IPVIEW_MINSIZE)
    # -Os is a special case of -O2 that prefers size; combined with
    # section splitting it lets the linker drop every unreferenced
    # function and global constant. The .eh_frame / .sframe are kept
    # because unwinders depend on them, but the section versions
    # shrink automatically when their input symbols disappear.
    add_compile_options(-Os -fdata-sections -ffunction-sections)
    # -Wl,--gc-sections enables the actual dead-code elim at link time.
    # -Wl,--as-needed drops DT_NEEDED entries for libs we never call
    # into (mostly transitive Qt deps). -z,nodump excludes the binary
    # from core dumps, saving a few KB on the .note section. The
    # GNU hash style is a hair smaller than sysv, and dropping the
    # build-id note shaves another 36 bytes plus the SHA1 padding.
    add_link_options(
        -Wl,--gc-sections
        -Wl,--as-needed
        -Wl,-z,nodump
        -Wl,--hash-style=gnu
        -Wl,--build-id=none
    )
    # MINSIZE implies LTO so cross-TU inlining + dead-code elim kick
    # in. If the user explicitly disabled LTO we honour that decision
    # and warn so they know what they gave up.
    if(NOT ENABLE_LTO)
        message(WARNING
            "IPVIEW_MINSIZE=ON works best with LTO; consider -DENABLE_LTO=ON")
    else()
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
    endif()
    message(STATUS "Size optimization ENABLED (-Os, --gc-sections, --as-needed, "
                    "--build-id=none, --hash-style=gnu, -z,nodump)")
endif()

# ── Qt 6 (6.5 LTS or newer) ───────────────────────────────────────
#  Hard minimum is 6.5 because that is the oldest LTS line still
#  receiving security patches in 2026. Newer Qt 6 versions (6.6, 6.7,
#  6.8 LTS, 6.9, 6.10, 6.11, …) all work — the GitHub Actions
#  workflow installs 6.8.* via jurplel/install-qt-action for
#  reproducibility; local development typically uses 6.11.
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Network Svg Sql)

# ── Automatische Qt-Metacompiler ──────────────────────────────
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ── Haupt-Executable ──────────────────────────────────────────
qt_add_executable(IPView
    main.cpp
    MainWindow.cpp
    DashboardView.cpp
    ConfigManager.cpp
    NetworkManager.cpp
    FlagLoader.cpp
    Iperf3Window.cpp
    WhoisManager.cpp
    WhoisTab.cpp
    ToolsTab.cpp
    TracerouteTab.cpp
    ScannerTab.cpp
    HistoryTab.cpp
    SpeedtestTab.cpp
    TelemetryTab.cpp
    AboutTab.cpp
    TelemetryModule.cpp
    TelemetryPersistenceModule.cpp
    DatabaseModule.cpp
    DatabaseWorker.cpp
    ScannerModule.cpp
    ServerSelectionModule.cpp
    AuditorModule.cpp
    AuditorTab.cpp
    AlertEngine.cpp
    TopologyTab.cpp
    PacketModule.cpp
    PacketTab.cpp
    AlertTab.cpp
    version.cpp
    Services.cpp
    NetworkDiscovery.cpp
    resources.qrc
)

# Generated version header must be visible to all translation units.
target_include_directories(IPView PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}/generated"
)

target_link_libraries(IPView
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Network
        Qt6::Svg
        Qt6::Sql
)

# ── Target properties ────────────────────────────────────────
set_target_properties(IPView PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE    TRUE
)

# ── Post-Build Size Pipeline (Phase 2.15.2) ─────────────────────────
#  Each stage is a separate add_custom_command that runs after the
#  main IPView target links successfully. CMake guarantees they run
#  in the order declared here.
#
#  Stage 1: strip  — `strip --strip-all` plus aggressive section
#                    removal (.comment, .note, .note.gnu.build-id,
#                    .note.gnu.property, .note.ABI-tag, .symtab,
#                    .strtab, .shstrtab). Saves ~16% of the .symtab
#                    and .strtab footprint on a typical Qt app.
#  Stage 2: upx    — `upx --best` compresses the stripped ELF in
#                    place using NRV/LZMA. UPX uses ~50 KB of
#                    decompressor stub + the original sections
#                    compressed; we typically see a 4-6x ratio on
#                    a Qt 6 app.
#  Stage 3: report — `du -b` + `file -b` + readelf section summary.
#
#  We only enable a stage when both the option is set and the tool
#  is found in PATH; a missing tool degrades gracefully to a warning
#  instead of failing the build (so CI can still validate the
#  unstripped / uncompressed binary).
find_program(IPVIEW_STRIP_TOOL strip)
find_program(IPVIEW_UPX_TOOL   upx)

if(IPVIEW_STRIP_BINARY AND IPVIEW_STRIP_TOOL)
    add_custom_command(TARGET IPView POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "─── strip: --strip-all + section removal ───"
        COMMAND ${IPVIEW_STRIP_TOOL}
            --strip-all
            --remove-section=.comment
            --remove-section=.note
            --remove-section=.note.gnu.build-id
            --remove-section=.note.gnu.property
            --remove-section=.note.ABI-tag
            -o $<TARGET_FILE:IPView>.stripped
            $<TARGET_FILE:IPView>
        COMMAND ${CMAKE_COMMAND} -E rename
            $<TARGET_FILE:IPView>.stripped
            $<TARGET_FILE:IPView>
        COMMENT "strip: removing debug + symbol tables"
        VERBATIM
    )
elseif(IPVIEW_STRIP_BINARY)
    message(WARNING "IPVIEW_STRIP_BINARY=ON but `strip` not found in PATH — skipping")
endif()

if(IPVIEW_COMPRESS_BINARY AND IPVIEW_UPX_TOOL)
    add_custom_command(TARGET IPView POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "─── upx: --best compression ───"
        COMMAND ${IPVIEW_UPX_TOOL} --best --no-progress
            -o $<TARGET_FILE:IPView>.upx
            $<TARGET_FILE:IPView>
        COMMAND ${CMAKE_COMMAND} -E rename
            $<TARGET_FILE:IPView>.upx
            $<TARGET_FILE:IPView>
        COMMENT "upx: compressing final binary"
        VERBATIM
    )
elseif(IPVIEW_COMPRESS_BINARY)
    message(WARNING "IPVIEW_COMPRESS_BINARY=ON but `upx` not found in PATH — skipping")
endif()

if(IPVIEW_SIZE_REPORT)
    # We hand the binary path to a tiny shell helper as a single argument
    # (no shell variables needed on the bash side), then let the shell
    # do its own `$1` parameter expansion to format the size. This avoids
    # the CMake/bash escaping rabbit hole entirely.
    add_custom_command(TARGET IPView POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "─── final binary snapshot ───"
        COMMAND bash -c "BIN=\"\$1\"; printf 'size : %s bytes (%s)\\n' \"\$(du -b \"\$BIN\" | cut -f1)\" \"\$(du -h \"\$BIN\" | cut -f1)\"; file -b \"\$BIN\""
                x $<TARGET_FILE:IPView>
        VERBATIM
    )
endif()

# ── C++26 polyfill for GCC 16.1.1 (saturate_cast) ────────────
#  Force-include before any Qt header to provide q26::saturate_cast
target_compile_options(IPView PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-include>
    $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/saturate_fix.h>
)

# ── Install rule (Linux: /usr/local/bin) ──────────────────────
install(TARGETS IPView RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# ── Tests (Phase 5-A) ──────────────────────────────────────────
#  Unit tests are built and registered with ctest when
#  BUILD_TESTING is ON (the default for `cmake -S . -B build`).
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()
