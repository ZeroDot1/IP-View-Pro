// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.15.0 — Contracts.hpp
//  C++26 contracts (P2900) compatibility shim.
//
//  GCC 16.1.1 does not yet implement P2900 (C++26 contracts). The
//  pre/post/assert keywords would generate syntax errors. This
//  header detects whether the compiler supports them and provides
//  IPVIEW_PRE / IPVIEW_POST / IPVIEW_ASSERT macros that:
//    * resolve to real contracts when the feature is available, and
//    * fall back to runtime checks that throw std::invalid_argument
//      (for pre/post) or std::logic_error (for assert) otherwise.
// ═══════════════════════════════════════════════════════════════════════════════

#ifndef IPVIEW_CONTRACTS_HPP
#define IPVIEW_CONTRACTS_HPP

#include <stdexcept>
#include <source_location>
#include <string>

#if defined(__cpp_contracts) && __cpp_contracts >= 202600L
#  define IPVIEW_HAS_CONTRACTS 1
#else
#  define IPVIEW_HAS_CONTRACTS 0
#endif

namespace IPView::Contracts {

[[noreturn]]
inline void contractViolation(const char* kind,
                              const char* expr,
                              std::source_location loc = std::source_location::current())
{
    std::string const message = std::string{loc.function_name()}
                              + " @ " + loc.file_name() + ":" + std::to_string(loc.line())
                              + " — " + kind + "(" + expr + ") failed";
    throw std::logic_error(message);  // pre/post + assert all use the logic family
}

template <typename Expr>
constexpr bool check(Expr&& e) noexcept(noexcept(static_cast<bool>(std::forward<Expr>(e))))
{
    return static_cast<bool>(std::forward<Expr>(e));
}

} // namespace IPView::Contracts

#if IPVIEW_HAS_CONTRACTS
#  define IPVIEW_PRE(cond)   pre(cond)
#  define IPVIEW_POST(cond)  post(cond)
#  define IPVIEW_ASSERT(cond) assert(cond)
#else
#  define IPVIEW_PRE(cond) \
       do { if (!::IPView::Contracts::check(cond)) \
            ::IPView::Contracts::contractViolation("pre",   #cond); } while (0)
#  define IPVIEW_POST(cond) \
       do { if (!::IPView::Contracts::check(cond)) \
            ::IPView::Contracts::contractViolation("post",  #cond); } while (0)
#  define IPVIEW_ASSERT(cond) \
       do { if (!::IPView::Contracts::check(cond)) \
            ::IPView::Contracts::contractViolation("assert", #cond); } while (0)
#endif

#endif // IPVIEW_CONTRACTS_HPP
