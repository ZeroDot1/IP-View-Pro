// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.15.4 — Format.hpp
//  Central compile-time string format constants. All values are
//  `inline constexpr std::string_view` so the linker dedupes the
//  storage and the compiler folds .arg() chains at compile time.
//
//  C++26 — `inline constexpr std::string_view` literal.
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#ifndef IPVIEW_FORMAT_HPP
#define IPVIEW_FORMAT_HPP

#include <string_view>

namespace IPView::Format {

// ─── Date / time ────────────────────────────────────────────────────────────
//
// These three format strings are used by QDateTime::toString() across
// the dashboard, history, alert and tray views. Qt's QDateTime
// recognises the same tokens as strftime (hh, mm, ss, yyyy, MM, dd).

/// Wall-clock time without date — e.g. "23:14:09". Used in
/// live "last refreshed" stamps and the tray tooltip.
inline constexpr std::string_view TIME_HMS = "hh:mm:ss";

/// Date + time to second precision — e.g. "2026-06-04 23:14:09".
/// Used in the persistent history table and the export-to-CSV file.
inline constexpr std::string_view DATETIME_SEC = "yyyy-MM-dd hh:mm:ss";

// ─── String styling ─────────────────────────────────────────────────────────

/// Generic "color: <color>;" QSS fragment. Used by inline label
/// colour overrides where the widget cannot be matched by a
/// global selector (e.g. labels inside a QTableWidget cell).
inline constexpr std::string_view QSS_COLOR_FMT = "color: %1;";

} // namespace IPView::Format

#endif // IPVIEW_FORMAT_HPP
