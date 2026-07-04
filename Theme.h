// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.15.5 — Theme.h
//  Central design-token system for Qt Style Sheets (QSS).
//  All colors, spacings, radii, shadows, animations, borders, carets,
//  data-viz palettes, typography, elevation, and opacity live here.
//
//  Palette: Teal Amber OLED (default), Teal Amber Light, High Contrast
//    Primary (Teal):  #00BCD4 — cool, professional, high-contrast on OLED black
//    Accent  (Amber): #FFB300 — warm highlight, badges, interactive focus
//    Base:            #000000 — true OLED black (zero power on OLED displays)
//
//  Three theme modes (selected at startup or via the View menu in v2.15.5):
//    1. Dark           — the original OLED-optimised Teal Amber
//    2. Light          — same Teal Amber hues on a light surface
//    3. High Contrast  — pure black/white, AAA accessibility (WCAG 2.1)
//
//  OLED Support: All Dark-mode backgrounds use pure black (#000000)
//  wherever possible. Light mode is intended for daylight / projector
//  use; High Contrast is intended for low-vision / accessibility use.
//
//  C++26 inline constexpr — resolved at compile time.
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#ifndef THEME_H
#define THEME_H

#include <QString>
#include <QStringBuilder>   // Efficient string concatenation
#include <QStringView>      // QStringView for colorStyle()
#include <QTableWidget>     // For applyTableStyle()
#include <QHeaderView>      // QHeaderView for verticalHeader() call
#include <QAbstractItemView>// QAbstractItemView::SelectionBehavior / EditTrigger

#include <array>            // C++26: constexpr std::array
#include <chrono>           // std::chrono::milliseconds
#include <cstddef>          // std::size_t
#include <string_view>      // std::string_view for constexpr token tables

// ═══════════════════════════════════════════════════════════════════════════════
//  DARK THEME — COLOR PALETTE
//  The default. Professional Amber on premium dark zinc.
// ═══════════════════════════════════════════════════════════════════════════════

// ── Base colors (Premium zinc hierarchy) ─────────────────────────────────────
inline constexpr auto C_BG          = "#09090B";   // Zinc-950 — main background
inline constexpr auto C_BG_ELEVATED = "#18181B";   // Zinc-900 — elevated surfaces
inline constexpr auto C_BG_SUNKEN   = "#030303";   // Sunken areas
inline constexpr auto C_BG_HOVER    = "#27272A";   // Hover overlay

inline constexpr auto C_SURFACE     = "#18181B";   // Default card/pane surface
inline constexpr auto C_SURFACE_HVR = "#27272A";   // Surface hover state
inline constexpr auto C_SURFACE_ACT = "#3F3F46";   // Surface active/pressed state

inline constexpr auto C_BORDER      = "#27272A";   // Zinc-800 border
inline constexpr auto C_BORDER_HVR  = "#D97706";   // Amber-600 border on hover
inline constexpr auto C_BORDER_FOC  = "#F59E0B";   // Amber focus ring

// ── Text colors ───────────────────────────────────────────────────────────────
inline constexpr auto C_TEXT        = "#FAFAFA";   // Primary text — near-white
inline constexpr auto C_TEXT_SEC    = "#D4D4D8";   // Secondary text — zinc-300
inline constexpr auto C_TEXT_DIM    = "#A1A1AA";   // Dimmed label text — zinc-400
inline constexpr auto C_TEXT_MUTED  = "#71717A";   // Muted/placeholder text — zinc-500
inline constexpr auto C_TEXT_INV    = "#000000";   // Inverted text (for amber backgrounds)

// ── Amber primary ────────────────────────────────────────────────────────────
inline constexpr auto C_PRIMARY     = "#F59E0B";   // Amber primary — focus, links, progress
inline constexpr auto C_PRIMARY_HVR = "#FBBF24";   // Amber hover
inline constexpr auto C_PRIMARY_ACT = "#D97706";   // Amber active/pressed

// ── Amber accent ─────────────────────────────────────────────────────────────
inline constexpr auto C_ACCENT      = "#F59E0B";   // Amber accent — badges, selected tabs
inline constexpr auto C_ACCENT_HVR  = "#FBBF24";   // Amber hover
inline constexpr auto C_ACCENT_ACT  = "#D97706";   // Amber active/pressed

// ── Semantic colors ───────────────────────────────────────────────────────────
inline constexpr auto C_SUCCESS     = "#10B981";   // Emerald-500 — positive/online states
inline constexpr auto C_SUCCESS_HVR = "#34D399";   // Success hover
inline constexpr auto C_WARNING     = "#F59E0B";   // Amber — warnings (same as accent)
inline constexpr auto C_ERROR       = "#EF4444";   // Red — errors, critical alerts
inline constexpr auto C_ERROR_HVR   = "#F87171";   // Error hover
inline constexpr auto C_INFO        = "#F59E0B";   // Amber — informational
inline constexpr auto C_CRITICAL    = "#DC2626";   // Deep red — critical alerts
inline constexpr auto C_MUTED       = "#27272A";   // Disabled / very low contrast

// ── Severity scale (alert engine) ────────────────────────────────────────────
inline constexpr auto C_SEVERITY_INFO     = "#F59E0B";   // Informational
inline constexpr auto C_SEVERITY_WARNING  = "#F59E0B";   // Warning
inline constexpr auto C_SEVERITY_CRITICAL = "#EF4444";   // Critical

// ── Status dot (used in tray, dashboard, alerts) ─────────────────────────────
inline constexpr auto C_DOT_ONLINE  = "#10B981";   // Green dot — connected
inline constexpr auto C_DOT_OFFLINE = "#EF4444";   // Red dot — disconnected
inline constexpr auto C_DOT_PENDING = "#F59E0B";   // Amber dot — refreshing
inline constexpr auto C_DOT_IDLE    = "#71717A";   // Grey dot — idle

// ── Data-visualization categorical palette (10-colour) ───────────────────────
//  Used by NetworkDiscovery, PortScanner, Telemetry graphs and any other
//  widget that needs to distinguish many series in a single visualisation.
//  The colours are picked for distinct hue + perceptual lightness on the
//  OLED-black background; they remain distinguishable in colour-blindness
//  simulations (deuteranopia + protanopia).
inline constexpr auto C_CHART_0  = "#F59E0B";   // Amber
inline constexpr auto C_CHART_1  = "#3B82F6";   // Blue
inline constexpr auto C_CHART_2  = "#10B981";   // Emerald Green
inline constexpr auto C_CHART_3  = "#EF4444";   // Red
inline constexpr auto C_CHART_4  = "#8B5CF6";   // Purple
inline constexpr auto C_CHART_5  = "#EC4899";   // Pink
inline constexpr auto C_CHART_6  = "#06B6D4";   // Cyan
inline constexpr auto C_CHART_7  = "#F97316";   // Orange
inline constexpr auto C_CHART_8  = "#84CC16";   // Lime
inline constexpr auto C_CHART_9  = "#64748B";   // Slate-grey

// ── Connection-state palette (PacketModule) ──────────────────────────────────
//  The Connections tab displays the kernel TCP/UDP state for every entry;
//  each state gets a dedicated token so the colour follows the design
//  system instead of being a magic hex in PacketTab.cpp.
inline constexpr auto C_STATE_ESTABLISHED = "#10B981";  // Green — active
inline constexpr auto C_STATE_LISTEN     = "#F59E0B";  // Amber — listening
inline constexpr auto C_STATE_TIME_WAIT  = "#D97706";  // Amber — ageing
inline constexpr auto C_STATE_CLOSE_WAIT = "#EF4444";  // Red — closing
inline constexpr auto C_STATE_FIN_WAIT   = "#B45309";  // Amber-deep
inline constexpr auto C_STATE_CLOSED     = "#71717A";  // Grey — closed
inline constexpr auto C_STATE_SYN_SENT   = "#8B5CF6";  // Purple — opening
inline constexpr auto C_STATE_UDP        = "#06B6D4";  // Cyan — UDP (no state)

// ── Latency / quality gradient (Topology + Iperf3) ──────────────────────────
//  Five discrete buckets from "excellent" to "timeout". The exact cutoffs
//  live in the helper latencyColorMs(double ms).
inline constexpr auto C_LATENCY_EXCELLENT = "#10B981";  // <10 ms    — green
inline constexpr auto C_LATENCY_GOOD      = "#84CC16";  // <50 ms    — lime-green
inline constexpr auto C_LATENCY_FAIR      = "#F59E0B";  // <150 ms   — amber
inline constexpr auto C_LATENCY_POOR      = "#EF4444";  // >=150 ms  — red
inline constexpr auto C_LATENCY_TIMEOUT   = "#71717A";  // -1 / no reply — grey

// ── Form validation states ──────────────────────────────────────────────────
//  Used by inputStyle() variants. The plain `inputStyle()` is "neutral";
//  the warning / success / error variants are produced by the helpers
//  inputStyleWarning() / inputStyleSuccess() / inputStyleError().
inline constexpr auto C_VALID_BORDER    = C_BORDER;
inline constexpr auto C_VALID_BG        = C_BG_ELEVATED;
inline constexpr auto C_WARNING_BORDER  = "#D97706";   // Amber-700
inline constexpr auto C_SUCCESS_BORDER  = "#10B981";   // Green
inline constexpr auto C_ERROR_BORDER    = "#EF4444";   // Red
inline constexpr auto C_VALID_ICON_OK   = "#10B981";
inline constexpr auto C_VALID_ICON_WARN = "#F59E0B";
inline constexpr auto C_VALID_ICON_ERR  = "#EF4444";

// ── Caret / selection / focus outline (input widgets) ────────────────────────
inline constexpr auto C_CARET          = "#F59E0B";   // Amber caret in QLineEdit
inline constexpr auto C_SELECTION_BG   = "#FEF08A";   // Light yellow/amber selection
inline constexpr auto C_SELECTION_FG   = "#000000";   // Black text on amber selection
inline constexpr auto C_FOCUS_OUTLINE  = "#F59E0B";   // Amber focus outline (2 px)

// ═══════════════════════════════════════════════════════════════════════════════
//  LIGHT THEME — COLOR PALETTE
//  Same Amber hues, but on a light surface for daylight / projector use.
//  Only the surface / text / border tokens differ from the dark theme; the
//  primary (amber) and accent (amber) are kept identical so the brand stays
//  consistent across modes.
// ═══════════════════════════════════════════════════════════════════════════════

inline constexpr auto C_L_BG          = "#FAFAFA";   // Near-white — main background
inline constexpr auto C_L_BG_ELEVATED = "#FFFFFF";   // Pure white — cards
inline constexpr auto C_L_BG_SUNKEN   = "#F4F4F5";   // Light grey — code/output
inline constexpr auto C_L_BG_HOVER    = "#F4F4F5";   // Hover overlay
inline constexpr auto C_L_SURFACE     = "#FFFFFF";
inline constexpr auto C_L_SURFACE_HVR = "#F4F4F5";
inline constexpr auto C_L_SURFACE_ACT = "#E4E4E7";

