// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.7.0 — main.cpp
//  C++26 (ISO/IEC 14882:2026) + Qt 6.11
//  consteval for compile-time metadata
//  Single-instance guard via QSharedMemory + QLocalServer
//  Public Domain — No License — No Restrictions
// ═══════════════════════════════════════════════════════════════════════════════

#include <QApplication>
#include <QStyleFactory>
#include <QIcon>
#include <QSharedMemory>
#include <QLocalServer>
#include <QLocalSocket>
#include <QMessageBox>
#include <QDebug>
#include "MainWindow.h"
#include "Theme.h"
#include "DatabaseModule.h"
#include "ConfigManager.h"
#include "Logger.h"

// ── Compile-time constant application metadata ─────────────────────────────
//  C++26 consteval: Guaranteed evaluation at compile time
consteval auto appName()        noexcept { return "IPView"; }
    consteval auto appVersion()     noexcept { return "2.9.1"; }
consteval auto appOrgName()     noexcept { return "IPView"; }
consteval auto appOrgDomain()   noexcept { return "ipview.local"; }
consteval auto appDisplayName() noexcept { return "IP View Pro"; }

// ── Unique single-instance keys ─────────────────────────────────────────
//  C++26 consteval: guaranteed compile-time evaluation
consteval auto sharedMemKey()   noexcept { return "IPView-Pro-SingleInstance"; }
consteval auto localServerKey() noexcept { return "IPView-Pro-LocalServer"; }

// ── Tooltip stylesheet ──────────────────────────────────────────────────────
[[nodiscard]] static QString tooltipStyleSheet()
{
    return QStringLiteral(
        "QToolTip { background-color: %1; color: %2; "
        "border: 1px solid %3; border-radius: 4px; padding: 4px; }"
    ).arg(C_BG_ELEVATED, C_TEXT, C_ACCENT);
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Single-instance guard
//  Checks at startup whether an IPView instance is already running.
//  If yes: brings the existing instance to the foreground and
//          terminates the new instance immediately.
//  If no: creates shared memory, starts QLocalServer.
// ═══════════════════════════════════════════════════════════════════════════════
int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    // ── Application metadata — resolved at compile time ──────────────
    QApplication::setApplicationName(    QLatin1StringView(appName()));
    QApplication::setApplicationVersion( QLatin1StringView(appVersion()));
    QApplication::setOrganizationName(   QLatin1StringView(appOrgName()));
    QApplication::setOrganizationDomain( QLatin1StringView(appOrgDomain()));
    QApplication::setApplicationDisplayName(QLatin1StringView(appDisplayName()));

    // ── Fusion theme + global stylesheet ──────────────────────────────
    app.setStyle(QStyleFactory::create("Fusion"));
    app.setStyleSheet(appStyleSheet() + QLatin1Char('\n') + tooltipStyleSheet());

    // ════════════════════════════════════════════════════════════════════
    //  CONFIG MANAGER (per-user, XDG-konform ~/.config/IPView/IPView.conf)
    // ════════════════════════════════════════════════════════════════════
    IPView::Config::Manager::initialize();

    // ════════════════════════════════════════════════════════════════════
    //  SINGLE INSTANCE CHECK
    // ════════════════════════════════════════════════════════════════════
    QSharedMemory sharedMem{QLatin1StringView(sharedMemKey())};

    // Attempt to attach to existing shared memory
    if (sharedMem.attach(QSharedMemory::ReadOnly)) {
        // ── An instance is already running ──────────────────────────────
        //  Detach the attachment (probe only)
        sharedMem.detach();

        // Send "show" command to the running instance via QLocalSocket
        QLocalSocket socket;
        socket.connectToServer(QLatin1StringView(localServerKey()));

        if (socket.waitForConnected(500)) {
            socket.write("show\n");
            socket.flush();
            socket.waitForBytesWritten(300);
            socket.disconnectFromServer();
        }

        // Terminate the second instance
        return 0;
    }

    // ── First instance: create shared memory ──────────────────────────
    if (sharedMem.create(1, QSharedMemory::ReadWrite)) {
        // Set the shared memory signal byte
        *static_cast<char*>(sharedMem.data()) = 1;
    }

    // ── QLocalServer: listens for "show" commands from other instances ────
    //  Remove any potentially leftover server from a previous run
    QLocalServer::removeServer(QLatin1StringView(localServerKey()));

    QLocalServer localServer;
    localServer.listen(QLatin1StringView(localServerKey()));

    // ════════════════════════════════════════════════════════════════════════
    //  DATABASE INITIALIZATION
    // ════════════════════════════════════════════════════════════════════════
    if (!IPView::Storage::DatabaseModule::init()) {
        IPView::Logger::warn("main: DatabaseModule initialization failed — "
                 "IP history will not be persisted.");
    } else {
        IPView::Logger::info("main: DatabaseModule initialized successfully.");
    }

    // ════════════════════════════════════════════════════════════════════
    //  MAIN WINDOW
    // ════════════════════════════════════════════════════════════════════
    MainWindow window;

    // ── On "show" command from second instance: activate window ───
    QObject::connect(&localServer, &QLocalServer::newConnection, [&]() {
        QLocalSocket *client = localServer.nextPendingConnection();
        if (client) {
            client->waitForReadyRead(200);
            client->deleteLater();
        }
        // Bring window to the foreground
        window.showNormal();
        window.activateWindow();
        window.raise();
    });

    window.setWindowIcon(QIcon(QStringLiteral(":/icon.svg")));
    window.show();

    int const exitCode = app.exec();
    return exitCode;
}
