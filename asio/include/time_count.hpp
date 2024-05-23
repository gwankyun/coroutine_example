#pragma once

#include <chrono>
#include <functional>

#include <cstdint>

// clang-format off
#ifndef TIME_COUNT_HEADR_ONLY
#  define TIME_COUNT_HEADR_ONLY 1
#endif
// clang-format on

namespace common
{
    namespace detail
    {
        inline std::uint64_t time_count(std::function<void()> _fn)
        {
            namespace c = std::chrono;
            auto start = c::steady_clock::now();
            _fn();
            auto end = c::steady_clock::now();
            auto duration = c::duration_cast<c::milliseconds>(end - start);
            return duration.count();
        }
    }

#if TIME_COUNT_HEADR_ONLY
    inline std::uint64_t time_count(std::function<void()> _fn)
    {
        return detail::time_count(_fn);
    }
#else
    std::uint64_t time_count(std::function<void()> _fn);
#endif
} // namespace common
