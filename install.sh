#!/bin/bash
# ═══════════════════════════════════════════════════════════════════════════════
#  IPView Pro v2.7.0 – Install / Uninstall Script
#  Usage:
#    sudo ./install.sh            # Install
#    sudo ./install.sh uninstall  # Uninstall
#
#  Requires: build.sh must have been run successfully first.
#  Public Domain — No License — No Restrictions.
# ═══════════════════════════════════════════════════════════════════════════════

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BINARY="${SCRIPT_DIR}/build/IPView"
PREFIX="${DESTDIR:-}/usr"
BINDIR="${PREFIX}/bin"
APPDIR="${PREFIX}/share/applications"
ICONDIR="${PREFIX}/share/icons/hicolor/scalable/apps"
PIXMAPDIR="${PREFIX}/share/pixmaps"
DATADIR="${PREFIX}/share/ipview-pro"
DOCDIR="${PREFIX}/share/doc/ipview-pro"
APPNAME="ipview-pro"

# ═══════════════════════════════════════════════════════════════════════════════
install_app() {
    echo "╔═══════════════════════════════════════════════╗"
    echo "║     IPView Pro — Installing                  ║"
    echo "╚═══════════════════════════════════════════════╝"

    # ── Prüfen ob Binary existiert ─────────────────────────────────────
    if [ ! -f "$BINARY" ]; then
        echo "  ERROR: Binary not found at $BINARY"
        echo "  Run './build.sh' first to compile the application."
        exit 1
    fi

    if [ ! -x "$BINARY" ]; then
        echo "  ERROR: Binary is not executable. Run './build.sh' first."
        exit 1
    fi

    # ── Binary ─────────────────────────────────────────────────────────
    echo "  Installing binary..."
    install -Dm755 "$BINARY" "${BINDIR}/${APPNAME}"
    echo "    → ${BINDIR}/${APPNAME}"

    # ── Desktop entry ──────────────────────────────────────────────────
    if [ -f "${SCRIPT_DIR}/ipview.desktop" ]; then
        echo "  Installing desktop entry..."
        install -Dm644 "${SCRIPT_DIR}/ipview.desktop" "${APPDIR}/${APPNAME}.desktop"
        echo "    → ${APPDIR}/${APPNAME}.desktop"
    else
        echo "  WARNING: ipview.desktop not found — creating default..."
        install -dm755 "${APPDIR}"
        cat > "${APPDIR}/${APPNAME}.desktop" <<-EOF
			[Desktop Entry]
			Name=IPView Pro
			Comment=Professional IP monitoring & network analysis tool
			GenericName=IP Monitor
			Exec=${APPNAME}
			Icon=${APPNAME}
			Type=Application
			Categories=Network;Monitor;Utility;
			Terminal=false
			StartupNotify=true
		EOF
        echo "    → ${APPDIR}/${APPNAME}.desktop (auto-generated)"
    fi

    # ── Application icon ───────────────────────────────────────────────
    echo "  Installing icons..."
    install -Dm644 "${SCRIPT_DIR}/icon.svg" "${ICONDIR}/${APPNAME}.svg"
    echo "    → ${ICONDIR}/${APPNAME}.svg"
    install -Dm644 "${SCRIPT_DIR}/icon.svg" "${PIXMAPDIR}/${APPNAME}.svg"
    echo "    → ${PIXMAPDIR}/${APPNAME}.svg"

    # ── SVG toolbar icons ──────────────────────────────────────────────
    if ls "${SCRIPT_DIR}/svgs/"*.svg 1>/dev/null 2>&1; then
        echo "  Installing toolbar icons..."
        install -dm755 "${DATADIR}/svgs"
        for svg in "${SCRIPT_DIR}/svgs/"*.svg; do
            install -Dm644 "${svg}" "${DATADIR}/svgs/$(basename "${svg}")"
        done
        echo "    → ${DATADIR}/svgs/ ($(ls "${SCRIPT_DIR}/svgs/"*.svg | wc -l) files)"
    fi

    # ── Polyfill (if present) ──────────────────────────────────────────
    if [ -f "${SCRIPT_DIR}/saturate_fix.h" ]; then
        install -Dm644 "${SCRIPT_DIR}/saturate_fix.h" "${DATADIR}/saturate_fix.h"
        echo "    → ${DATADIR}/saturate_fix.h"
    fi

    # ── Documentation ──────────────────────────────────────────────────
    echo "  Installing documentation..."
    install -dm755 "${DOCDIR}"
    for doc in README.md CHANGELOG.md; do
        if [ -f "${SCRIPT_DIR}/${doc}" ]; then
            install -Dm644 "${SCRIPT_DIR}/${doc}" "${DOCDIR}/${doc}"
        fi
    done
    echo "    → ${DOCDIR}/"

    # ── Post-install: update desktop database + icon cache ─────────────
    echo "  Updating desktop database..."
    update-desktop-database -q 2>/dev/null || true
    echo "  Updating icon cache..."
    gtk-update-icon-cache -q -t -f /usr/share/icons/hicolor 2>/dev/null || true

    echo ""
    echo "╔═══════════════════════════════════════════════╗"
    echo "║  IPView Pro installed successfully!          ║"
    echo "╚═══════════════════════════════════════════════╝"
    echo ""
    echo "  Run:  ipview-pro"
    echo "  Menu: IPView Pro → Network → IP Monitor"
    echo ""
}

