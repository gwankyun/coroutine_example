#pragma once

#define SPDLOG_LEVEL_TRACE 0
#define SPDLOG_LEVEL_DEBUG 1
#define SPDLOG_LEVEL_INFO 2
#define SPDLOG_LEVEL_WARN 3
#define SPDLOG_LEVEL_ERROR 4
#define SPDLOG_LEVEL_CRITICAL 5
#define SPDLOG_LEVEL_OFF 6

#if !defined(SPDLOG_ACTIVE_LEVEL)
#  define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_TRACE
#  define SPDLOG_LOGGER_TRACE(logger, ...) spdlog::get(logger).trace(__VA_ARGS__)
#  define SPDLOG_TRACE(...) spdlog::get().trace(__VA_ARGS__)
#else
#  define SPDLOG_LOGGER_TRACE(logger, ...) (void)0
#  define SPDLOG_TRACE(...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_DEBUG
#  define SPDLOG_LOGGER_DEBUG(logger, ...) spdlog::get(logger).debug(__VA_ARGS__)
#  define SPDLOG_DEBUG(...) spdlog::get().debug(__VA_ARGS__)
#else
#  define SPDLOG_LOGGER_DEBUG(logger, ...) (void)0
#  define SPDLOG_DEBUG(...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_INFO
#  define SPDLOG_LOGGER_INFO(logger, ...) spdlog::get(logger).info(__VA_ARGS__)
#  define SPDLOG_INFO(...) spdlog::get().info(__VA_ARGS__)
#else
#  define SPDLOG_LOGGER_INFO(logger, ...) (void)0
#  define SPDLOG_INFO(...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_WARN
#  define SPDLOG_LOGGER_WARN(logger, ...) spdlog::get(logger).warn(__VA_ARGS__)
#  define SPDLOG_WARN(...) spdlog::get().warn(__VA_ARGS__)
#else
#  define SPDLOG_LOGGER_WARN(logger, ...) (void)0
#  define SPDLOG_WARN(...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_ERROR
#  define SPDLOG_LOGGER_ERROR(logger, ...) spdlog::get(logger).error(__VA_ARGS__)
#  define SPDLOG_ERROR(...) spdlog::get().error(__VA_ARGS__)
#else
#  define SPDLOG_LOGGER_ERROR(logger, ...) (void)0
#  define SPDLOG_ERROR(...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_CRITICAL
#  define SPDLOG_LOGGER_CRITICAL(logger, ...) spdlog::get(logger).critical(__VA_ARGS__)
#  define SPDLOG_CRITICAL(...) spdlog::get().critical(__VA_ARGS__)
#else
#  define SPDLOG_LOGGER_CRITICAL(logger, ...) (void)0
#  define SPDLOG_CRITICAL(...) (void)0
#endif