inline constexpr auto C_L_BORDER      = "#E4E4E7";   // Subtle border on white
inline constexpr auto C_L_BORDER_HVR  = "#D97706";   // Amber-dark border on hover
inline constexpr auto C_L_BORDER_FOC  = "#F59E0B";   // Amber-dark focus ring

inline constexpr auto C_L_TEXT        = "#09090B";   // Near-black primary text
inline constexpr auto C_L_TEXT_SEC    = "#3F3F46";   // Dark grey
inline constexpr auto C_L_TEXT_DIM    = "#71717A";
inline constexpr auto C_L_TEXT_MUTED  = "#A1A1AA";
inline constexpr auto C_L_TEXT_INV    = "#FFFFFF";   // Inverted (for amber/teal fills)

inline constexpr auto C_L_MUTED       = "#E4E4E7";   // Disabled on light

inline constexpr auto C_L_CARET          = "#D97706";
inline constexpr auto C_L_SELECTION_BG   = "#FEF3C7";
inline constexpr auto C_L_SELECTION_FG   = "#78350F";
inline constexpr auto C_L_FOCUS_OUTLINE  = "#F59E0B";

// ── Light-theme accent / primary / status fallbacks ─────────────────────────
//  Same Amber hues as the Dark theme — the Light variant changes
//  only the surface / text / border, not the brand palette. These are
//  defined here (before appStyleSheetLight) so the .arg() chain inside
//  the function can refer to them by name.
inline constexpr auto C_L_ACCENT_FALLBACK = "#F59E0B";   // Same as C_ACCENT
inline constexpr auto C_L_ACCENT_HOVER   = "#FBBF24";   // Same as C_ACCENT_HVR
inline constexpr auto C_L_PRIMARY        = "#F59E0B";   // Same as C_PRIMARY
inline constexpr auto C_L_PRIMARY_HOVER  = "#FBBF24";   // Same as C_PRIMARY_HVR
inline constexpr auto C_L_ERROR          = "#EF4444";   // Same as C_ERROR
inline constexpr auto C_L_SUCCESS        = "#10B981";   // Same as C_SUCCESS

// ═══════════════════════════════════════════════════════════════════════════════
//  HIGH-CONTRAST THEME — COLOR PALETTE
//  Pure black on white (or white on black) with AAA contrast ratios.
//  Intended for low-vision / accessibility users and projector demos.
//  All decorative colours collapse to the foreground / background pair.
// ═══════════════════════════════════════════════════════════════════════════════

inline constexpr auto C_HC_BG          = "#000000";   // Pure black
inline constexpr auto C_HC_BG_ELEVATED = "#000000";
inline constexpr auto C_HC_BG_SUNKEN   = "#000000";
inline constexpr auto C_HC_BG_HOVER    = "#1A1A1A";
inline constexpr auto C_HC_SURFACE     = "#000000";
inline constexpr auto C_HC_SURFACE_HVR = "#1A1A1A";
inline constexpr auto C_HC_SURFACE_ACT = "#2A2A2A";

inline constexpr auto C_HC_BORDER      = "#FFFFFF";   // White border on black
inline constexpr auto C_HC_BORDER_HVR  = "#FFFF00";   // Yellow on hover
inline constexpr auto C_HC_BORDER_FOC  = "#00FFFF";   // Cyan focus

inline constexpr auto C_HC_TEXT        = "#FFFFFF";   // Pure white
inline constexpr auto C_HC_TEXT_SEC    = "#FFFFFF";
inline constexpr auto C_HC_TEXT_DIM    = "#E0E0E0";
inline constexpr auto C_HC_TEXT_MUTED  = "#C0C0C0";
inline constexpr auto C_HC_TEXT_INV    = "#000000";

inline constexpr auto C_HC_PRIMARY     = "#00FFFF";   // Cyan in HC mode
inline constexpr auto C_HC_ACCENT      = "#FFFF00";   // Yellow in HC mode
inline constexpr auto C_HC_SUCCESS     = "#00FF00";   // Pure green
inline constexpr auto C_HC_WARNING     = "#FFFF00";   // Pure yellow
inline constexpr auto C_HC_ERROR       = "#FF0000";   // Pure red
inline constexpr auto C_HC_CRITICAL    = "#FF0000";
inline constexpr auto C_HC_MUTED       = "#808080";

inline constexpr auto C_HC_CARET         = "#00FFFF";
inline constexpr auto C_HC_SELECTION_BG  = "#FFFF00";
inline constexpr auto C_HC_SELECTION_FG  = "#000000";
inline constexpr auto C_HC_FOCUS_OUTLINE = "#00FFFF";

// ═══════════════════════════════════════════════════════════════════════════════
//  SHARED DESIGN TOKENS  (mode-independent)
//  Spacing, radii, sizing, typography, animation, shadows, z-index,
//  opacity, border widths. These are identical across all three themes
//  because they are dimensions, not colours.
// ═══════════════════════════════════════════════════════════════════════════════

// ── Spacing scale (8 px base, with 4-px half-step) ────────────────────────────
//  Tabs: compact — all 12 tabs fit without scrolling
inline constexpr auto PADDING_NONE  = "0px";
inline constexpr auto PADDING_XS    = "2px";
inline constexpr auto PADDING_S     = "4px";
inline constexpr auto PADDING_M     = "8px";
inline constexpr auto PADDING_L     = "10px";
inline constexpr auto PADDING_XL    = "12px";
inline constexpr auto PADDING_2XL   = "16px";
inline constexpr auto PADDING_3XL   = "20px";
inline constexpr auto PADDING_4XL   = "24px";
inline constexpr auto PADDING_5XL   = "32px";

inline constexpr auto PADDING_TAB   = "6px 12px";
inline constexpr auto PADDING_TAB_L = "6px 18px";   // Optional wider tab variant
inline constexpr auto PADDING_BTN   = "8px 20px";
inline constexpr auto PADDING_BTN_S = "6px 14px";
inline constexpr auto PADDING_INP   = "8px 12px";
inline constexpr auto PADDING_CRD   = "20px";

// ── Radii (border-radius) ────────────────────────────────────────────────────
inline constexpr auto RADIUS_NONE  = "0px";
inline constexpr auto RADIUS_XS    = "2px";
inline constexpr auto RADIUS_SM    = "4px";
inline constexpr auto RADIUS_MD    = "6px";
inline constexpr auto RADIUS_LG    = "8px";
inline constexpr auto RADIUS_XL    = "12px";
inline constexpr auto RADIUS_2XL   = "14px";
inline constexpr auto RADIUS_PILL  = "9999px";     // For pill-shaped badges/buttons

// ── Sizing tokens (fixed heights, widths, icon sizes) ────────────────────────
inline constexpr auto ICON_SZ_XS   = "10px";
inline constexpr auto ICON_SZ_SM   = "14px";
inline constexpr auto ICON_SZ_MD   = "16px";
inline constexpr auto ICON_SZ_LG   = "20px";
inline constexpr auto ICON_SZ_XL   = "24px";
inline constexpr auto ICON_SZ_2XL  = "32px";
inline constexpr auto ICON_SZ_3XL  = "48px";       // About-tab feature icons

inline constexpr auto TRAY_ICON_SZ = 64;           // System tray icon (px)
inline constexpr auto FLAG_W       = 64;           // Dashboard flag (px)
inline constexpr auto FLAG_H       = 42;
inline constexpr auto BTN_MIN_W    = 80;           // Min width for dialog buttons
inline constexpr auto BTN_MIN_H    = 32;           // Min height for primary buttons
inline constexpr auto ROW_H_TABLE  = 28;           // Default table row height
inline constexpr auto ROW_H_COMPACT= 22;           // Dense data table rows
inline constexpr auto TOOLBAR_H    = 40;           // QToolBar height
inline constexpr auto STATUSBAR_H  = 24;           // QStatusBar height

// ── Typography scale ─────────────────────────────────────────────────────────
inline constexpr auto FONT_FAMILY      = "'Segoe UI', 'Noto Sans', 'Ubuntu', sans-serif";
inline constexpr auto FONT_MONOSPACE   = "'JetBrains Mono', 'Cascadia Code', 'Consolas', monospace";
inline constexpr auto FONT_HEADING     = "'Segoe UI Semibold', 'Segoe UI', sans-serif";

inline constexpr auto FONT_SIZE_XS     = "10px";
inline constexpr auto FONT_SIZE_SM     = "11px";
inline constexpr auto FONT_SIZE_BASE   = "12px";
inline constexpr auto FONT_SIZE_MD     = "13px";
inline constexpr auto FONT_SIZE_LG     = "14px";
inline constexpr auto FONT_SIZE_XL     = "16px";
inline constexpr auto FONT_SIZE_2XL    = "18px";
inline constexpr auto FONT_SIZE_3XL    = "20px";
inline constexpr auto FONT_SIZE_4XL    = "22px";
inline constexpr auto FONT_SIZE_DISPLAY= "26px";    // Dashboard IP card

inline constexpr auto FONT_WEIGHT_NORMAL  = "400";
inline constexpr auto FONT_WEIGHT_MEDIUM  = "500";
inline constexpr auto FONT_WEIGHT_BOLD    = "700";

inline constexpr auto LINE_HEIGHT_TIGHT   = "1.1";
inline constexpr auto LINE_HEIGHT_NORMAL  = "1.4";
inline constexpr auto LINE_HEIGHT_RELAXED = "1.6";

// ── Animation durations (C++ side; CSS still uses ms) ────────────────────────
inline constexpr std::chrono::milliseconds ANIM_HOVER    {120};
inline constexpr std::chrono::milliseconds ANIM_PRESS    {80};
inline constexpr std::chrono::milliseconds ANIM_FADE     {200};
inline constexpr std::chrono::milliseconds ANIM_SLIDE    {240};
inline constexpr std::chrono::milliseconds ANIM_PULSE    {1500};
inline constexpr std::chrono::milliseconds ANIM_TOAST    {300};
inline constexpr std::chrono::milliseconds ANIM_TRAYTIP  {2000};

// ── Easing functions (CSS) ───────────────────────────────────────────────────
inline constexpr auto EASE_OUT       = "cubic-bezier(0.16, 1, 0.3, 1)";
inline constexpr auto EASE_IN        = "cubic-bezier(0.7, 0, 0.84, 0)";
inline constexpr auto EASE_IN_OUT    = "cubic-bezier(0.65, 0, 0.35, 1)";
inline constexpr auto EASE_LINEAR    = "linear";