# ═══════════════════════════════════════════════════════════════════════════════
uninstall_app() {
    echo "╔═══════════════════════════════════════════════╗"
    echo "║     IPView Pro — Uninstalling                ║"
    echo "╚═══════════════════════════════════════════════╝"

    local errors=0

    # Binary
    if [ -f "${BINDIR}/${APPNAME}" ]; then
        rm -f "${BINDIR}/${APPNAME}"
        echo "  ✓ ${BINDIR}/${APPNAME}"
    else
        echo "  - ${BINDIR}/${APPNAME} (not found)"
    fi

    # Desktop entry
    if [ -f "${APPDIR}/${APPNAME}.desktop" ]; then
        rm -f "${APPDIR}/${APPNAME}.desktop"
        echo "  ✓ ${APPDIR}/${APPNAME}.desktop"
    else
        echo "  - ${APPDIR}/${APPNAME}.desktop (not found)"
    fi

    # Icon
    if [ -f "${ICONDIR}/${APPNAME}.svg" ]; then
        rm -f "${ICONDIR}/${APPNAME}.svg"
        echo "  ✓ ${ICONDIR}/${APPNAME}.svg"
    else
        echo "  - ${ICONDIR}/${APPNAME}.svg (not found)"
    fi

    # Pixmap
    if [ -f "${PIXMAPDIR}/${APPNAME}.svg" ]; then
        rm -f "${PIXMAPDIR}/${APPNAME}.svg"
        echo "  ✓ ${PIXMAPDIR}/${APPNAME}.svg"
    else
        echo "  - ${PIXMAPDIR}/${APPNAME}.svg (not found)"
    fi

    # Data directory (icons + polyfill)
    if [ -d "${DATADIR}" ]; then
        rm -rf "${DATADIR}"
        echo "  ✓ ${DATADIR}/"
    else
        echo "  - ${DATADIR}/ (not found)"
    fi

    # Documentation
    if [ -d "${DOCDIR}" ]; then
        rm -rf "${DOCDIR}"
        echo "  ✓ ${DOCDIR}/"
    else
        echo "  - ${DOCDIR}/ (not found)"
    fi

    # ── Post-remove: update databases ─────────────────────────────────
    echo "  Updating desktop database..."
    update-desktop-database -q 2>/dev/null || true
    echo "  Updating icon cache..."
    gtk-update-icon-cache -q -t -f /usr/share/icons/hicolor 2>/dev/null || true

    echo ""
    echo "╔═══════════════════════════════════════════════╗"
    echo "║  IPView Pro uninstalled.                     ║"
    echo "╚═══════════════════════════════════════════════╝"
}

# ═══════════════════════════════════════════════════════════════════════════════
#  MAIN
# ═══════════════════════════════════════════════════════════════════════════════

case "${1:-install}" in
    help|--help|-h)
        echo "Usage: sudo ./install.sh [install|uninstall]"
        echo ""
        echo "  install   (default) Install IPView Pro system-wide"
        echo "  uninstall           Remove IPView Pro from the system"
        exit 0
        ;;
esac

# Root-Prüfung (nicht nötig für help)
if [ "$(id -u)" -ne 0 ]; then
    echo "ERROR: This script must be run as root (sudo)."
    echo "  sudo ./install.sh"
    echo "  sudo ./install.sh uninstall"
    exit 1
fi

case "${1:-install}" in
    install|install-app)
        install_app
        ;;
    uninstall|remove|delete)
        uninstall_app
        ;;
    *)
        echo "ERROR: Unknown command '$1'."
        echo "Usage: sudo ./install.sh [install|uninstall]"
        exit 1
        ;;
esac
