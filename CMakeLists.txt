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
    VERSION      2.9.1
    DESCRIPTION  "Professional IP Monitoring & Network Analysis Tool"
    LANGUAGES    CXX
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
    add_compile_definitions(_FORTIFY_SOURCE=3)
endif()

# Mit Sanitizern bauen: cmake -DSANITIZE=ON ..
option(SANITIZE "Build with AddressSanitizer + UBSan" OFF)
if(SANITIZE)
    add_compile_options(-fsanitize=address,undefined -g -O1)
    add_link_options(-fsanitize=address,undefined)
    message(STATUS "Sanitizers ENABLED (Address + UndefinedBehavior)")
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
    resources.qrc
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