// ── Shadow tokens (used for hover/elevation effects) ─────────────────────────
//  CSS box-shadow strings; reusable across widgets.
inline constexpr auto SHADOW_NONE   = "none";
inline constexpr auto SHADOW_XS     = "0 1px 1px rgba(0, 0, 0, 0.3)";
inline constexpr auto SHADOW_SM     = "0 1px 2px rgba(0, 0, 0, 0.4)";
inline constexpr auto SHADOW_MD     = "0 2px 6px rgba(0, 0, 0, 0.5)";
inline constexpr auto SHADOW_LG     = "0 4px 12px rgba(0, 0, 0, 0.6)";
inline constexpr auto SHADOW_XL     = "0 8px 24px rgba(0, 0, 0, 0.7)";
inline constexpr auto SHADOW_FOCUS  = "0 0 0 2px rgba(0, 188, 212, 0.4)";   // Teal focus ring
inline constexpr auto SHADOW_GLOW_T = "0 0 12px rgba(0, 188, 212, 0.5)";     // Teal glow
inline constexpr auto SHADOW_GLOW_A = "0 0 12px rgba(255, 179, 0, 0.5)";     // Amber glow

// ── Z-index / elevation tokens (for QGraphicsView, overlay layers) ───────────
inline constexpr int Z_BASE         = 0;
inline constexpr int Z_RAISED       = 10;
inline constexpr int Z_DROPDOWN     = 50;
inline constexpr int Z_STICKY       = 75;
inline constexpr int Z_TOOLTIP      = 100;
inline constexpr int Z_TOAST        = 200;
inline constexpr int Z_OVERLAY      = 500;
inline constexpr int Z_MODAL        = 1000;
inline constexpr int Z_CRITICAL     = 9999;        // Always-on-top crash dialogs

// ── Opacity / alpha tokens (for disabled, hover overlays) ────────────────────
inline constexpr auto OPACITY_DISABLED = "0.5";
inline constexpr auto OPACITY_HINT     = "0.7";
inline constexpr auto OPACITY_HOVER    = "0.85";
inline constexpr auto OPACITY_PRESSED  = "0.95";
inline constexpr auto OPACITY_FULL     = "1.0";

// ── Border widths ────────────────────────────────────────────────────────────
inline constexpr auto BORDER_NONE   = "0px";
inline constexpr auto BORDER_HAIR   = "0.5px";
inline constexpr auto BORDER_THIN   = "1px";
inline constexpr auto BORDER_MED    = "2px";
inline constexpr auto BORDER_THICK  = "3px";
inline constexpr auto BORDER_HEAVY  = "4px";

// ── Focus outline (AAA accessibility: 2 px solid, 2 px offset) ──────────────
inline constexpr auto FOCUS_WIDTH   = "2px";
inline constexpr auto FOCUS_OFFSET  = "2px";

// ── Breakpoints (responsive layout — for future use) ─────────────────────────
//  Width cutoffs for the layout to switch between compact / default / wide.
inline constexpr int BREAKPOINT_COMPACT  = 480;
inline constexpr int BREAKPOINT_DEFAULT  = 900;
inline constexpr int BREAKPOINT_WIDE     = 1280;
inline constexpr int BREAKPOINT_XWIDE    = 1920;

// ═══════════════════════════════════════════════════════════════════════════════
//  THEME MODE — the one place to ask "which palette should I render?"
// ═══════════════════════════════════════════════════════════════════════════════

/// Enumeration of the available themes. Stored in the config file and
/// applied at startup by main.cpp. Adding a new mode is a 4-step change:
///   1. Add the enum value here.
///   2. Add a *_TOKENS list to one of the token tables below.
///   3. Add an appStyleSheet*() function that uses the new tokens.
///   4. Extend applyTheme() to dispatch to the new function.
enum class ThemeMode : int {
    Dark          = 0,    // OLED-optimised default
    Light         = 1,    // Daylight / projector
    HighContrast  = 2,    // AAA accessibility
};

/// Return the human-readable name of a theme mode. Used by the About
/// tab and any UI surface that wants to display the current theme.
[[nodiscard]] inline constexpr std::string_view themeModeName(ThemeMode m) noexcept
{
    switch (m) {
        case ThemeMode::Dark:         return "Dark (OLED)";
        case ThemeMode::Light:        return "Light";
        case ThemeMode::HighContrast: return "High Contrast";
    }
    return "Dark (OLED)";
}

// ═══════════════════════════════════════════════════════════════════════════════
//  CONSTEXPR TOKEN TABLES
//  Type-safe iteration over the design tokens. C++26 std::array
//  guarantees the storage is in .rodata and indexed at compile time.
//  Useful for tooling, palette dumps, and consistency checks
//  (e.g. CI lint that catches hard-coded hex).
// ═══════════════════════════════════════════════════════════════════════════════

namespace IPView::Theme {

// All known Dark-theme color tokens. The Light and High-Contrast tokens
// live in dedicated arrays further down so a CI lint can pick the right
// table for the active theme.
inline constexpr auto COLOR_TOKENS_DARK = std::to_array<std::string_view>({
    "C_BG", "C_BG_ELEVATED", "C_BG_SUNKEN", "C_BG_HOVER",
    "C_SURFACE", "C_SURFACE_HVR", "C_SURFACE_ACT",
    "C_BORDER", "C_BORDER_HVR", "C_BORDER_FOC",
    "C_TEXT", "C_TEXT_SEC", "C_TEXT_DIM", "C_TEXT_MUTED", "C_TEXT_INV",
    "C_PRIMARY", "C_PRIMARY_HVR", "C_PRIMARY_ACT",
    "C_ACCENT", "C_ACCENT_HVR", "C_ACCENT_ACT",
    "C_SUCCESS", "C_SUCCESS_HVR", "C_WARNING", "C_ERROR", "C_ERROR_HVR",
    "C_INFO", "C_CRITICAL", "C_MUTED",
    "C_SEVERITY_INFO", "C_SEVERITY_WARNING", "C_SEVERITY_CRITICAL",
    "C_DOT_ONLINE", "C_DOT_OFFLINE", "C_DOT_PENDING", "C_DOT_IDLE",
    "C_CHART_0", "C_CHART_1", "C_CHART_2", "C_CHART_3", "C_CHART_4",
    "C_CHART_5", "C_CHART_6", "C_CHART_7", "C_CHART_8", "C_CHART_9",
    "C_STATE_ESTABLISHED", "C_STATE_LISTEN", "C_STATE_TIME_WAIT",
    "C_STATE_CLOSE_WAIT", "C_STATE_FIN_WAIT", "C_STATE_CLOSED",
    "C_STATE_SYN_SENT", "C_STATE_UDP",
    "C_LATENCY_EXCELLENT", "C_LATENCY_GOOD", "C_LATENCY_FAIR",
    "C_LATENCY_POOR", "C_LATENCY_TIMEOUT",
    "C_VALID_BORDER", "C_VALID_BG", "C_WARNING_BORDER", "C_SUCCESS_BORDER",
    "C_ERROR_BORDER", "C_VALID_ICON_OK", "C_VALID_ICON_WARN", "C_VALID_ICON_ERR",
    "C_CARET", "C_SELECTION_BG", "C_SELECTION_FG", "C_FOCUS_OUTLINE",
});

inline constexpr auto COLOR_TOKENS_LIGHT = std::to_array<std::string_view>({
    "C_L_BG", "C_L_BG_ELEVATED", "C_L_BG_SUNKEN", "C_L_BG_HOVER",
    "C_L_SURFACE", "C_L_SURFACE_HVR", "C_L_SURFACE_ACT",
    "C_L_BORDER", "C_L_BORDER_HVR", "C_L_BORDER_FOC",
    "C_L_TEXT", "C_L_TEXT_SEC", "C_L_TEXT_DIM", "C_L_TEXT_MUTED", "C_L_TEXT_INV",
    "C_L_MUTED", "C_L_CARET", "C_L_SELECTION_BG", "C_L_SELECTION_FG", "C_L_FOCUS_OUTLINE",
});

inline constexpr auto COLOR_TOKENS_HC = std::to_array<std::string_view>({
    "C_HC_BG", "C_HC_BG_ELEVATED", "C_HC_BG_SUNKEN", "C_HC_BG_HOVER",
    "C_HC_SURFACE", "C_HC_SURFACE_HVR", "C_HC_SURFACE_ACT",
    "C_HC_BORDER", "C_HC_BORDER_HVR", "C_HC_BORDER_FOC",
    "C_HC_TEXT", "C_HC_TEXT_SEC", "C_HC_TEXT_DIM", "C_HC_TEXT_MUTED", "C_HC_TEXT_INV",
    "C_HC_PRIMARY", "C_HC_ACCENT", "C_HC_SUCCESS", "C_HC_WARNING",
    "C_HC_ERROR", "C_HC_CRITICAL", "C_HC_MUTED",
    "C_HC_CARET", "C_HC_SELECTION_BG", "C_HC_SELECTION_FG", "C_HC_FOCUS_OUTLINE",
});

inline constexpr auto SPACING_TOKENS = std::to_array<std::string_view>({
    "PADDING_NONE", "PADDING_XS", "PADDING_S", "PADDING_M", "PADDING_L",
    "PADDING_XL", "PADDING_2XL", "PADDING_3XL", "PADDING_4XL", "PADDING_5XL",
    "PADDING_TAB", "PADDING_TAB_L", "PADDING_BTN", "PADDING_BTN_S",
    "PADDING_INP", "PADDING_CRD",
});

inline constexpr auto RADIUS_TOKENS = std::to_array<std::string_view>({
    "RADIUS_NONE", "RADIUS_XS", "RADIUS_SM", "RADIUS_MD", "RADIUS_LG",
    "RADIUS_XL", "RADIUS_2XL", "RADIUS_PILL",
});

inline constexpr auto SIZING_TOKENS = std::to_array<std::string_view>({
    "ICON_SZ_XS", "ICON_SZ_SM", "ICON_SZ_MD", "ICON_SZ_LG",
    "ICON_SZ_XL", "ICON_SZ_2XL", "ICON_SZ_3XL",
    "TRAY_ICON_SZ", "FLAG_W", "FLAG_H", "BTN_MIN_W", "BTN_MIN_H",
    "ROW_H_TABLE", "ROW_H_COMPACT", "TOOLBAR_H", "STATUSBAR_H",
});

inline constexpr auto TYPOGRAPHY_TOKENS = std::to_array<std::string_view>({
    "FONT_FAMILY", "FONT_MONOSPACE", "FONT_HEADING",
    "FONT_SIZE_XS", "FONT_SIZE_SM", "FONT_SIZE_BASE", "FONT_SIZE_MD",
    "FONT_SIZE_LG", "FONT_SIZE_XL", "FONT_SIZE_2XL", "FONT_SIZE_3XL",
    "FONT_SIZE_4XL", "FONT_SIZE_DISPLAY",
    "FONT_WEIGHT_NORMAL", "FONT_WEIGHT_MEDIUM", "FONT_WEIGHT_BOLD",
    "LINE_HEIGHT_TIGHT", "LINE_HEIGHT_NORMAL", "LINE_HEIGHT_RELAXED",
});

inline constexpr auto ANIMATION_TOKENS = std::to_array<std::string_view>({
    "ANIM_HOVER", "ANIM_PRESS", "ANIM_FADE", "ANIM_SLIDE",
    "ANIM_PULSE", "ANIM_TOAST", "ANIM_TRAYTIP",
    "EASE_OUT", "EASE_IN", "EASE_IN_OUT", "EASE_LINEAR",
});

inline constexpr auto SHADOW_TOKENS = std::to_array<std::string_view>({
    "SHADOW_NONE", "SHADOW_XS", "SHADOW_SM", "SHADOW_MD", "SHADOW_LG",
    "SHADOW_XL", "SHADOW_FOCUS", "SHADOW_GLOW_T", "SHADOW_GLOW_A",
});

inline constexpr auto Z_INDEX_TOKENS = std::to_array<std::string_view>({
    "Z_BASE", "Z_RAISED", "Z_DROPDOWN", "Z_STICKY", "Z_TOOLTIP",
    "Z_TOAST", "Z_OVERLAY", "Z_MODAL", "Z_CRITICAL",
});

inline constexpr auto OPACITY_TOKENS = std::to_array<std::string_view>({
    "OPACITY_DISABLED", "OPACITY_HINT", "OPACITY_HOVER",
    "OPACITY_PRESSED", "OPACITY_FULL",
});

inline constexpr auto BORDER_TOKENS = std::to_array<std::string_view>({
    "BORDER_NONE", "BORDER_HAIR", "BORDER_THIN", "BORDER_MED",
    "BORDER_THICK", "BORDER_HEAVY",
    "FOCUS_WIDTH", "FOCUS_OFFSET",
});

inline constexpr auto BREAKPOINT_TOKENS = std::to_array<std::string_view>({
    "BREAKPOINT_COMPACT", "BREAKPOINT_DEFAULT",
    "BREAKPOINT_WIDE", "BREAKPOINT_XWIDE",
});

} // namespace IPView::Theme

