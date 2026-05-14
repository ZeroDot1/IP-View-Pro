// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.7.0 — AboutTab.cpp
//  Displays build information: Compiler, System, Version, Date.
//  No License — Public Domain.
//
//  Macros used:
//    __DATE__, __TIME__ — embedded by the compiler at compile time
//    __VERSION__        — GCC-specific compiler version
//    __cplusplus        — C++ standard version
// ═══════════════════════════════════════════════════════════════════════════════

#include "AboutTab.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QApplication>
#include <QSysInfo>
#include <QFrame>

// ── Compiler Detection ─────────────────────────────────────────────────────
[[nodiscard]]
QString AboutTab::compilerInfo() noexcept
{
    QString name;

#if defined(__clang__)
    name = QStringLiteral("Clang");
#  define COMPILER_VERSION_STR __clang_version__
#elif defined(__GNUC__) || defined(__GNUG__)
    name = QStringLiteral("GCC");
#  define COMPILER_VERSION_STR __VERSION__
#elif defined(_MSC_VER)
    name = QStringLiteral("MSVC");
#  define COMPILER_VERSION_STR ""
#else
    name = QStringLiteral("Unknown");
#  define COMPILER_VERSION_STR ""
#endif

    QString version = QStringLiteral(COMPILER_VERSION_STR);

    // C++ standard
    QString cppStd;
    switch (__cplusplus) {
    case 202101L: cppStd = QStringLiteral("C++23");  break;
    case 202002L: cppStd = QStringLiteral("C++20");  break;
    case 201703L: cppStd = QStringLiteral("C++17");  break;
    case 201402L: cppStd = QStringLiteral("C++14");  break;
    case 201103L: cppStd = QStringLiteral("C++11");  break;
    default:
        if (__cplusplus >= 202600L) cppStd = QStringLiteral("C++26");
        else                        cppStd = QStringLiteral("C++%1").arg(__cplusplus);
        break;
    }

    return QStringLiteral("%1 %2 (%3)").arg(name, version.trimmed(), cppStd);
}

// ── System Information ───────────────────────────────────────────────────
[[nodiscard]]
QString AboutTab::systemInfo() noexcept
{
    return QStringLiteral("%1 %2 (%3)")
        .arg(QSysInfo::kernelType(),          // "linux"
             QSysInfo::kernelVersion(),       // "6.x.y-arch1-1"
             QSysInfo::currentCpuArchitecture()); // "x86_64"
}

// ── Build Info String ──────────────────────────────────────────────────────
[[nodiscard]]
QString AboutTab::buildInfo() noexcept
{
    return QStringLiteral(
        "Compiled : %1 at %2\n"
        "Compiler : %3\n"
        "System   : %4\n"
        "Version  : %5\n"
        "Qt       : %6\n"
        "Terms    : Public Domain"
    )
        .arg(QStringLiteral(__DATE__),
             QStringLiteral(__TIME__),
             compilerInfo(),
             systemInfo(),
             QApplication::applicationVersion(),
             QStringLiteral(QT_VERSION_STR));
}

// ═══════════════════════════════════════════════════════════════════════════════
void AboutTab::setupUI() noexcept
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(40, 40, 40, 40);
    layout->setSpacing(20);

    // ── App Name ──────────────────────────────────────────────────────────
    auto *titleLabel = new QLabel(QStringLiteral("IPView Pro"));
    titleLabel->setFont(QFont(QStringLiteral("Segoe UI"), 28, QFont::Bold));
    titleLabel->setStyleSheet(QStringLiteral("color: #e94560;"));
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);

    // ── Frame with Build Info ─────────────────────────────────────────────
    auto *infoFrame = new QFrame();
    infoFrame->setStyleSheet(QStringLiteral(
        "QFrame { background-color: #1a1a2e; border: 1px solid #2a2a3e; "
        "border-radius: 12px; padding: 24px; }"
    ));

    auto *infoLayout = new QVBoxLayout(infoFrame);
    infoLayout->setSpacing(8);

    QStringList const lines = buildInfo().split(QLatin1Char('\n'));
    for (QString const& line : lines) {
        auto *label = new QLabel(line);
        label->setFont(QFont(QStringLiteral("JetBrains Mono"), 11));
        label->setStyleSheet(QStringLiteral("color: #ffffff; background: transparent;"));
        label->setAlignment(Qt::AlignCenter);

        // Bold label prefix
        if (line.contains(QLatin1Char(':'))) {
            qsizetype const colonIdx = line.indexOf(QLatin1Char(':'));
            QString const key = line.first(colonIdx + 1);
            QString const val = line.mid(colonIdx + 1);
            label->setText(QStringLiteral("<b>%1</b>%2")
                               .arg(key.toHtmlEscaped(), val.toHtmlEscaped()));
        }

        infoLayout->addWidget(label);
    }

    layout->addWidget(infoFrame);

    // ── Public Domain ──────────────────────────────────────────────────
    auto *pdLabel = new QLabel(QStringLiteral(
        "Public Domain — No License — No Restrictions"));
    pdLabel->setFont(QFont(QStringLiteral("Segoe UI"), 10));
    pdLabel->setStyleSheet(QStringLiteral(
        "color: #888888; font-style: italic; background: transparent;"));
    pdLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(pdLabel);

    layout->addStretch();
}

// ═══════════════════════════════════════════════════════════════════════════════
AboutTab::AboutTab(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}
