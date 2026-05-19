// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.7.0 — Theme.h
//  Central design token system for Qt Style Sheets (QSS).
//  All colors, spacings, radii, and shadows in one place.
//
//  C++26 constexpr — guaranteed to be resolved at compile time.
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#ifndef THEME_H
#define THEME_H

#include <QString>
#include <QStringBuilder>   // efficient string concatenation

// ═══════════════════════════════════════════════════════════════════════════════
//  COLOR PALETTE — OLED Dark Theme
//  Inspired by modern dashboards: dark, rich accents, high contrast.
//
//  Primary: cool tech blue   (#00d4ff → #0077AA)
//  Accent:  signal red        (#e94560 → #c0392b)
//  Success: vibrant green    (#00ff88 → #00cc6a)
//  Base:    deep black        (#0f0f1a → #060612)
// ═══════════════════════════════════════════════════════════════════════════════

// ── Base colors ─────────────────────────────────────────────────────────────
inline constexpr auto C_BG          = "#0f0f1a";
inline constexpr auto C_BG_ELEVATED = "#1a1a2e";
inline constexpr auto C_BG_SUNKEN   = "#050510";
inline constexpr auto C_BG_HOVER    = "#22223a";

inline constexpr auto C_SURFACE     = "#1a1a2e";
inline constexpr auto C_SURFACE_HVR = "#252540";
inline constexpr auto C_SURFACE_ACT = "#303050";

inline constexpr auto C_BORDER      = "#2a2a3e";
inline constexpr auto C_BORDER_HVR  = "#e94560";
inline constexpr auto C_BORDER_FOC  = "#00d4ff";

inline constexpr auto C_TEXT        = "#ffffff";
inline constexpr auto C_TEXT_SEC    = "#a0a0b8";
inline constexpr auto C_TEXT_DIM    = "#666680";
inline constexpr auto C_TEXT_MUTED  = "#888888";

inline constexpr auto C_ACCENT      = "#e94560";
inline constexpr auto C_ACCENT_HVR  = "#ff5a7a";
inline constexpr auto C_ACCENT_ACT  = "#c0392b";
inline constexpr auto C_PRIMARY     = "#00d4ff";
inline constexpr auto C_PRIMARY_HVR = "#33ddff";
inline constexpr auto C_PRIMARY_ACT = "#0099cc";
inline constexpr auto C_SUCCESS     = "#00ff88";
inline constexpr auto C_SUCCESS_HVR = "#33ffaa";
inline constexpr auto C_WARNING     = "#ffaa00";
inline constexpr auto C_ERROR       = "#ff4444";
inline constexpr auto C_INFO        = "#00d4ff";

// ── Spacing ────────────────────────────────────────────────────────────────
// Tabs: compact — all 8 tabs fit without scrolling
inline constexpr auto PADDING_TAB   = "6px 12px";
inline constexpr auto PADDING_TAB_L = "6px 18px";   // for optional wider tabs
inline constexpr auto PADDING_BTN   = "8px 20px";
inline constexpr auto PADDING_BTN_S = "6px 14px";
inline constexpr auto PADDING_INP   = "8px 12px";
inline constexpr auto PADDING_CRD   = "20px";
inline constexpr auto PADDING_M     = "10px";

// ── Radii ──────────────────────────────────────────────────────────────────
inline constexpr auto RADIUS_SM   = "4px";
inline constexpr auto RADIUS_MD   = "6px";
inline constexpr auto RADIUS_LG   = "8px";
inline constexpr auto RADIUS_XL   = "12px";
inline constexpr auto RADIUS_2XL  = "14px";