// ═══════════════════════════════════════════════════════════════════════════════
//  GLOBAL APP STYLESHEETS
//  Three stylesheets — one per ThemeMode — applied once via applyTheme().
//  Every colour token is interpolated through QString::arg() so the
//  same hard-coded palette logic can be reused for the three modes.
// ═══════════════════════════════════════════════════════════════════════════════

/// Dark theme — OLED black, Teal Amber accents. The default theme.
[[nodiscard]]
inline QString appStyleSheetDark() noexcept
{
    return QStringLiteral(
        // ── Global ────────────────────────────────────────────────────
        "QWidget {"
        "  background-color: %1; color: %2;"
        "  font-family: 'Segoe UI', 'Noto Sans', 'Ubuntu', sans-serif;"
        "  font-size: 13px;"
        "}"
        // ── Tooltip ────────────────────────────────────────────────────
        "QToolTip {"
        "  background-color: %3; color: %2;"
        "  border: 1px solid %4; border-radius: %5;"
        "  padding: 6px 10px; font-size: 12px;"
        "}"
        // ── QTabWidget ─────────────────────────────────────────────────
        "QTabWidget::pane {"
        "  border: 1px solid %4; border-radius: %6; background: %1;"
        "  top: -1px;"
        "}"
        "QTabBar {"
        "  qproperty-drawBase: false;"
        "}"
        "QTabBar::tab {"
        "  background: %3; color: %7; padding: %8;"
        "  border-top-left-radius: %6; border-top-right-radius: %6;"
        "  font-weight: bold; font-size: 11px; border: none;"
        "  margin-right: 1px; margin-left: 0;"
        "  min-width: 0;"  // Allow shrinking; no scroll buttons
        "}"
        "QTabBar::tab:selected {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "    stop:0 %4, stop:1 %19);"
        "  color: %1;"  // OLED black text on amber tab for contrast
        "}"
        "QTabBar::tab:hover:!selected {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "    stop:0 %9, stop:1 %3);"
        "  color: %2;"
        "}"
        // ── QPushButton ─────────────────────────────────────────────────
        "QPushButton {"
        "  background-color: %3; color: %2;"
        "  border: 1px solid %10; border-radius: %11;"
        "  padding: %12; font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background-color: %9; border-color: %13;"
        "}"
        "QPushButton:pressed {"
        "  background-color: %14;"
        "}"
        "QPushButton:disabled {"
        "  color: %15; border-color: %16; background-color: %17;"
        "  opacity: 0.5;"
        "}"
        // ── Accent button (Refresh, Lookup, Ping, GO) — Amber ──────────
        "QPushButton[accent=\"true\"] {"
        "  background-color: %4; color: %1; border: none;"  // Black text on amber
        "}"
        "QPushButton[accent=\"true\"]:hover {"
        "  background-color: %18;"
        "}"
        "QPushButton[accent=\"true\"]:pressed {"
        "  background-color: %19;"
        "}"
        // ── Primary button (Teal) ────────────────────────────────────
        "QPushButton[primary=\"true\"] {"
        "  background-color: %20; color: %1; border: none;"
        "}"
        "QPushButton[primary=\"true\"]:hover {"
        "  background-color: %21;"
        "}"
        "QPushButton[primary=\"true\"]:pressed {"
        "  background-color: %22;"
        "}"
        // ── Danger button (Destructive actions) ─────────────────────
        "QPushButton[danger=\"true\"] {"
        "  background-color: %23; color: %2; border: none;"
        "}"
        "QPushButton[danger=\"true\"]:hover {"
        "  background-color: %24;"
        "}"
        // ── Ghost button (transparent, no fill) ──────────────────────
        "QPushButton[ghost=\"true\"] {"
        "  background-color: transparent; color: %2;"
        "  border: 1px solid %10;"
        "}"
        "QPushButton[ghost=\"true\"]:hover {"
        "  background-color: %9; border-color: %13;"
        "}"
        // ── QLineEdit / QTextEdit / QPlainTextEdit ─────────────────────
        "QLineEdit {"
        "  background-color: %3; color: %2;"
        "  border: 1px solid %10; border-radius: %11;"
        "  padding: %20; selection-background-color: %21;"
        "  selection-color: %1;"
        "}"
        "QLineEdit:focus {"
        "  border-color: %21;"
        "  outline: 2px solid %26; outline-offset: 1px;"
        "}"
        "QLineEdit:hover:!focus {"
        "  border-color: %13;"
        "}"
        "QTextEdit, QPlainTextEdit {"
        "  background-color: %3; color: %2;"
        "  border: 1px solid %10; border-radius: %11;"
        "  padding: %20; selection-background-color: %21;"
        "  selection-color: %1;"
        "}"
        "QTextEdit:focus, QPlainTextEdit:focus {"
        "  border-color: %21;"
        "}"
        // ── QSpinBox (used in Iperf3Window, HistoryTab) ───────────────
        "QSpinBox, QDoubleSpinBox {"
        "  background-color: %3; color: %2;"
        "  border: 1px solid %10; border-radius: %11;"
        "  padding: 4px 8px; selection-background-color: %21;"
        "}"
        "QSpinBox:focus, QDoubleSpinBox:focus { border-color: %21; }"
        "QSpinBox::up-button, QDoubleSpinBox::up-button {"
        "  width: 16px; border: none; background: %9;"
        "}"
        "QSpinBox::down-button, QDoubleSpinBox::down-button {"
        "  width: 16px; border: none; background: %9;"
        "}"
        // ── QComboBox ──────────────────────────────────────────────────
        "QComboBox {"
        "  background-color: %3; color: %2;"
        "  border: 1px solid %10; border-radius: %11;"
        "  padding: %20;"
        "}"
        "QComboBox:hover {"
        "  border-color: %13;"
        "}"
        "QComboBox:focus {"
        "  border-color: %21;"
        "  outline: 2px solid %26; outline-offset: 1px;"
        "}"
        "QComboBox::drop-down { border: none; width: 28px; }"
        "QComboBox::down-arrow {"
        "  image: none; border-left: 5px solid transparent;"
        "  border-right: 5px solid transparent;"
        "  border-top: 6px solid %2; margin-right: 6px;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background-color: %3; color: %2;"
        "  selection-background-color: %4; border: 1px solid %10;"
        "  selection-color: %1;"  // Black text on amber selection
        "  outline: none;"
        "}"
        // ── QCheckBox ─────────────────────────────────────────────────
        "QCheckBox {"
        "  color: %2; font-size: 12px; spacing: 6px;"
        "}"
        "QCheckBox::indicator {"
        "  width: 16px; height: 16px; border-radius: 3px;"
        "  border: 1px solid %10; background: %3;"
        "}"
        "QCheckBox::indicator:checked {"
        "  background-color: %4; border-color: %4;"
        "}"
        "QCheckBox::indicator:hover {"
        "  border-color: %13;"
        "}"
        "QCheckBox:disabled { color: %27; }"
        // ── QProgressBar ───────────────────────────────────────────────
        "QProgressBar {"
        "  background: %3; border-radius: 5px; border: none;"
        "  color: %2; font-size: 11px; font-weight: bold;"
        "  text-align: center;"
        "}"
        "QProgressBar::chunk {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "    stop:0 %19, stop:0.5 %4, stop:1 %18);"  // Amber gradient
        "  border-radius: 5px;"
        "}"
        // ── QTableWidget ───────────────────────────────────────────────
        "QTableWidget {"
        "  background-color: %3; color: %2;"
        "  gridline-color: %10; border: none; font-size: 13px;"
        "  alternate-background-color: %23;"
        "  selection-background-color: %4;"
        "  selection-color: %1;"
        "}"
        "QTableWidget::item { padding: 6px 8px; }"
        "QTableWidget::item:selected {"
        "  background-color: %4; color: %1;"  // Black text on amber selection
        "}"
        "QHeaderView::section {"
        "  background-color: %3; color: %7;"
        "  border: none; border-bottom: 1px solid %10;"
        "  padding: 8px; font-weight: bold; font-size: 11px;"
        "}"
        "QHeaderView::section:hover {"
        "  background-color: %9; color: %2;"
        "}"
        // ── QTreeWidget / QListWidget ───────────────────────────────────
        "QTreeWidget, QListWidget {"
        "  background-color: %3; color: %2;"
        "  alternate-background-color: %23;"
        "  border: 1px solid %10; border-radius: %11;"
        "  padding: 4px;"
        "  selection-background-color: %4;"
        "  selection-color: %1;"
        "  outline: none;"
        "}"
        "QTreeWidget::item, QListWidget::item { padding: 4px 8px; }"
        "QTreeWidget::item:hover, QListWidget::item:hover {"
        "  background-color: %9; color: %2;"
        "}"
        // ── QToolButton (compact icon buttons in toolbars) ─────────────
        "QToolButton {"
        "  background-color: transparent; color: %2;"
        "  border: 1px solid transparent; border-radius: %5;"
        "  padding: 4px; margin: 1px;"
        "}"
        "QToolButton:hover { background-color: %9; border-color: %10; }"
        "QToolButton:pressed { background-color: %14; }"
        "QToolButton:checked { background-color: %4; color: %1; }"
        // ── QToolBar ──────────────────────────────────────────────────
        "QToolBar {"
        "  background: %1; border: none; spacing: 2px;"
        "  padding: 2px;"
        "}"
        // ── QScrollBar (OLED dark theme) ────────────────────────────────
        "QScrollBar:vertical {"
        "  background: %1; width: 10px; margin: 0;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: %10; border-radius: 5px; min-height: 30px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "  background: %4;"  // Amber handle on hover
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "  height: 0; background: none; border: none;"
        "}"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {"
        "  background: none;"
        "}"
        "QScrollBar:horizontal {"
        "  background: %1; height: 10px; margin: 0;"
        "}"
        "QScrollBar::handle:horizontal {"
        "  background: %10; border-radius: 5px; min-width: 30px;"
        "}"
        "QScrollBar::handle:horizontal:hover {"
        "  background: %4;"  // Amber handle on hover
        "}"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {"
        "  width: 0; background: none; border: none;"
        "}"
        "QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {"
        "  background: none;"
        "}"
        // ── QGroupBox ─────────────────────────────────────────────────
        "QGroupBox {"
        "  font-weight: bold; color: %7;"
        "  border: 1px solid %10; border-radius: %6;"
        "  margin-top: 12px; padding-top: 18px; background-color: %1;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin; left: 14px;"
        "  padding: 0 6px; background-color: %1;"
        "}"
        // ── QMenu ─────────────────────────────────────────────────────
        "QMenu {"
        "  background-color: %3; color: %2;"
        "  border: 1px solid %10; border-radius: %11; padding: 4px;"
        "}"
        "QMenu::item { padding: 6px 24px; border-radius: 4px; }"
        "QMenu::item:selected { background-color: %4; color: %1; }"  // Black text on amber
        "QMenu::separator {"
        "  height: 1px; background: %10; margin: 4px 8px;"
        "}"
        // ── QFrame (Cards) ──────────────────────────────────────────────
        "QFrame[card=\"true\"] {"
        "  background-color: %3; border-radius: %6;"
        "  border: 1px solid %10; padding: %24;"
        "}"
        "QFrame[card=\"true\"]:hover {"
        "  border-color: %13;"
        "}"
        // ── QDialog ────────────────────────────────────────────────────
        "QDialog {"
        "  background: %1; color: %2;"
        "}"
        // ── QDialogButtonBox ────────────────────────────────────────────
        "QDialogButtonBox QPushButton {"
        "  min-width: 80px;"
        "}"
        // ── QStatusBar (status row at the bottom of the main window) ───
        "QStatusBar {"
        "  background: %1; color: %7;"
        "  border-top: 1px solid %10;"
        "  padding: 2px 8px; font-size: 11px;"
        "}"
        // ── QLabel[statusdot=\"online\"|\"offline\"|\"pending\"|\"idle\"] ──
        //  Status indicators (round coloured dot in a QLabel) — used in
        //  the dashboard and the tray tooltip.
        "QLabel[statusdot=\"online\"]  { color: %28; font-weight: bold; }"
        "QLabel[statusdot=\"offline\"] { color: %23; font-weight: bold; }"
        "QLabel[statusdot=\"pending\"] { color: %4;  font-weight: bold; }"
        "QLabel[statusdot=\"idle\"]    { color: %15; font-weight: bold; }"
        // ── QLabel[badge=\"info\"|\"warning\"|\"critical\"|\"success\"] ───
        //  Pill-shaped badges used by the alert engine.
        "QLabel[badge=\"true\"] {"
        "  border-radius: %5; padding: 2px 8px;"
        "  font-size: 10px; font-weight: bold;"
        "}"
        "QLabel[badge=\"info\"]     { background: %21; color: %1; }"
        "QLabel[badge=\"warning\"]  { background: %4;  color: %1; }"
        "QLabel[badge=\"critical\"] { background: %23; color: %2; }"
        "QLabel[badge=\"success\"]  { background: %5;  color: %1; }"
    )
        .arg(C_BG,          C_TEXT,        C_BG_ELEVATED,  //  1  2  3
             C_ACCENT,       RADIUS_SM,     RADIUS_LG,      //  4  5  6
             C_TEXT_DIM,     PADDING_TAB,   C_BG_HOVER,     //  7  8  9
             C_BORDER,       RADIUS_MD,     PADDING_BTN,    // 10 11 12
             C_ACCENT_HVR,   C_ACCENT_ACT,  C_TEXT_MUTED,   // 13 14 15
             C_BORDER,       C_BG_SUNKEN,   C_ACCENT_HVR,   // 16 17 18
             C_ACCENT_ACT,   PADDING_INP,   C_PRIMARY,      // 19 20 21
             C_SUCCESS,      C_BG_ELEVATED, PADDING_CRD,    // 22 23 24
             C_CARET,        C_FOCUS_OUTLINE,                // 25 26
             C_MUTED,        C_DOT_ONLINE)                  // 27 28
        ;
}

