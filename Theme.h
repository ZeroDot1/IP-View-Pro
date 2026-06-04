// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.15.0 — Theme.h
//  Central design token system for Qt Style Sheets (QSS).
//  All colors, spacings, radii, and shadows are defined here.
//
//  Palette: Teal Amber OLED
//    Primary (Teal):  #00BCD4 — cool, professional, high-contrast on OLED black
//    Accent  (Amber): #FFB300 — warm highlight, badges, interactive focus
//    Base:            #000000 — true OLED black (zero power on OLED displays)
//
//  OLED Support: All backgrounds use pure black (#000000) wherever possible.
//  No light theme is provided — this is an exclusively dark, OLED-optimized UI.
//
//  C++26 inline constexpr — resolved at compile time.
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#ifndef THEME_H
#define THEME_H

#include <QString>
#include <QStringBuilder>   // Efficient string concatenation

// ═══════════════════════════════════════════════════════════════════════════════
//  COLOR PALETTE — Teal Amber OLED Dark Theme
//
//  Primary (Teal):   #00BCD4 → #0097A7  — cool, professional
//  Accent  (Amber):  #FFB300 → #FF8F00  — warm highlight, interactive focus
//  Base (OLED):      #000000            — true OLED black
//  Success (Teal):   #26A69A            — teal-green for positive states
//  Error:            #EF5350            — red for errors and critical alerts
// ═══════════════════════════════════════════════════════════════════════════════

// ── Base colors (OLED black hierarchy) ───────────────────────────────────────
inline constexpr auto C_BG          = "#000000";   // True OLED black — main background
inline constexpr auto C_BG_ELEVATED = "#0D0D0D";   // Slightly elevated surface (cards, panels)
inline constexpr auto C_BG_SUNKEN   = "#000000";   // Sunken areas (code/output regions)
inline constexpr auto C_BG_HOVER    = "#1A1A1A";   // Hover overlay on black

inline constexpr auto C_SURFACE     = "#0D0D0D";   // Default card/pane surface
inline constexpr auto C_SURFACE_HVR = "#1A1A1A";   // Surface hover state
inline constexpr auto C_SURFACE_ACT = "#262626";   // Surface active/pressed state

inline constexpr auto C_BORDER      = "#1E1E1E";   // Subtle border on OLED black
inline constexpr auto C_BORDER_HVR  = "#FFB300";   // Amber border on hover
inline constexpr auto C_BORDER_FOC  = "#00BCD4";   // Teal focus ring

// ── Text colors ───────────────────────────────────────────────────────────────
inline constexpr auto C_TEXT        = "#F5F5F5";   // Primary text — near-white
inline constexpr auto C_TEXT_SEC    = "#B0BEC5";   // Secondary text — blue-grey
inline constexpr auto C_TEXT_DIM    = "#607D8B";   // Dimmed label text
inline constexpr auto C_TEXT_MUTED  = "#546E7A";   // Muted/placeholder text

// ── Teal primary ─────────────────────────────────────────────────────────────
inline constexpr auto C_PRIMARY     = "#00BCD4";   // Teal primary — focus, links, progress
inline constexpr auto C_PRIMARY_HVR = "#26C6DA";   // Teal hover
inline constexpr auto C_PRIMARY_ACT = "#0097A7";   // Teal active/pressed

// ── Amber accent ─────────────────────────────────────────────────────────────
inline constexpr auto C_ACCENT      = "#FFB300";   // Amber accent — badges, selected tabs
inline constexpr auto C_ACCENT_HVR  = "#FFCA28";   // Amber hover
inline constexpr auto C_ACCENT_ACT  = "#FF8F00";   // Amber active/pressed

// ── Semantic colors ───────────────────────────────────────────────────────────
inline constexpr auto C_SUCCESS     = "#26A69A";   // Teal-green — positive/online states
inline constexpr auto C_SUCCESS_HVR = "#4DB6AC";   // Success hover
inline constexpr auto C_WARNING     = "#FFB300";   // Amber — warnings (same as accent)
inline constexpr auto C_ERROR       = "#EF5350";   // Red — errors, critical alerts
inline constexpr auto C_INFO        = "#00BCD4";   // Teal — informational

// ── Spacing ────────────────────────────────────────────────────────────────
// Tabs: compact — all 12 tabs fit without scrolling
inline constexpr auto PADDING_TAB   = "6px 12px";
inline constexpr auto PADDING_TAB_L = "6px 18px";   // Optional wider tab variant
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
//  GLOBAL APP STYLESHEET — Teal Amber OLED Dark Theme
//  Applied once in main.cpp via app.setStyleSheet().
//  Covers all widget types: buttons, inputs, scrollbars, tabs, etc.
//  No light theme. OLED black (#000000) everywhere possible.
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
        // ── QLineEdit / QTextEdit / QPlainTextEdit ─────────────────────
        "QLineEdit {"
        "  background-color: %3; color: %2;"
        "  border: 1px solid %10; border-radius: %11;"
        "  padding: %20; selection-background-color: %21;"
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
//  QUICK HELPERS — Widget-specific inline styles
//  All helpers follow the Teal Amber OLED palette.
// ═══════════════════════════════════════════════════════════════════════════════

