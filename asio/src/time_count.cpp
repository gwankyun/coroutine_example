#include "../include/time_count.h"

namespace common
{
    std::uint64_t time_count(std::function<void()> _fn)
    {
        return detail::time_count(_fn);
    }
} // namespace common