/// Light theme — same Teal Amber hues on a light surface.
[[nodiscard]]
inline QString appStyleSheetLight() noexcept
{
    return QStringLiteral(
        // ── Global ────────────────────────────────────────────────────
        "QWidget {"
        "  background-color: %1; color: %2;"
        "  font-family: 'Segoe UI', 'Noto Sans', 'Ubuntu', sans-serif;"
        "  font-size: 13px;"
        "}"
        // ── Tooltip ────────────────────────────────────────────────────
        "QToolTip {"
        "  background-color: %3; color: %2;"
        "  border: 1px solid %4; border-radius: %5;"
        "  padding: 6px 10px; font-size: 12px;"
        "}"
        // ── QTabWidget ─────────────────────────────────────────────────
        "QTabWidget::pane {"
        "  border: 1px solid %4; border-radius: %6; background: %1;"
        "  top: -1px;"
        "}"
        "QTabBar::tab {"
        "  background: %3; color: %7; padding: %8;"
        "  border-top-left-radius: %6; border-top-right-radius: %6;"
        "  font-weight: bold; font-size: 11px; border: none;"
        "  margin-right: 1px; margin-left: 0;"
        "  min-width: 0;"
        "}"
        "QTabBar::tab:selected {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "    stop:0 %4, stop:1 %19);"
        "  color: %1;"
        "}"
        "QTabBar::tab:hover:!selected {"
        "  background: %9; color: %2;"
        "}"
        // ── QPushButton ─────────────────────────────────────────────────
        "QPushButton {"
        "  background-color: %3; color: %2;"
        "  border: 1px solid %4; border-radius: %5;"
        "  padding: %6; font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: %9; border-color: %10; }"
        "QPushButton:pressed { background-color: %11; }"
        "QPushButton:disabled { color: %12; opacity: 0.5; }"
        "QPushButton[accent=\"true\"] {"
        "  background-color: %13; color: %1; border: none;"
        "}"
        "QPushButton[accent=\"true\"]:hover { background-color: %14; }"
        "QPushButton[primary=\"true\"] {"
        "  background-color: %15; color: %1; border: none;"
        "}"
        "QPushButton[primary=\"true\"]:hover { background-color: %16; }"
        "QPushButton[danger=\"true\"] {"
        "  background-color: %17; color: %1; border: none;"
        "}"
        "QPushButton[ghost=\"true\"] {"
        "  background-color: transparent; color: %2;"
        "  border: 1px solid %4;"
        "}"
        "QPushButton[ghost=\"true\"]:hover { background-color: %9; }"
        // ── Inputs ────────────────────────────────────────────────────
        "QLineEdit, QTextEdit, QPlainTextEdit, QSpinBox, QDoubleSpinBox {"
        "  background-color: %3; color: %2;"
        "  border: 1px solid %4; border-radius: %5;"
        "  padding: %6; selection-background-color: %18; selection-color: %1;"
        "}"
        "QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus,"
        "QSpinBox:focus, QDoubleSpinBox:focus {"
        "  border-color: %15; outline: 2px solid %20; outline-offset: 1px;"
        "}"
        "QComboBox {"
        "  background-color: %3; color: %2;"
        "  border: 1px solid %4; border-radius: %5; padding: %6;"
        "}"
        "QComboBox:hover { border-color: %10; }"
        "QComboBox:focus { border-color: %15; outline: 2px solid %20; }"
        "QComboBox QAbstractItemView {"
        "  background-color: %3; color: %2;"
        "  selection-background-color: %13; selection-color: %1;"
        "  border: 1px solid %4; outline: none;"
        "}"
        "QCheckBox { color: %2; spacing: 6px; }"
        "QCheckBox::indicator {"
        "  width: 16px; height: 16px; border-radius: 3px;"
        "  border: 1px solid %4; background: %3;"
        "}"
        "QCheckBox::indicator:checked {"
        "  background-color: %13; border-color: %13;"
        "}"
        // ── QProgressBar ───────────────────────────────────────────────
        "QProgressBar {"
        "  background: %3; border-radius: 5px; border: 1px solid %4;"
        "  color: %2; text-align: center;"
        "}"
        "QProgressBar::chunk {"
        "  background: %15; border-radius: 5px;"
        "}"
        // ── QTableWidget / QTreeWidget / QListWidget ───────────────────
        "QTableWidget, QTreeWidget, QListWidget {"
        "  background-color: %3; color: %2;"
        "  gridline-color: %4; border: 1px solid %4; border-radius: %5;"
        "  alternate-background-color: %9;"
        "  selection-background-color: %13; selection-color: %1;"
        "  outline: none;"
        "}"
        "QTableWidget::item, QTreeWidget::item, QListWidget::item { padding: 6px 8px; }"
        "QHeaderView::section {"
        "  background-color: %9; color: %2;"
        "  border: none; border-bottom: 1px solid %4;"
        "  padding: 8px; font-weight: bold;"
        "}"
        // ── QToolButton / QToolBar ────────────────────────────────────
        "QToolButton {"
        "  background: transparent; color: %2;"
        "  border: 1px solid transparent; border-radius: %5;"
        "  padding: 4px;"
        "}"
        "QToolButton:hover { background: %9; }"
        "QToolBar { background: %1; border: none; spacing: 2px; padding: 2px; }"
        // ── QScrollBar ─────────────────────────────────────────────────
        "QScrollBar:vertical { background: %1; width: 10px; }"
        "QScrollBar::handle:vertical { background: %4; border-radius: 5px; min-height: 30px; }"
        "QScrollBar::handle:vertical:hover { background: %10; }"
        "QScrollBar:horizontal { background: %1; height: 10px; }"
        "QScrollBar::handle:horizontal { background: %4; border-radius: 5px; min-width: 30px; }"
        "QScrollBar::handle:horizontal:hover { background: %10; }"
        "QScrollBar::add-line, QScrollBar::sub-line { background: none; border: none; }"
        // ── QGroupBox ─────────────────────────────────────────────────
        "QGroupBox {"
        "  font-weight: bold; color: %2;"
        "  border: 1px solid %4; border-radius: %5;"
        "  margin-top: 12px; padding-top: 18px;"
        "}"
        // ── QMenu ─────────────────────────────────────────────────────
        "QMenu {"
        "  background-color: %3; color: %2;"
        "  border: 1px solid %4; border-radius: %5; padding: 4px;"
        "}"
        "QMenu::item { padding: 6px 24px; border-radius: 4px; }"
        "QMenu::item:selected { background-color: %13; color: %1; }"
        "QMenu::separator { height: 1px; background: %4; margin: 4px 8px; }"
        // ── QFrame[card] ──────────────────────────────────────────────
        "QFrame[card=\"true\"] {"
        "  background-color: %3; border-radius: %5;"
        "  border: 1px solid %4; padding: %21;"
        "}"
        "QFrame[card=\"true\"]:hover { border-color: %10; }"
        // ── QDialog / QStatusBar ───────────────────────────────────────
        "QDialog { background: %1; color: %2; }"
        "QStatusBar {"
        "  background: %9; color: %2;"
        "  border-top: 1px solid %4; padding: 2px 8px;"
        "}"
        "QDialogButtonBox QPushButton { min-width: 80px; }"
        // ── QLabel statusdot / badge ──────────────────────────────────
        "QLabel[statusdot=\"online\"]  { color: %22; font-weight: bold; }"
        "QLabel[statusdot=\"offline\"] { color: %17; font-weight: bold; }"
        "QLabel[statusdot=\"pending\"] { color: %13; font-weight: bold; }"
        "QLabel[statusdot=\"idle\"]    { color: %12; font-weight: bold; }"
        "QLabel[badge=\"true\"] {"
        "  border-radius: %5; padding: 2px 8px;"
        "  font-size: 10px; font-weight: bold;"
        "}"
        "QLabel[badge=\"info\"]     { background: %15; color: %1; }"
        "QLabel[badge=\"warning\"]  { background: %13; color: %1; }"
        "QLabel[badge=\"critical\"] { background: %17; color: %1; }"
        "QLabel[badge=\"success\"]  { background: %22; color: %1; }"
    )
        .arg(C_L_BG,        C_L_TEXT,      C_L_BG_ELEVATED, //  1  2  3
             C_L_BORDER,     RADIUS_MD,     PADDING_BTN,     //  4  5  6
             C_L_TEXT_DIM,   PADDING_TAB,   C_L_BG_HOVER,    //  7  8  9
             C_L_BORDER_HVR, C_L_SURFACE_ACT,                // 10 11
             C_L_TEXT_MUTED, C_L_ACCENT_FALLBACK,            // 12 13
             C_L_ACCENT_HOVER,                                // 14
             C_L_PRIMARY, C_L_PRIMARY_HOVER,                 // 15 16
             C_L_ERROR,                                         // 17
             C_L_SELECTION_BG, C_L_CARET, C_L_FOCUS_OUTLINE,  // 18 19 20
             PADDING_CRD, C_L_SUCCESS)                        // 21 22
        ;
}

