# IPView — Professional IP Monitoring & Network Analysis Tool
# Requires: Qt 6.11+, CMake 3.28+, C++26 compiler (GCC 14+ / Clang 18+)
#
# ════════════════════════════════════════════════════════════════
#  C++26 (ISO/IEC 14882:2026) | Qt 6.11 (March 2026)
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
    VERSION      2.15.0
    DESCRIPTION  "Professional IP Monitoring & Network Analysis Tool"
    LANGUAGES    CXX
)

# ── Version metadata (single source of truth) ────────────────────────
#  version.hpp is generated from version.hpp.in so main.cpp can read
#  the current version from a compile-time constant. Keeping the
#  canonical value in CMake means CMakeLists.txt, the about dialog,
#  and the runtime metadata are always in sync.
set(IPVIEW_VERSION_MAJOR  2)
set(IPVIEW_VERSION_MINOR  15)
set(IPVIEW_VERSION_PATCH  0)
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
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
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

# LTO ist standardmäßig an, lässt sich per -DENABLE_LTO=OFF deaktivieren.
option(ENABLE_LTO "Enable link-time optimization for Release builds" ON)
if(ENABLE_LTO AND CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
    message(STATUS "LTO enabled for Release build")
endif()

# Test-Builds (Phase 5) sind standardmäßig aus.
option(BUILD_TESTING "Build the unit / integration test targets" OFF)
if(BUILD_TESTING)
    enable_testing()
    message(STATUS "Test target enabled")
endif()

# ── Qt 6.11 (latest release: March 2026) ──────────────────────
find_package(Qt6 6.11 REQUIRED COMPONENTS Core Widgets Network Svg Sql)

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

# ── Ziel-Eigenschaften ────────────────────────────────────────
set_target_properties(IPView PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE    TRUE
)

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