// ═══════════════════════════════════════════════════════════════════════════════
//  GLOBAL APP STYLESHEET
//  Set once in main.cpp via app.setStyleSheet().
//  Covers all widget types: buttons, inputs, scrollbars, tabs, etc.
// ═══════════════════════════════════════════════════════════════════════════════
[[nodiscard]]
inline QString appStyleSheet() noexcept
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
        "  transition: background 0.12s cubic-bezier(0.4,0,0.2,1),"
        "              color 0.12s cubic-bezier(0.4,0,0.2,1),"
        "              border-bottom 0.12s cubic-bezier(0.4,0,0.2,1);"
        "  min-width: 0;"  // allow shrinking, no scroll buttons
        "}"
        "QTabBar::tab:selected {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "    stop:0 %4, stop:1 %19);"
        "  color: %2;"
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
        "  transition: background 0.12s ease, border-color 0.12s ease,"
        "              color 0.12s ease;"
        "}"
        "QPushButton:hover {"
        "  background-color: %9; border-color: %13;"
        "}"
        "QPushButton:pressed {"
        "  background-color: %14;"
        "}"
        "QPushButton:disabled {"
        "  color: %15; border-color: %16; background-color: %17;"
        "}"
        // ── Accent button (Refresh, Lookup, Ping, GO) ──────────────────
        "QPushButton[accent=\"true\"] {"
        "  background-color: %4; color: %2; border: none;"
        "}"
        "QPushButton[accent=\"true\"]:hover {"
        "  background-color: %18;"
        "}"
        "QPushButton[accent=\"true\"]:pressed {"
        "  background-color: %19;"
        "}"
        // ── QLineEdit / QTextEdit / QPlainTextEdit ─────────────────────
        "QLineEdit {"
        "  background-color: %3; color: %2;"
        "  border: 1px solid %10; border-radius: %11;"
        "  padding: %20; selection-background-color: %4;"
        "  transition: border-color 0.12s ease;"
        "}"
        "QLineEdit:focus {"
        "  border-color: %21;"
        "}"
        "QLineEdit:hover:!focus {"
        "  border-color: %13;"
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
        // ── QProgressBar ───────────────────────────────────────────────
        "QProgressBar {"
        "  background: %3; border-radius: 5px; border: none;"
        "  color: %2; font-size: 11px; font-weight: bold;"
        "  text-align: center;"
        "}"
        "QProgressBar::chunk {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        "    stop:0 %4, stop:0.5 %21, stop:1 %22);"
        "  border-radius: 5px;"
        "}"
        // ── QTableWidget ───────────────────────────────────────────────
        "QTableWidget {"
        "  background-color: %3; color: %2;"
        "  gridline-color: %10; border: none; font-size: 13px;"
        "  alternate-background-color: %23;"
        "}"
        "QTableWidget::item { padding: 6px 8px; }"
        "QTableWidget::item:selected {"
        "  background-color: %4; color: %2;"
        "}"
        "QHeaderView::section {"
        "  background-color: %3; color: %7;"
        "  border: none; border-bottom: 1px solid %10;"
        "  padding: 8px; font-weight: bold; font-size: 11px;"
        "}"
        // ── QScrollBar (dark theme) ────────────────────────────────────
        "QScrollBar:vertical {"
        "  background: %1; width: 10px; margin: 0;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: %10; border-radius: 5px; min-height: 30px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "  background: %15;"
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
        "  background: %15;"
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
        "QMenu::item:selected { background-color: %4; color: %2; }"
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
    )
        .arg(C_BG,          C_TEXT,        C_BG_ELEVATED,  //  1  2  3
             C_ACCENT,       RADIUS_SM,     RADIUS_LG,      //  4  5  6
             C_TEXT_DIM,     PADDING_TAB,   C_BG_HOVER,     //  7  8  9
             C_BORDER,       RADIUS_MD,     PADDING_BTN,     // 10 11 12
             C_ACCENT_HVR,   C_ACCENT_ACT,  C_TEXT_MUTED,    // 13 14 15
             C_BORDER,       C_BG_SUNKEN,   C_ACCENT_HVR,    // 16 17 18
             C_ACCENT_ACT,   PADDING_INP,   C_PRIMARY,       // 19 20 21
             C_SUCCESS,      C_BG_ELEVATED, PADDING_CRD)     // 22 23 24
        ;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  QUICK HELPERS — for widget-specific individual styles
// ═══════════════════════════════════════════════════════════════════════════════

// ── Buttons: accent (red) ──────────────────────────────────────────────────
[[nodiscard]] inline QString btnAccentStyle() noexcept
{
    return QStringLiteral(
        "QPushButton { background-color: %1; color: %2; border: none;"
        "  border-radius: %3; padding: %4; font-weight: bold;"
        "  transition: background 0.12s ease; }"
        "QPushButton:hover { background-color: %5; }"
        "QPushButton:pressed { background-color: %6; }"
    ).arg(C_ACCENT, C_TEXT, RADIUS_MD, PADDING_BTN, C_ACCENT_HVR, C_ACCENT_ACT);
}