// ── Accent button (Amber) ─────────────────────────────────────────────────
[[nodiscard]] inline QString btnAccentStyle() noexcept
{
    return QStringLiteral(
        "QPushButton { background-color: %1; color: %2; border: none;"
        "  border-radius: %3; padding: %4; font-weight: bold; }"
        "QPushButton:hover { background-color: %5; }"
        "QPushButton:pressed { background-color: %6; }"
    ).arg(C_ACCENT, C_BG, RADIUS_MD, PADDING_BTN, C_ACCENT_HVR, C_ACCENT_ACT);
    //   ^Amber    ^Black text for contrast
}

// ── Secondary button (outline / subtle) ────────────────────────────────────
[[nodiscard]] inline QString btnSecondaryStyle() noexcept
{
    return QStringLiteral(
        "QPushButton { background-color: %1; color: %2;"
        "  border: 1px solid %3; border-radius: %4; padding: %5; }"
        "QPushButton:hover { background-color: %6; border-color: %7; }"
        "QPushButton:pressed { background-color: %8; }"
    ).arg(C_BG_ELEVATED, C_TEXT, C_BORDER, RADIUS_MD, PADDING_BTN,
          C_BG_HOVER, C_PRIMARY, C_SURFACE_ACT);
}

// ── Small button (outline, compact) ───────────────────────────────────────
[[nodiscard]] inline QString btnSmallStyle() noexcept
{
    return QStringLiteral(
        "QPushButton { background-color: %1; color: %2;"
        "  border: 1px solid %3; border-radius: %4; padding: %5; font-size: 11px; }"
        "QPushButton:hover { background-color: %6; border-color: %7; }"
    ).arg(C_BG_ELEVATED, C_TEXT, C_BORDER, RADIUS_MD, PADDING_BTN_S,
          C_BG_HOVER, C_PRIMARY);
}

// ── Input field (normal state) ────────────────────────────────────────────
[[nodiscard]] inline QString inputStyle() noexcept
{
    return QStringLiteral(
        "QLineEdit { background-color: %1; color: %2;"
        "  border: 1px solid %3; border-radius: %4; padding: %5; }"
        "QLineEdit:focus { border-color: %6; }"  // Teal focus ring
        "QLineEdit:hover:!focus { border-color: %7; }"  // Amber hover border
    ).arg(C_BG_ELEVATED, C_TEXT, C_BORDER, RADIUS_MD, PADDING_INP,
          C_PRIMARY, C_ACCENT);
}

// ── Input field (error state) ─────────────────────────────────────────────
[[nodiscard]] inline QString inputStyleError() noexcept
{
    return QStringLiteral(
        "QLineEdit { background-color: %1; color: %2;"
        "  border: 1px solid %3; border-radius: %4; padding: %5; }"
        "QLineEdit:focus { border-color: %3; }"
    ).arg(C_BG_ELEVATED, C_TEXT, C_ERROR, RADIUS_MD, PADDING_INP);
}

// ── ComboBox ──────────────────────────────────────────────────────────────
[[nodiscard]] inline QString comboStyle() noexcept
{
    return QStringLiteral(
        "QComboBox { background-color: %1; color: %2;"
        "  border: 1px solid %3; border-radius: %4; padding: %5; }"
        "QComboBox:hover { border-color: %6; }"  // Amber hover border
        "QComboBox::drop-down { border: none; width: 24px; }"
        "QComboBox::down-arrow {"
        "  image: none; border-left: 5px solid transparent;"
        "  border-right: 5px solid transparent;"
        "  border-top: 6px solid %2; margin-right: 6px; }"
        "QComboBox QAbstractItemView {"
        "  background-color: %1; color: %2;"
        "  selection-background-color: %7; selection-color: %8;"
        "  border: 1px solid %3; outline: none; }"
    ).arg(C_BG_ELEVATED, C_TEXT, C_BORDER, RADIUS_MD, PADDING_INP,
          C_ACCENT, C_ACCENT, C_BG);
    //                        ^ Amber selection  ^ Black text on amber
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

// ── Teal primary button (links, navigation actions) ───────────────────────
[[nodiscard]] inline QString btnPrimaryStyle() noexcept
{
    return QStringLiteral(
        "QPushButton { background-color: %1; color: %2; border: none;"
        "  border-radius: %3; padding: %4; font-weight: bold; }"
        "QPushButton:hover { background-color: %5; }"
        "QPushButton:pressed { background-color: %6; }"
    ).arg(C_PRIMARY, C_BG, RADIUS_MD, PADDING_BTN, C_PRIMARY_HVR, C_PRIMARY_ACT);
    //   ^Teal       ^Black text for contrast
}

#endif // THEME_H
