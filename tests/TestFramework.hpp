// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.15.4 — tests/TestFramework.hpp
//
//  A zero-dependency, header-only unit-test mini-framework. The full
//  test suite uses doctest in the upstream plan, but the build is
//  100 % offline on this machine, so we ship a minimal stand-in
//  that covers exactly the assertions we need:
//
//    IPVIEW_TEST_CASE(name, ...)   — opens a complete function whose
//                                    body is the variadic argument.
//                                    The user writes statements,
//                                    separated by ';', between the
//                                    parentheses, no braces required.
//    IPVIEW_CHECK(expr)            — evaluates expr, logs failure, keeps running.
//    IPVIEW_REQUIRE(expr)          — evaluates expr, fails fast (returns).
//    IPVIEW_CHECK_EQ(a, b)         — uses operator==, logs both sides.
//    IPVIEW_CHECK_THROWS_AS(e, E)  — catches E (or a derived class).
//    IPVIEW_CHECK_NOTHROW(expr)    — succeeds when expr does not throw.
//
//  Exit code 0 on success, 1 on any failure. Failures are written to
//  stderr in a compact form. No colours, no dependencies, no surprises.
//
//  This file is public domain, no restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#ifndef IPVIEW_TEST_FRAMEWORK_HPP
#define IPVIEW_TEST_FRAMEWORK_HPP

#include <cstdio>
#include <cstdlib>
#include <exception>
#include <sstream>
#include <string>
#include <string_view>
#include <source_location>
#include <vector>

namespace IPView::Test {

struct Case {
    std::string_view name;
    void (*run)();
    std::source_location loc;
};

struct Result {
    int totalCases   = 0;
    int passedCases  = 0;
    int failedCases  = 0;
    int totalChecks  = 0;
    int failedChecks = 0;
};

inline std::vector<Case>& registry() noexcept
{
    static std::vector<Case> r;
    return r;
}

inline Result& result() noexcept
{
    static Result r;
    return r;
}

namespace detail {

inline void logCheckFailure(std::string_view expr, std::source_location loc)
{
    ++result().failedChecks;
    std::fprintf(stderr,
        "  [FAIL] CHECK(%.*s)  at  %s:%u\n",
        static_cast<int>(expr.size()), expr.data(),
        loc.file_name(), loc.line());
}

inline void logRequireFailure(std::string_view expr, std::source_location loc)
{
    ++result().failedChecks;
    std::fprintf(stderr,
        "  [FAIL] REQUIRE(%.*s)  at  %s:%u  →  returning early\n",
        static_cast<int>(expr.size()), expr.data(),
        loc.file_name(), loc.line());
}

// Generic value-to-string converter. The free function keeps the
// failure log readable for built-in types, QString (Qt), and
// std::string alike. Any type with an ostream operator<< works.
template <typename T>
inline std::string formatValue(T const& v) noexcept
{
    if constexpr (std::is_arithmetic_v<T>) {
        return std::to_string(v);
    } else if constexpr (requires (T t) { t.toUtf8(); }) {
        // QString, QByteArray, etc.
        return std::string{v.toUtf8().constData()};
    } else if constexpr (requires (T t) { std::string(t); }) {
        return std::string{v};
    } else if constexpr (requires (std::ostream& os, T t) { os << t; }) {
        std::ostringstream os;
        os << v;
        return os.str();
    } else {
        return "<unprintable>";
    }
}

template <typename A, typename B>
inline void logEqFailure(std::string_view label, A const& a, B const& b,
                         std::source_location loc)
{
    ++result().failedChecks;
    std::fprintf(stderr,
        "  [FAIL] %.*s  at  %s:%u\n"
        "         lhs = %s\n"
        "         rhs = %s\n",
        static_cast<int>(label.size()), label.data(),
        loc.file_name(), loc.line(),
        formatValue(a).c_str(),
        formatValue(b).c_str());
}

struct CaseScope {
    bool ok = true;
    std::string_view name;
    std::source_location loc;