// ── Buttons: secondary (outline) ────────────────────────────────────────────
[[nodiscard]] inline QString btnSecondaryStyle() noexcept
{
    return QStringLiteral(
        "QPushButton { background-color: %1; color: %2;"
        "  border: 1px solid %3; border-radius: %4; padding: %5;"
        "  transition: background 0.12s ease, border-color 0.12s ease; }"
        "QPushButton:hover { background-color: %6; border-color: %3; }"
        "QPushButton:pressed { background-color: %7; }"
    ).arg(C_BG_ELEVATED, C_TEXT, C_BORDER, RADIUS_MD, PADDING_BTN,
          C_BG_HOVER, C_SURFACE_ACT);
}

// ── Buttons: small (outline) ───────────────────────────────────────────────
[[nodiscard]] inline QString btnSmallStyle() noexcept
{
    return QStringLiteral(
        "QPushButton { background-color: %1; color: %2;"
        "  border: 1px solid %3; border-radius: %4; padding: %5; font-size: 11px;"
        "  transition: background 0.12s ease, border-color 0.12s ease; }"
        "QPushButton:hover { background-color: %6; border-color: %3; }"
    ).arg(C_BG_ELEVATED, C_TEXT, C_BORDER, RADIUS_MD, PADDING_BTN_S, C_BG_HOVER);
}

// ── Input field (LineEdit) ─────────────────────────────────────────────────
[[nodiscard]] inline QString inputStyle() noexcept
{
    return QStringLiteral(
        "QLineEdit { background-color: %1; color: %2;"
        "  border: 1px solid %3; border-radius: %4; padding: %5;"
        "  transition: border-color 0.12s ease; }"
        "QLineEdit:focus { border-color: %6; }"
        "QLineEdit:hover:!focus { border-color: %7; }"
    ).arg(C_BG_ELEVATED, C_TEXT, C_BORDER, RADIUS_MD, PADDING_INP,
          C_PRIMARY, C_ACCENT);
}

// ── ComboBox ──────────────────────────────────────────────────────────────
[[nodiscard]] inline QString comboStyle() noexcept
{
    return QStringLiteral(
        "QComboBox { background-color: %1; color: %2;"
        "  border: 1px solid %3; border-radius: %4; padding: %5; }"
        "QComboBox:hover { border-color: %6; }"
        "QComboBox::drop-down { border: none; width: 24px; }"
        "QComboBox::down-arrow {"
        "  image: none; border-left: 5px solid transparent;"
        "  border-right: 5px solid transparent;"
        "  border-top: 6px solid %2; margin-right: 6px; }"
        "QComboBox QAbstractItemView {"
        "  background-color: %1; color: %2;"
        "  selection-background-color: %7; border: 1px solid %3; outline: none; }"
    ).arg(C_BG_ELEVATED, C_TEXT, C_BORDER, RADIUS_MD, PADDING_INP,
          C_ACCENT, C_ACCENT);
}

// ── Output / monospace area ──────────────────────────────────────────────────
[[nodiscard]] inline QString monoStyle() noexcept
{
    return QStringLiteral(
        "QTextEdit, QPlainTextEdit { background-color: %1; color: %2;"
        "  border: none; border-radius: %3; padding: %4;"
        "  font-family: 'JetBrains Mono', 'Cascadia Code', 'Consolas', monospace;"
        "  font-size: 12px; }"
    ).arg(C_BG_SUNKEN, C_SUCCESS, RADIUS_LG, PADDING_M);
}

// ── Card / Frame ──────────────────────────────────────────────────────────
[[nodiscard]] inline QString cardStyle() noexcept
{
    return QStringLiteral(
        "QFrame { background-color: %1; border-radius: %2;"
        "  border: 1px solid %3; padding: %4; }"
        "QFrame:hover { border-color: %5; }"
    ).arg(C_BG_ELEVATED, RADIUS_XL, C_BORDER, PADDING_CRD, C_ACCENT);
}

// ── Status label ──────────────────────────────────────────────────────────
[[nodiscard]] inline QString statusLabelStyle() noexcept
{
    return QStringLiteral(
        "QLabel { color: %1; font-size: 10px; padding: 5px; }"
    ).arg(C_ACCENT);
}

// ── Online label (green) ────────────────────────────────────────────────────
[[nodiscard]] inline QString onlineLabelStyle() noexcept
{
    return QStringLiteral(
        "QLabel { color: %1; font-weight: bold; }"
    ).arg(C_SUCCESS);
}

#endif // THEME_H