/// High-Contrast theme — pure black + pure white + saturated accents.
/// All decorative colours collapse to the FG/BG pair. Used by the
/// accessibility / low-vision theme switch.
[[nodiscard]]
inline QString appStyleSheetHighContrast() noexcept
{
    return QStringLiteral(
        "QWidget {"
        "  background-color: %1; color: %2;"
        "  font-family: 'Segoe UI', 'Noto Sans', 'Ubuntu', sans-serif;"
        "  font-size: 14px;"   // Slightly larger for legibility
        "}"
        "QToolTip {"
        "  background-color: %2; color: %1;"
        "  border: 2px solid %3; padding: 6px 10px;"
        "  font-size: 13px; font-weight: bold;"
        "}"
        "QTabWidget::pane {"
        "  border: 2px solid %3; background: %1;"
        "}"
        "QTabBar::tab {"
        "  background: %1; color: %2; padding: 8px 16px;"
        "  font-weight: bold; font-size: 13px; border: 2px solid %3;"
        "  margin-right: 2px;"
        "}"
        "QTabBar::tab:selected {"
        "  background: %3; color: %1;"  // Black on white
        "}"
        "QTabBar::tab:hover:!selected {"
        "  background: %4; color: %1;"
        "}"
        "QPushButton {"
        "  background-color: %1; color: %2;"
        "  border: 2px solid %3; border-radius: 0;"
        "  padding: 10px 24px; font-weight: bold; min-height: 32px;"
        "}"
        "QPushButton:hover { background-color: %2; color: %1; }"
        "QPushButton:pressed { background-color: %5; color: %1; }"
        "QPushButton:disabled { color: %6; border-color: %6; }"
        "QPushButton[accent=\"true\"], QPushButton[primary=\"true\"],"
        "QPushButton[danger=\"true\"] {"
        "  background-color: %3; color: %1; border: 2px solid %3;"
        "}"
        "QPushButton[accent=\"true\"]:hover, QPushButton[primary=\"true\"]:hover {"
        "  background-color: %4; color: %1; border-color: %4;"
        "}"
        "QPushButton[danger=\"true\"]:hover {"
        "  background-color: %7; color: %1; border-color: %7;"
        "}"
        "QLineEdit, QTextEdit, QPlainTextEdit,"
        "QSpinBox, QDoubleSpinBox, QComboBox {"
        "  background-color: %1; color: %2;"
        "  border: 2px solid %3; border-radius: 0;"
        "  padding: 8px 12px; selection-background-color: %2;"
        "  selection-color: %1;"
        "}"
        "QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus,"
        "QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus {"
        "  border: 2px solid %2; outline: 2px solid %2; outline-offset: 2px;"
        "}"
        "QCheckBox { color: %2; font-size: 14px; spacing: 8px; }"
        "QCheckBox::indicator {"
        "  width: 18px; height: 18px; border: 2px solid %3; background: %1;"
        "}"
        "QCheckBox::indicator:checked {"
        "  background-color: %2; border-color: %2;"
        "}"
        "QProgressBar {"
        "  background: %1; border: 2px solid %3; border-radius: 0;"
        "  color: %2; text-align: center; font-weight: bold;"
        "}"
        "QProgressBar::chunk {"
        "  background: %2; border-radius: 0;"
        "}"
        "QTableWidget, QTreeWidget, QListWidget {"
        "  background-color: %1; color: %2;"
        "  gridline-color: %3; border: 2px solid %3;"
        "  alternate-background-color: %8;"
        "  selection-background-color: %2; selection-color: %1;"
        "  outline: none; font-size: 13px;"
        "}"
        "QTableWidget::item, QTreeWidget::item, QListWidget::item { padding: 6px 8px; }"
        "QHeaderView::section {"
        "  background-color: %2; color: %1;"
        "  border: 2px solid %1; padding: 8px; font-weight: bold;"
        "}"
        "QToolButton {"
        "  background: %1; color: %2; border: 2px solid %3;"
        "  border-radius: 0; padding: 6px;"
        "}"
        "QToolButton:hover { background: %2; color: %1; }"
        "QToolBar { background: %1; border-bottom: 2px solid %3; spacing: 2px; }"
        "QScrollBar:vertical { background: %1; width: 14px; }"
        "QScrollBar::handle:vertical {"
        "  background: %3; min-height: 30px; border: 1px solid %2;"
        "}"
        "QScrollBar::handle:vertical:hover { background: %2; }"
        "QScrollBar:horizontal { background: %1; height: 14px; }"
        "QScrollBar::handle:horizontal {"
        "  background: %3; min-width: 30px; border: 1px solid %2;"
        "}"
        "QScrollBar::add-line, QScrollBar::sub-line { background: none; border: none; }"
        "QGroupBox {"
        "  font-weight: bold; color: %2;"
        "  border: 2px solid %3; border-radius: 0;"
        "  margin-top: 14px; padding-top: 18px;"
        "}"
        "QMenu {"
        "  background-color: %1; color: %2;"
        "  border: 2px solid %3; padding: 4px;"
        "}"
        "QMenu::item { padding: 8px 24px; }"
        "QMenu::item:selected { background-color: %2; color: %1; }"
        "QMenu::separator { height: 1px; background: %3; margin: 4px 8px; }"
        "QFrame[card=\"true\"] {"
        "  background-color: %1; border: 2px solid %3; padding: %9;"
        "}"
        "QDialog { background: %1; color: %2; }"
        "QStatusBar {"
        "  background: %2; color: %1; font-weight: bold;"
        "  border-top: 2px solid %3; padding: 4px 8px;"
        "}"
        "QDialogButtonBox QPushButton { min-width: 80px; }"
        "QLabel[statusdot=\"online\"]  { color: %9;  font-weight: bold; }"
        "QLabel[statusdot=\"offline\"] { color: %7;  font-weight: bold; }"
        "QLabel[statusdot=\"pending\"] { color: %4;  font-weight: bold; }"
        "QLabel[statusdot=\"idle\"]    { color: %3;  font-weight: bold; }"
        "QLabel[badge=\"true\"] {"
        "  border: 2px solid %2; padding: 2px 8px;"
        "  font-size: 11px; font-weight: bold;"
        "}"
        "QLabel[badge=\"info\"]     { background: %2; color: %1; }"
        "QLabel[badge=\"warning\"]  { background: %4; color: %1; }"
        "QLabel[badge=\"critical\"] { background: %7; color: %1; }"
        "QLabel[badge=\"success\"]  { background: %9; color: %1; }"
    )
        .arg(C_HC_BG,        C_HC_TEXT,     C_HC_BORDER,     //  1  2  3
             C_HC_ACCENT,    C_HC_SUCCESS,  C_HC_MUTED,      //  4  5  6
             C_HC_ERROR,     C_HC_BG_HOVER,                   //  7  8
             PADDING_CRD,    C_HC_PRIMARY)                    //  9 10
        ;
}

