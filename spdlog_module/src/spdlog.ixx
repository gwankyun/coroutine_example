module;

#define SPDLOG_HEADER_UNIT 1

#if !SPDLOG_HEADER_UNIT
#  include <spdlog/fmt/bin_to_hex.h>
#  include <spdlog/spdlog.h>
#endif

export module spdlog;

#if !SPDLOG_HEADER_UNIT
export namespace spdlog
{
    namespace level
    {
        using spdlog::level::level_enum;
    }
    using spdlog::critical;
    using spdlog::debug;
    using spdlog::default_logger;
    using spdlog::default_logger_raw;
    using spdlog::error;
    using spdlog::info;
    using spdlog::logger;
    using spdlog::set_level;
    using spdlog::set_pattern;
    using spdlog::source_loc;
    using spdlog::trace;
    using spdlog::warn;

    using spdlog::to_hex;
} // namespace spdlog
#else
export import <spdlog/spdlog.h>;
#endif
