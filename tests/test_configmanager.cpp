// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.15.5 — test_configmanager.cpp
//
//  Unit tests for ConfigManager::loadClampedInt() (Phase 4-B).
//  These tests touch the real QSettings layer so they need to
//  be run with a clean user config — the test suite uses a
//  throwaway organisation/application name in setUp() to keep
//  them isolated from the production INI file at
//  ~/.config/IPView/IPView.conf.
// ═══════════════════════════════════════════════════════════════════════════════

#include "TestFramework.hpp"
#include "ConfigManager.h"
#include <QSettings>
#include <QCoreApplication>
#include <QStringLiteral>

using IPView::Config::Manager;

// Helper: Manager's static sSettings uses org="IPView" /
// app="IPView". Setting the same on the QCoreApplication makes
// the test's QSettings s; queries land in the same INI file
// Manager::loadClampedInt() reads from.
struct ScopedOrgApp {
    ScopedOrgApp() noexcept {
        QCoreApplication::setOrganizationName(QStringLiteral("IPView"));
        QCoreApplication::setApplicationName(QStringLiteral("IPView"));
    }
};

IPVIEW_TEST_CASE(loadClampedInt_returns_default_when_missing,
    ScopedOrgApp _org;
    QSettings s;
    s.remove("Test/DefinitelyMissing");
    int v = IPView::Config::Manager::loadClampedInt(
        QLatin1StringView("Test/DefinitelyMissing"),
        42, 0, 100);
    IPVIEW_CHECK_EQ(v, 42);
)

IPVIEW_TEST_CASE(loadClampedInt_passes_through_in_range,
    ScopedOrgApp _org;
    QSettings s;
    s.setValue("Test/ClampedIntValue", 50);
    int v = IPView::Config::Manager::loadClampedInt(
        QLatin1StringView("Test/ClampedIntValue"),
        0, 0, 100);
    IPVIEW_CHECK_EQ(v, 50);
    s.remove("Test/ClampedIntValue");
)

IPVIEW_TEST_CASE(loadClampedInt_clamps_below_minimum,
    ScopedOrgApp _org;
    QSettings s;
    s.setValue("Test/ClampedIntValue", -10);
    int v = IPView::Config::Manager::loadClampedInt(
        QLatin1StringView("Test/ClampedIntValue"),
        0, 0, 100);
    IPVIEW_CHECK_EQ(v, 0);
    s.remove("Test/ClampedIntValue");
)

IPVIEW_TEST_CASE(loadClampedInt_clamps_above_maximum,
    ScopedOrgApp _org;
    QSettings s;
    s.setValue("Test/ClampedIntValue", 9999);
    int v = IPView::Config::Manager::loadClampedInt(
        QLatin1StringView("Test/ClampedIntValue"),
        0, 0, 100);
    IPVIEW_CHECK_EQ(v, 100);
    s.remove("Test/ClampedIntValue");
)

IPVIEW_TEST_CASE(loadClampedInt_writes_back_clamped_value,
    ScopedOrgApp _org;
    QSettings s;
    s.setValue("Test/ClampedIntValue", 9999);
    (void)IPView::Config::Manager::loadClampedInt(
        QLatin1StringView("Test/ClampedIntValue"),
        0, 0, 100);
    int after = s.value("Test/ClampedIntValue").toInt();
    IPVIEW_CHECK_EQ(after, 100);
    s.remove("Test/ClampedIntValue");
)