/// Backward-compatible alias. The pre-v2.15.5 code calls appStyleSheet()
/// without a mode argument — that still works, returning the Dark theme.
/// New code should call appStyleSheetDark()/appStyleSheetLight()/
/// appStyleSheetHighContrast() explicitly.
[[nodiscard]]
inline QString appStyleSheet() noexcept
{
    return appStyleSheetDark();
}

/// Theme-mode dispatch. Returns the correct stylesheet for the given mode.
[[nodiscard]]
inline QString appStyleSheetFor(ThemeMode mode) noexcept
{
    switch (mode) {
        case ThemeMode::Dark:         return appStyleSheetDark();
        case ThemeMode::Light:        return appStyleSheetLight();
        case ThemeMode::HighContrast: return appStyleSheetHighContrast();
    }
    return appStyleSheetDark();
}

// ═══════════════════════════════════════════════════════════════════════════════
//  WIDGET HELPER STYLES
//  These return per-widget QSS fragments that are not covered by the
//  global app stylesheet. They all follow the Teal Amber OLED palette
//  unless the helper is theme-aware (see the applyTo*() family at the
//  end of the section).
// ═══════════════════════════════════════════════════════════════════════════════

// ── Accent button (Amber) ─────────────────────────────────────────────────
[[nodiscard]] inline QString btnAccentStyle() noexcept
{
    return QStringLiteral(
        "QPushButton { background-color: %1; color: %2; border: none;"
        "  border-radius: %3; padding: %4; font-weight: bold; }"
        "QPushButton:hover { background-color: %5; }"
        "QPushButton:pressed { background-color: %6; }"
        "QPushButton:disabled { background-color: %7; color: %8; }"
    ).arg(C_ACCENT, C_BG, RADIUS_MD, PADDING_BTN,
          C_ACCENT_HVR, C_ACCENT_ACT, C_MUTED, C_TEXT_MUTED);
    //   ^Amber    ^Black text for contrast
}

// ── Primary button (Teal) ─────────────────────────────────────────────────
[[nodiscard]] inline QString btnPrimaryStyle() noexcept
{
    return QStringLiteral(
        "QPushButton { background-color: %1; color: %2; border: none;"
        "  border-radius: %3; padding: %4; font-weight: bold; }"
        "QPushButton:hover { background-color: %5; }"
        "QPushButton:pressed { background-color: %6; }"
        "QPushButton:disabled { background-color: %7; color: %8; }"
    ).arg(C_PRIMARY, C_BG, RADIUS_MD, PADDING_BTN,
          C_PRIMARY_HVR, C_PRIMARY_ACT, C_MUTED, C_TEXT_MUTED);
    //   ^Teal       ^Black text for contrast
}

// ── Secondary button (outline / subtle) ────────────────────────────────────
[[nodiscard]] inline QString btnSecondaryStyle() noexcept
{
    return QStringLiteral(
        "QPushButton { background-color: %1; color: %2;"
        "  border: 1px solid %3; border-radius: %4; padding: %5; }"
        "QPushButton:hover { background-color: %6; border-color: %7; }"
        "QPushButton:pressed { background-color: %8; }"
        "QPushButton:disabled { color: %9; border-color: %10; }"
    ).arg(C_BG_ELEVATED, C_TEXT, C_BORDER, RADIUS_MD, PADDING_BTN,
          C_BG_HOVER, C_PRIMARY, C_SURFACE_ACT, C_TEXT_MUTED, C_MUTED);
}

// ── Small button (outline, compact) ───────────────────────────────────────
[[nodiscard]] inline QString btnSmallStyle() noexcept
{
    return QStringLiteral(
        "QPushButton { background-color: %1; color: %2;"
        "  border: 1px solid %3; border-radius: %4; padding: %5; font-size: 11px; }"
        "QPushButton:hover { background-color: %6; border-color: %7; }"
        "QPushButton:disabled { color: %8; border-color: %9; }"
    ).arg(C_BG_ELEVATED, C_TEXT, C_BORDER, RADIUS_MD, PADDING_BTN_S,
          C_BG_HOVER, C_PRIMARY, C_TEXT_MUTED, C_MUTED);
}

// ── Danger button (destructive actions) ────────────────────────────────────
[[nodiscard]] inline QString btnDangerStyle() noexcept
{
    return QStringLiteral(
        "QPushButton { background-color: %1; color: %2; border: none;"
        "  border-radius: %3; padding: %4; font-weight: bold; }"
        "QPushButton:hover { background-color: %5; }"
        "QPushButton:pressed { background-color: %6; }"
    ).arg(C_ERROR, C_TEXT, RADIUS_MD, PADDING_BTN, C_ERROR_HVR, C_CRITICAL);
}

// ── Ghost button (transparent) ──────────────────────────────────────────────
[[nodiscard]] inline QString btnGhostStyle() noexcept
{
    return QStringLiteral(
        "QPushButton { background-color: transparent; color: %1;"
        "  border: 1px solid %2; border-radius: %3; padding: %4; }"
        "QPushButton:hover { background-color: %5; border-color: %6; }"
    ).arg(C_TEXT, C_BORDER, RADIUS_MD, PADDING_BTN, C_BG_HOVER, C_PRIMARY);
}

// ── Input field (normal state) ────────────────────────────────────────────
[[nodiscard]] inline QString inputStyle() noexcept
{
    return QStringLiteral(
        "QLineEdit { background-color: %1; color: %2;"
        "  border: 1px solid %3; border-radius: %4; padding: %5;"
        "  selection-background-color: %6; selection-color: %7;"
        "}"
        "QLineEdit:focus { border-color: %8;"
        "  outline: 2px solid %9; outline-offset: 1px; }"
        "QLineEdit:hover:!focus { border-color: %10; }"
        "QLineEdit:disabled { color: %11; background: %12; }"
    ).arg(C_BG_ELEVATED, C_TEXT, C_BORDER, RADIUS_MD, PADDING_INP,
          C_SELECTION_BG, C_SELECTION_FG,
          C_PRIMARY, C_FOCUS_OUTLINE, C_ACCENT,
          C_TEXT_MUTED, C_BG_SUNKEN);
}

// ── Input field (warning state) ───────────────────────────────────────────
[[nodiscard]] inline QString inputStyleWarning() noexcept
{
    return QStringLiteral(
        "QLineEdit { background-color: %1; color: %2;"
        "  border: 1px solid %3; border-radius: %4; padding: %5;"
        "  selection-background-color: %6; selection-color: %7; }"
        "QLineEdit:focus { border-color: %3; outline: 2px solid %3; }"
    ).arg(C_BG_ELEVATED, C_TEXT, C_WARNING_BORDER, RADIUS_MD, PADDING_INP,
          C_SELECTION_BG, C_SELECTION_FG);
}

// ── Input field (success state) ───────────────────────────────────────────
[[nodiscard]] inline QString inputStyleSuccess() noexcept
{
    return QStringLiteral(
        "QLineEdit { background-color: %1; color: %2;"
        "  border: 1px solid %3; border-radius: %4; padding: %5;"
        "  selection-background-color: %6; selection-color: %7; }"
        "QLineEdit:focus { border-color: %3; outline: 2px solid %3; }"
    ).arg(C_BG_ELEVATED, C_TEXT, C_SUCCESS_BORDER, RADIUS_MD, PADDING_INP,
          C_SELECTION_BG, C_SELECTION_FG);
}

// ── Input field (error state) ─────────────────────────────────────────────
[[nodiscard]] inline QString inputStyleError() noexcept
{
    return QStringLiteral(
        "QLineEdit { background-color: %1; color: %2;"
        "  border: 1px solid %3; border-radius: %4; padding: %5;"
        "  selection-background-color: %6; selection-color: %7; }"
        "QLineEdit:focus { border-color: %3; outline: 2px solid %3; }"
    ).arg(C_BG_ELEVATED, C_TEXT, C_ERROR_BORDER, RADIUS_MD, PADDING_INP,
          C_SELECTION_BG, C_SELECTION_FG);
}

// ── ComboBox ──────────────────────────────────────────────────────────────
[[nodiscard]] inline QString comboStyle() noexcept
{
    return QStringLiteral(
        "QComboBox { background-color: %1; color: %2;"
        "  border: 1px solid %3; border-radius: %4; padding: %5;"
        "  selection-background-color: %6; selection-color: %7; }"
        "QComboBox:hover { border-color: %8; }"
        "QComboBox::drop-down { border: none; width: 24px; }"
        "QComboBox::down-arrow {"
        "  image: none; border-left: 5px solid transparent;"
        "  border-right: 5px solid transparent;"
        "  border-top: 6px solid %2; margin-right: 6px; }"
        "QComboBox QAbstractItemView {"
        "  background-color: %1; color: %2;"
        "  selection-background-color: %9; selection-color: %10;"
        "  border: 1px solid %3; outline: none; }"
    ).arg(C_BG_ELEVATED, C_TEXT, C_BORDER, RADIUS_MD, PADDING_INP,
          C_SELECTION_BG, C_SELECTION_FG,
          C_ACCENT, C_ACCENT, C_BG);
    //                                    ^ Amber selection ^ Black text on amber
}

// ── Monospace output area (terminal/code output) ──────────────────────────
[[nodiscard]] inline QString monoStyle() noexcept
{
    return QStringLiteral(
        "QTextEdit, QPlainTextEdit { background-color: %1; color: %2;"
        "  border: none; border-radius: %3; padding: %4;"
        "  font-family: 'JetBrains Mono', 'Cascadia Code', 'Consolas', monospace;"
        "  font-size: 12px; }"
    ).arg(C_BG_SUNKEN, C_SUCCESS, RADIUS_LG, PADDING_M);
    //                 ^ Teal-green text for terminal output
}

// ── Card / Frame ──────────────────────────────────────────────────────────
[[nodiscard]] inline QString cardStyle() noexcept
{
    return QStringLiteral(
        "QFrame { background-color: %1; border-radius: %2;"
        "  border: 1px solid %3; padding: %4; }"
        "QFrame:hover { border-color: %5; }"  // Amber border on hover
    ).arg(C_BG_ELEVATED, RADIUS_XL, C_BORDER, PADDING_CRD, C_ACCENT);
}

// ── Status label ──────────────────────────────────────────────────────────
[[nodiscard]] inline QString statusLabelStyle() noexcept
{
    return QStringLiteral(
        "QLabel { color: %1; font-size: 10px; padding: 5px; }"
    ).arg(C_TEXT_DIM);
}

// ── Online / positive state label (teal-green) ────────────────────────────
[[nodiscard]] inline QString onlineLabelStyle() noexcept
{
    return QStringLiteral(
        "QLabel { color: %1; font-weight: bold; }"
    ).arg(C_SUCCESS);
}

