# Maintainer: IPView Developers
# Contributor: Public Domain
#
# ═══════════════════════════════════════════════════════════════════════════════
# IPView Pro v2.7.0 — PKGBUILD for Arch Linux
#
# Build & install:
#   makepkg -si
#
# Build only (package):
#   makepkg -f
#
# Clean:
#   makepkg -c
#
# Remove:
#   sudo pacman -R ipview-pro
#
# ═══════════════════════════════════════════════════════════════════════════════

pkgname=ipview-pro
pkgver=2.7.0
pkgrel=1
pkgdesc="Professional IP monitoring & network analysis tool — Qt 6.11, C++26"
arch=('x86_64')
url="https://github.com/USER/IPView"
license=('LicenseRef-Public-Domain')
depends=(
    'qt6-base>=6.11'
    'qt6-svg>=6.11'
    'hicolor-icon-theme'
    'desktop-file-utils'
)
makedepends=(
    'cmake>=3.28'
    'gcc>=14'
    'glibc'
)
optdepends=(
    'speedtest-cli: Internet speed test functionality'
    'iperf3: Network throughput measurement'
    'traceroute: Network hop tracing'
)
install="${pkgname}.install"

# ── Local source build — run makepkg from project root ───────────────────
#  This PKGBUILD expects to be executed from within the cloned repository.
#  All source files are listed explicitly to maintain reproducible builds.
source=(
    "icon.svg"
    "ipview.desktop"
    "saturate_fix.h"
    "CMakeLists.txt"
    "main.cpp"
    "AboutTab.cpp"
    "AboutTab.h"
    "DataNormalizer.h"
    "FlagLoader.cpp"
    "FlagLoader.h"
    "HistoryTab.cpp"
    "HistoryTab.h"
    "Iperf3Window.cpp"
    "Iperf3Window.h"
    "MainWindow.cpp"
    "MainWindow.h"
    "NetworkManager.cpp"
    "NetworkManager.h"
    "SecurityUtil.h"
    "SpeedtestTab.cpp"
    "SpeedtestTab.h"
    "Theme.h"
    "ToolsTab.cpp"
    "ToolsTab.h"
    "TracerouteTab.cpp"
    "TracerouteTab.h"
    "WhoisManager.cpp"
    "WhoisManager.h"
    "WhoisTab.cpp"
    "WhoisTab.h"
    "resources.qrc"
    "svgs/camera.svg"
    "svgs/chart-bar.svg"
    "svgs/checkmark.svg"
    "svgs/clipboard.svg"
    "svgs/floppy-disk.svg"
    "svgs/info.svg"
    "svgs/lightning.svg"
    "svgs/scroll.svg"
    "svgs/search.svg"
    "svgs/warning.svg"
    "svgs/wastebasket.svg"
    "svgs/wrench.svg"
    "README.md"
    "CHANGELOG.md"
)
sha256sums=('SKIP')

build() {
    cd "${srcdir}"
    cmake \
        -S . \
        -B build \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr \
        -DCMAKE_CXX_STANDARD=26 \
        -Wno-dev
    cmake --build build -j"$(nproc)"
}

package() {
    cd "${srcdir}"

    # ── Binary ──────────────────────────────────────────────────────────
    install -Dm755 build/IPView "${pkgdir}/usr/bin/ipview-pro"

    # ── Desktop entry (start menu) ─────────────────────────────────────
    install -Dm644 ipview.desktop "${pkgdir}/usr/share/applications/ipview-pro.desktop"

    # ── Application icon ───────────────────────────────────────────────
    install -Dm644 icon.svg "${pkgdir}/usr/share/icons/hicolor/scalable/apps/ipview-pro.svg"
    install -Dm644 icon.svg "${pkgdir}/usr/share/pixmaps/ipview-pro.svg"

    # ── SVG toolbar icons (12× 512×512 px) ────────────────────────────
    install -dm755 "${pkgdir}/usr/share/ipview-pro/svgs"
    install -Dm644 svgs/*.svg "${pkgdir}/usr/share/ipview-pro/svgs/"

    # ── Build-time polyfill (needed on GCC 16.1) ──────────────────────
    install -Dm644 saturate_fix.h "${pkgdir}/usr/share/ipview-pro/saturate_fix.h"

    # ── Documentation ──────────────────────────────────────────────────
    install -Dm644 README.md   "${pkgdir}/usr/share/doc/ipview-pro/README.md"
    install -Dm644 CHANGELOG.md "${pkgdir}/usr/share/doc/ipview-pro/CHANGELOG.md"
}
