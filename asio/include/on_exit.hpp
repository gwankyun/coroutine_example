#pragma once
#include <functional>

struct OnExit
{
    using fn_t = std::function<void()>;
    OnExit(fn_t _fn) : fn(_fn)
    {
    }
    ~OnExit()
    {
        fn();
    }
    fn_t fn;
};

// clang-format off
#ifndef CAT_IMPL
#  define CAT_IMPL(a, b) a##b
#endif

#ifndef CAT
#  define CAT(a, b) CAT_IMPL(a, b)
#endif

#ifndef ON_EXIT
#  define ON_EXIT(...) OnExit CAT(unique_, __LINE__)(##__VA_ARGS__)
#endif
// clang-format on
