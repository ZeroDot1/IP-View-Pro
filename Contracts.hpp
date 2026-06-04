// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.15.4 — Contracts.hpp
//
//  C++26 contracts (P2900) compatibility shim.
//
//  GCC 16.1.1 does not yet implement P2900 (C++26 contracts). The
//  pre/post/assert keywords would generate syntax errors. This
//  header detects whether the compiler supports them and provides
//  IPVIEW_PRE / IPVIEW_POST / IPVIEW_ASSERT macros that:
//    * resolve to real contracts when the feature is available, and
//    * fall back to a conditional throw of std::logic_error otherwise.
//
//  The fallback form is a single expression (no do-while) so it can
//  be embedded inside other expression contexts, e.g. CHECK macros
//  or the condition of an if-statement, without surprising the
//  preprocessor.
// ═══════════════════════════════════════════════════════════════════════════════

#ifndef IPVIEW_CONTRACTS_HPP
#define IPVIEW_CONTRACTS_HPP

#include <stdexcept>
#include <source_location>
#include <string>
#include <utility>

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
    throw std::logic_error(message);
}

} // namespace IPView::Contracts

#if IPVIEW_HAS_CONTRACTS
#  define IPVIEW_PRE(cond)    pre(cond)
#  define IPVIEW_POST(cond)   post(cond)
#  define IPVIEW_ASSERT(cond) assert(cond)
#else
// Expression form: (cond) ? void(0) : throw-via-helper.
// Both branches are valid in expression context.
#  define IPVIEW_PRE(cond) \
       ((cond) ? void(0) \
               : ::IPView::Contracts::contractViolation("pre", #cond, std::source_location::current()))
#  define IPVIEW_POST(cond) \
       ((cond) ? void(0) \
               : ::IPView::Contracts::contractViolation("post", #cond, std::source_location::current()))
#  define IPVIEW_ASSERT(cond) \
       ((cond) ? void(0) \
               : ::IPView::Contracts::contractViolation("assert", #cond, std::source_location::current()))
#endif

#endif // IPVIEW_CONTRACTS_HPP
