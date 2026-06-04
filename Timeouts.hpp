// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.15.5 — Timeouts.hpp
//  Centralized compile-time constants for all timeouts, intervals, and
//  duration knobs used across the project. Magic numbers in call sites
//  are replaced with named constants so tuning is a one-line change.
//
//  All values are std::chrono types (or std::size_t where appropriate) and
//  are exposed as `inline constexpr` so the linker dedupes and the
//  compiler folds arithmetic at compile time.
//
//  Naming convention:
//    *Ms / *Sec / *Min / *Hour — std::chrono durations
//    *_COUNT                  — plain size_t counts
//    *_BYTES                  — size_t byte counts
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#ifndef IPVIEW_TIMEOUTS_HPP
#define IPVIEW_TIMEOUTS_HPP

#include <chrono>
#include <cstddef>

namespace IPView::Timeouts {

// ── Network I/O ───────────────────────────────────────────────────────────
inline constexpr std::chrono::milliseconds HTTP_DEFAULT       {10'000};
inline constexpr std::chrono::milliseconds HTTP_TRANSFER     {15'000};
inline constexpr std::chrono::milliseconds HTTP_FALLBACK     {10'000};
inline constexpr std::chrono::milliseconds TCP_CONNECT        {500};
inline constexpr std::chrono::milliseconds TCP_DONE           {200};
inline constexpr std::chrono::milliseconds SOCKET_READY      {200};

// ── Inter-instance coordination (main.cpp) ────────────────────────────────
inline constexpr std::chrono::milliseconds INSTANCE_CONNECT   {500};
inline constexpr std::chrono::milliseconds INSTANCE_FLUSH     {300};
inline constexpr std::chrono::milliseconds INSTANCE_READ      {200};

// ── Polling / refresh intervals ───────────────────────────────────────────
inline constexpr std::chrono::milliseconds POLL_PACKET_MIN {1'000};
inline constexpr std::chrono::milliseconds POLL_PACKET_DEFAULT {5'000};
inline constexpr std::chrono::milliseconds POLL_TELEMETRY_DEFAULT {2'000};
inline constexpr std::chrono::milliseconds POLL_TELEMETRY_MIN     {500};
inline constexpr std::chrono::milliseconds POLL_TELEMETRY_MAX   {10'000};
inline constexpr std::chrono::milliseconds POLL_ALERT         {3'000};

// ── Process teardown waits ────────────────────────────────────────────────
inline constexpr std::chrono::milliseconds PROCESS_QUIT      {2'000};
inline constexpr std::chrono::milliseconds PROCESS_QUIT_FAST {1'000};
inline constexpr std::chrono::milliseconds PROCESS_QUIT_SLOW {5'000};
inline constexpr std::chrono::milliseconds PROCESS_TRACEROUTE {500};

// ── Database worker thread ─────────────────────────────────────────────────
inline constexpr std::chrono::milliseconds DB_WORKER_QUIT    {3'000};
inline constexpr std::chrono::milliseconds DB_WORKER_TICK    {1'000};

// ── Notification / tray ───────────────────────────────────────────────────
inline constexpr std::chrono::milliseconds TRAY_TOOLTIP     {2'000};

// ── Alert engine defaults ──────────────────────────────────────────────────
inline constexpr std::chrono::seconds      COOLDOWN_DEFAULT   {300};
inline constexpr std::size_t               ALERT_ERROR_WARN   {100};
inline constexpr std::size_t               ALERT_ERROR_CRIT  {1'000};
inline constexpr double                    ALERT_BW_WARN_MBPS  {100.0};
inline constexpr double                    ALERT_BW_CRIT_MBPS  {500.0};

// ── QProcess argument buffer hints ────────────────────────────────────────
inline constexpr std::size_t               IPERF3_STATS_PERIOD_MS {1'000};
inline constexpr std::size_t               SCANNER_BATCH_RESERVE  {100};
inline constexpr std::size_t               SERVERS_RESERVE        {128};

} // namespace IPView::Timeouts

#endif // IPVIEW_TIMEOUTS_HPP
