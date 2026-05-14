// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.7.0 — saturate_fix.h
//  C++26 polyfill for GCC 16.1.1: q26::saturate_cast
//
//  Problem:  Qt 6.11 checks __cpp_lib_saturate_cast (P0543, C++26) and
//            attempts using std::saturate_cast in namespace q26.
//            GCC 16.1.1 defines the macro, but libstdc++ does not provide
//            the function yet in the <numeric> header.
//
//  Solution:   • __cpp_lib_saturate_cast is reset (so Qt
//              does not attempt to import the missing std::saturate_cast)
//            • q26::saturate_cast is provided as a C++26-compliant polyfill
//
//  Force-included via CMake target_compile_options(-include ...)
// ═══════════════════════════════════════════════════════════════════════════════

#ifndef SATURATE_FIX_H
#define SATURATE_FIX_H

// GCC 16.1.1 has defined __cpp_lib_saturate_cast, but std::saturate_cast
// is missing from the <numeric> header. We disable the macro so that Qt's
// q26numeric.h does not attempt to use std::saturate_cast.
#if defined(__cpp_lib_saturate_cast) && __cpp_lib_saturate_cast >= 201907L
#  undef __cpp_lib_saturate_cast
#endif

#include <limits>
#include <type_traits>

// ── C++26-compliant saturate_cast (P0543R5) ────────────────────────────────
//  Qt 6.11 expects this in namespace q26 (defined in QtCore/q26numeric.h)
namespace q26 {

template<typename R, typename T>
    requires (std::is_arithmetic_v<R> && std::is_arithmetic_v<T>)
constexpr R saturate_cast(T x) noexcept
{
    if constexpr (std::is_floating_point_v<R> && std::is_floating_point_v<T>) {
        // float → float
        if (x > static_cast<T>(std::numeric_limits<R>::max()))
            return std::numeric_limits<R>::max();
        if (x < static_cast<T>(std::numeric_limits<R>::lowest()))
            return std::numeric_limits<R>::lowest();
        return static_cast<R>(x);
    }
    else if constexpr (std::is_integral_v<R> && std::is_integral_v<T>) {
        if constexpr (std::is_signed_v<R> == std::is_signed_v<T>) {
            if (x > static_cast<T>(std::numeric_limits<R>::max()))
                return std::numeric_limits<R>::max();
            if (x < static_cast<T>(std::numeric_limits<R>::lowest()))
                return std::numeric_limits<R>::lowest();
            return static_cast<R>(x);
        }
        else if constexpr (std::is_signed_v<R> && std::is_unsigned_v<T>) {
            // signed dst, unsigned src
            if (x > static_cast<std::make_unsigned_t<R>>(std::numeric_limits<R>::max()))
                return std::numeric_limits<R>::max();
            return static_cast<R>(x);
        }
        else {
            // unsigned dst, signed src
            if (x < 0)
                return 0;
            if (static_cast<std::make_unsigned_t<T>>(x) > std::numeric_limits<R>::max())
                return std::numeric_limits<R>::max();
            return static_cast<R>(x);
        }
    }
    else {
        // float ↔ integer
        if (x > static_cast<T>(std::numeric_limits<R>::max()))
            return std::numeric_limits<R>::max();
        if (x < static_cast<T>(std::numeric_limits<R>::lowest()))
            return std::numeric_limits<R>::lowest();
        return static_cast<R>(x);
    }
}

} // namespace q26

#endif // SATURATE_FIX_H