    explicit CaseScope(std::string_view n, std::source_location l)
        : name(n), loc(l)
    {
        ++result().totalCases;
        std::fprintf(stderr, "── %.*s  (%s:%u) ──\n",
            static_cast<int>(name.size()), name.data(),
            loc.file_name(), loc.line());
    }
    ~CaseScope()
    {
        if (ok) {
            ++result().passedCases;
            std::fprintf(stderr, "   ok\n");
        } else {
            ++result().failedCases;
        }
    }
};

} // namespace detail

struct Registrar {
    Registrar(std::string_view n, void (*fn)(), std::source_location l)
    {
        registry().push_back(Case{n, fn, l});
    }
};

} // namespace IPView::Test

// ── Macros ──────────────────────────────────────────────────────────────
//  IPVIEW_TEST_CASE(name, body...) creates a complete function whose
//  body is the variadic argument. The body must be a sequence of
//  statements separated by ';', enclosed in a do-while(0) inside the
//  generated function so the user's statements stay in a single
//  block. The macro opens and closes its own braces; the user does
//  not write { or }.
//
//  Example:
//      IPVIEW_TEST_CASE(addition_works,
//          IPVIEW_CHECK_EQ(1 + 1, 2);
//          IPVIEW_CHECK_EQ(2 * 3, 6);
//      )
//
#define IPVIEW_TEST_CASE(name, ...)                                             \
    static void ipview_test_##name();                                           \
    static ::IPView::Test::Registrar ipview_reg_##name{                         \
        #name, &ipview_test_##name, std::source_location::current()};           \
    static void ipview_test_##name()                                            \
    {                                                                           \
        ::IPView::Test::detail::CaseScope ipview_case{                          \
            #name, std::source_location::current()};                            \
        do { __VA_ARGS__; } while (0);                                           \
    }

#define IPVIEW_CHECK(expr)                                                      \
        do {                                                                    \
            ++::IPView::Test::result().totalChecks;                             \
            if (!(expr)) {                                                      \
                ::IPView::Test::detail::logCheckFailure(#expr,                  \
                    std::source_location::current());                           \
                ipview_case.ok = false;                                         \
            }                                                                   \
        } while (0)

#define IPVIEW_REQUIRE(expr)                                                    \
        do {                                                                    \
            ++::IPView::Test::result().totalChecks;                             \
            if (!(expr)) {                                                      \
                ::IPView::Test::detail::logRequireFailure(#expr,                \
                    std::source_location::current());                           \
                ipview_case.ok = false;                                         \
                return;                                                         \
            }                                                                   \
        } while (0)

#define IPVIEW_CHECK_EQ(a, b)                                                   \
        do {                                                                    \
            ++::IPView::Test::result().totalChecks;                             \
            auto const& _ipview_a = (a);                                        \
            auto const& _ipview_b = (b);                                        \
            if (!(_ipview_a == _ipview_b)) {                                    \
                ::IPView::Test::detail::logEqFailure(                           \
                    #a " == " #b, _ipview_a, _ipview_b,                         \
                    std::source_location::current());                           \
                ipview_case.ok = false;                                         \
            }                                                                   \
        } while (0)

#define IPVIEW_CHECK_THROWS_AS(expr, Exc)                                       \
        do {                                                                    \
            ++::IPView::Test::result().totalChecks;                             \
            try { expr;                                                         \
                ::IPView::Test::detail::logCheckFailure(                        \
                    "expected " #expr " to throw " #Exc " (no throw observed)", \
                    std::source_location::current());                           \
                ipview_case.ok = false;                                         \
            } catch (Exc const&) {                                              \
            } catch (...) {                                                     \
                ::IPView::Test::detail::logCheckFailure(                        \
                    #expr " threw a type not derived from " #Exc,               \
                    std::source_location::current());                           \
                ipview_case.ok = false;                                         \
            }                                                                   \
        } while (0)

#define IPVIEW_CHECK_NOTHROW(expr)                                              \
        do {                                                                    \
            ++::IPView::Test::result().totalChecks;                             \
            try { expr; } catch (...) {                                         \
                ::IPView::Test::detail::logCheckFailure(                        \
                    #expr " unexpectedly threw",                                \
                    std::source_location::current());                           \
                ipview_case.ok = false;                                         \
            }                                                                   \
        } while (0)

#endif // IPVIEW_TEST_FRAMEWORK_HPP
