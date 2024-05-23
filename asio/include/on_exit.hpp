#pragma once
#include <functional>

// clang-format off
#ifndef ON_EXIT_HEADR_ONLY
#  define ON_EXIT_HEADR_ONLY 1
#endif
// clang-format on

namespace common
{
    struct OnExit
    {
        using Fn = std::function<void()>;
        Fn fn;
#if ON_EXIT_HEADR_ONLY
        OnExit(Fn _fn) : fn(_fn)
        {
        }
        ~OnExit()
        {
            fn();
        }
#else
        OnExit(Fn _fn);
        ~OnExit();
#endif
    };
} // namespace common

// clang-format off
#ifndef CAT_IMPL
#  define CAT_IMPL(a, b) a##b
#endif

#ifndef CAT
#  define CAT(a, b) CAT_IMPL(a, b)
#endif

#ifndef ON_EXIT
#  define ON_EXIT(...) common::OnExit CAT(on_exit_, __LINE__)(##__VA_ARGS__)
#endif
// clang-format on