// ── Status dot (used in dashboard and tray) ───────────────────────────────
[[nodiscard]] inline QString statusDotStyle(const QString &color) noexcept
{
    // Used for inline dot rendering: setStyleSheet(colorDotStyle(C_DOT_ONLINE))
    return QStringLiteral(
        "QLabel { color: %1; font-weight: bold; font-size: 14px; }"
    ).arg(color);
}

// ── Severity badge style (alert engine) ──────────────────────────────────
[[nodiscard]] inline QString severityBadgeStyle(const QString &bg,
                                                const QString &fg) noexcept
{
    return QStringLiteral(
        "QLabel { background-color: %1; color: %2; border-radius: %3;"
        "  padding: 2px 8px; font-size: 10px; font-weight: bold; }"
    ).arg(bg, fg, RADIUS_PILL);
}

// ── Notification / Toast ──────────────────────────────────────────────────
[[nodiscard]] inline QString toastStyle() noexcept
{
    return QStringLiteral(
        "QFrame { background-color: %1; color: %2;"
        "  border: 1px solid %3; border-radius: %4;"
        "  padding: 12px 16px; }"
    ).arg(C_BG_ELEVATED, C_TEXT, C_ACCENT, RADIUS_LG);
}

// ── Splitter (used in AuditorTab, Connections) ────────────────────────────
[[nodiscard]] inline QString splitterStyle() noexcept
{
    return QStringLiteral(
        "QSplitter::handle { background-color: %1; }"
        "QSplitter::handle:horizontal { width: 1px; }"
        "QSplitter::handle:vertical   { height: 1px; }"
        "QSplitter::handle:hover { background-color: %2; }"
    ).arg(C_BORDER, C_ACCENT);
}

// ── Tree widget (used by HistoryTab, possibly others) ────────────────────
[[nodiscard]] inline QString treeWidgetStyle() noexcept
{
    return QStringLiteral(
        "QTreeWidget { background-color: %1; color: %2;"
        "  alternate-background-color: %3;"
        "  border: 1px solid %4; border-radius: %5;"
        "  selection-background-color: %6; selection-color: %7;"
        "  outline: none; }"
        "QTreeWidget::item { padding: 4px 8px; }"
        "QTreeWidget::item:hover { background-color: %8; }"
    ).arg(C_BG_ELEVATED, C_TEXT, C_BG, C_BORDER, RADIUS_MD,
          C_ACCENT, C_BG, C_BG_HOVER);
}

// ── List widget (used by AuditorTab, HistoryTab) ────────────────────────
[[nodiscard]] inline QString listWidgetStyle() noexcept
{
    return QStringLiteral(
        "QListWidget { background-color: %1; color: %2;"
        "  alternate-background-color: %3;"
        "  border: 1px solid %4; border-radius: %5;"
        "  padding: 4px;"
        "  selection-background-color: %6; selection-color: %7;"
        "  outline: none; }"
        "QListWidget::item { padding: 4px 8px; border-radius: 4px; }"
        "QListWidget::item:hover { background-color: %8; }"
    ).arg(C_BG_ELEVATED, C_TEXT, C_BG, C_BORDER, RADIUS_MD,
          C_ACCENT, C_BG, C_BG_HOVER);
}

// ── Toolbar ──────────────────────────────────────────────────────────────
[[nodiscard]] inline QString toolbarStyle() noexcept
{
    return QStringLiteral(
        "QToolBar { background: %1; border: none; spacing: 2px; padding: 2px; }"
        "QToolButton {"
        "  background: transparent; color: %2;"
        "  border: 1px solid transparent; border-radius: %3;"
        "  padding: 4px; margin: 1px;"
        "}"
        "QToolButton:hover { background: %4; border-color: %5; }"
        "QToolButton:pressed { background: %6; }"
        "QToolButton:checked { background: %7; color: %1; }"
    ).arg(C_BG, C_TEXT, RADIUS_SM, C_BG_HOVER, C_BORDER, C_SURFACE_ACT, C_ACCENT);
}

/// Inline `color: <c>;` QSS fragment for a single label / widget that
/// cannot be matched by a global selector. Cheap to call: a single
/// QStringLiteral + .arg(). The argument is `const char*` so the
/// `C_*` hex-token constants can be passed without explicit conversion.
[[nodiscard]] inline QString colorStyle(const char *color) noexcept
{
    return QStringLiteral("color: %1;").arg(QString::fromLatin1(color));
}

// ═══════════════════════════════════════════════════════════════════════════════
//  DATA-VISUALIZATION HELPERS
//  Pure functions — no QObject, no widget dependencies. Safe to call
//  from any thread.
// ═══════════════════════════════════════════════════════════════════════════════

/// Return the i-th chart colour from the 10-colour categorical palette.
/// `index` is taken modulo 10 so out-of-range values wrap around instead
/// of producing a Q_ASSERT in release builds.
[[nodiscard]] inline QString chartColor(int index) noexcept
{
    constexpr const char *palette[10] = {
        C_CHART_0, C_CHART_1, C_CHART_2, C_CHART_3, C_CHART_4,
        C_CHART_5, C_CHART_6, C_CHART_7, C_CHART_8, C_CHART_9,
    };
    constexpr int N = sizeof(palette) / sizeof(palette[0]);
    return QString::fromLatin1(palette[((index % N) + N) % N]);
}

/// Map a kernel TCP/UDP state name (e.g. "ESTABLISHED", "TIME_WAIT",
/// "UDP") to its palette colour. Unknown states fall through to
/// C_STATE_CLOSED (grey) so the table never produces an empty cell.
[[nodiscard]] inline QString connectionStateColor(const QString &state) noexcept
{
    if (state == QStringLiteral("ESTABLISHED")) return QString::fromLatin1(C_STATE_ESTABLISHED);
    if (state == QStringLiteral("LISTEN"))     return QString::fromLatin1(C_STATE_LISTEN);
    if (state == QStringLiteral("TIME_WAIT"))  return QString::fromLatin1(C_STATE_TIME_WAIT);
    if (state == QStringLiteral("CLOSE_WAIT")) return QString::fromLatin1(C_STATE_CLOSE_WAIT);
    if (state == QStringLiteral("FIN_WAIT1") ||
        state == QStringLiteral("FIN_WAIT2"))  return QString::fromLatin1(C_STATE_FIN_WAIT);
    if (state == QStringLiteral("CLOSED"))     return QString::fromLatin1(C_STATE_CLOSED);
    if (state == QStringLiteral("SYN_SENT") ||
        state == QStringLiteral("SYN_RECV"))   return QString::fromLatin1(C_STATE_SYN_SENT);
    if (state == QStringLiteral("UDP") ||
        state == QStringLiteral("udp"))        return QString::fromLatin1(C_STATE_UDP);
    return QString::fromLatin1(C_STATE_CLOSED);
}

/// Map a round-trip latency (ms) to its gradient bucket. The cutoffs
/// match the Topology tab's quality labels. A negative or zero value
/// counts as "timeout" (grey) because 0.0 ms is what traceroute emits
/// for a missing reply.
[[nodiscard]] inline QString latencyColorMs(double ms) noexcept
{
    if (ms <= 0.0)  return QString::fromLatin1(C_LATENCY_TIMEOUT);
    if (ms <  10.0) return QString::fromLatin1(C_LATENCY_EXCELLENT);
    if (ms <  50.0) return QString::fromLatin1(C_LATENCY_GOOD);
    if (ms < 150.0) return QString::fromLatin1(C_LATENCY_FAIR);
    return                 QString::fromLatin1(C_LATENCY_POOR);
}

/// Map a transfer speed (Mbps) to its Iperf3 progress-bar colour.
[[nodiscard]] inline QString speedColorMbps(double mbps) noexcept
{
    if (mbps >= 100.0) return QString::fromLatin1(C_SUCCESS);
    if (mbps >=  50.0) return QString::fromLatin1(C_PRIMARY);
    if (mbps >=  10.0) return QString::fromLatin1(C_WARNING);
    return                    QString::fromLatin1(C_ERROR);
}

// ═══════════════════════════════════════════════════════════════════════════════
//  BEHAVIOUR HELPERS
//  Not QSS — per-widget behavioural defaults. Centralising them here
//  keeps every widget in the project visually and behaviourally consistent.
// ═══════════════════════════════════════════════════════════════════════════════

/// Apply the standard IP View Pro styling and behaviour to a QTableWidget.
/// The optional \a sortable flag enables column-header sorting (off by
/// default because the speed-test, packet, and topology tables are
/// filled in arrival order). The optional \a showVerticalHeader flag
/// shows the row-number column (off by default — the OLED look is
/// header-free on the left edge). Safe to call with a null pointer.
inline void applyTableStyle(QTableWidget *table,
                            bool sortable       = false,
                            bool showVerticalHeader = false) noexcept
{
    if (table == nullptr) return;
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setAlternatingRowColors(true);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSortingEnabled(sortable);
    if (auto *vh = table->verticalHeader()) vh->setVisible(showVerticalHeader);
}

// ═══════════════════════════════════════════════════════════════════════════════
//  THEME APPLICATION
//  The single entry point called from main.cpp. Picks the right
//  stylesheet for the requested mode and applies it to the QApplication.
//
//  Implemented as free functions (not member of a Theme class) so the
//  call site is short and so the function can be called from anywhere
//  in the project. The QApplication forward declaration is replaced by
//  a real include so we can call setStyleSheet() on the incomplete type.
// ═══════════════════════════════════════════════════════════════════════════════

#include <QCoreApplication>   // Provides QCoreApplication
#include <QApplication>       // Provides QApplication::setStyleSheet

namespace IPView::Theme {

/// Apply the requested theme to a QApplication. Safe to call before
/// or after widgets are constructed; QSS changes are picked up
/// immediately on the next event-loop tick.
inline void applyTheme(QApplication &app, ThemeMode mode) noexcept
{
    app.setStyleSheet(appStyleSheetFor(mode));
}

/// Convenience overload for the case where the caller only has a
/// QCoreApplication pointer (e.g. early in main() before QApplication
/// is constructed). Falls through to no-op since QCoreApplication
/// cannot take a stylesheet.
inline void applyTheme(QCoreApplication & /*app*/, ThemeMode /*mode*/) noexcept
{
    // QCoreApplication has no setStyleSheet — nothing to do.
}

/// Re-apply the currently-active theme. Useful after the user toggles
/// the theme at runtime via the View menu.
inline void refreshTheme(QApplication &app, ThemeMode mode) noexcept
{
    // QApplication first clears the old stylesheet, then applies the
    // new one. Without the clear, a partial override can stick.
    app.setStyleSheet(QString());
    applyTheme(app, mode);
}

} // namespace IPView::Theme

#endif // THEME_H
