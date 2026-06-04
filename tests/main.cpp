// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.15.4 — tests/main.cpp
//
//  Test binary entry point. Pulls in all test translation units and
//  drives the IPView::Test::registry() cases.
// ═══════════════════════════════════════════════════════════════════════════════

#include "TestFramework.hpp"

#include <cstdio>

int main()
{
    using namespace IPView::Test;
    std::fprintf(stderr, "IPView Pro v2.15.4 — test runner\n");
    std::fprintf(stderr, "─────────────────────────────────\n");

    for (auto const& c : registry()) {
        c.run();
    }

    auto const& r = result();
    std::fprintf(stderr, "─────────────────────────────────\n");
    std::fprintf(stderr,
        "Cases:  %d total, %d passed, %d failed\n"
        "Checks: %d total, %d failed\n",
        r.totalCases, r.passedCases, r.failedCases,
        r.totalChecks, r.failedChecks);
    return r.failedCases == 0 ? 0 : 1;
}
