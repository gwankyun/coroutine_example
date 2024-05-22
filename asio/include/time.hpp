#pragma once

#include <chrono>
#include <functional>

#include <cstdint>

namespace common
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
} // namespace common
