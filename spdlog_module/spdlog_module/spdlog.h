#pragma once

#ifndef SPDLOG_FUNCTION
#  define SPDLOG_FUNCTION static_cast<const char*>(__FUNCTION__)
#endif

#ifndef SPDLOG_NO_SOURCE_LOC
#  define SPDLOG_LOGGER_CALL(logger, level, ...) \
      (logger)->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, \
                    level, __VA_ARGS__)
#else
#  define SPDLOG_LOGGER_CALL(logger, level, ...) \
      (logger)->log(spdlog::source_loc{}, level, __VA_ARGS__)
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_TRACE
#  define SPDLOG_LOGGER_TRACE(logger, ...) \
      SPDLOG_LOGGER_CALL(logger, spdlog::level::trace, __VA_ARGS__)
#  define SPDLOG_TRACE(...) \
      SPDLOG_LOGGER_TRACE(spdlog::default_logger_raw(), __VA_ARGS__)
#else
#  define SPDLOG_LOGGER_TRACE(logger, ...) (void)0
#  define SPDLOG_TRACE(...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_DEBUG
#  define SPDLOG_LOGGER_DEBUG(logger, ...) \
      SPDLOG_LOGGER_CALL(logger, spdlog::level::debug, __VA_ARGS__)
#  define SPDLOG_DEBUG(...) \
      SPDLOG_LOGGER_DEBUG(spdlog::default_logger_raw(), __VA_ARGS__)
#else
#  define SPDLOG_LOGGER_DEBUG(logger, ...) (void)0
#  define SPDLOG_DEBUG(...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_INFO
#  define SPDLOG_LOGGER_INFO(logger, ...) \
      SPDLOG_LOGGER_CALL(logger, spdlog::level::info, __VA_ARGS__)
#  define SPDLOG_INFO(...) \
      SPDLOG_LOGGER_INFO(spdlog::default_logger_raw(), __VA_ARGS__)
#else
#  define SPDLOG_LOGGER_INFO(logger, ...) (void)0
#  define SPDLOG_INFO(...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_WARN
#  define SPDLOG_LOGGER_WARN(logger, ...) \
      SPDLOG_LOGGER_CALL(logger, spdlog::level::warn, __VA_ARGS__)
#  define SPDLOG_WARN(...) \
      SPDLOG_LOGGER_WARN(spdlog::default_logger_raw(), __VA_ARGS__)
#else
#  define SPDLOG_LOGGER_WARN(logger, ...) (void)0
#  define SPDLOG_WARN(...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_ERROR
#  define SPDLOG_LOGGER_ERROR(logger, ...) \
      SPDLOG_LOGGER_CALL(logger, spdlog::level::err, __VA_ARGS__)
#  define SPDLOG_ERROR(...) \
      SPDLOG_LOGGER_ERROR(spdlog::default_logger_raw(), __VA_ARGS__)
#else
#  define SPDLOG_LOGGER_ERROR(logger, ...) (void)0
#  define SPDLOG_ERROR(...) (void)0
#endif

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_CRITICAL
#  define SPDLOG_LOGGER_CRITICAL(logger, ...) \
      SPDLOG_LOGGER_CALL(logger, spdlog::level::critical, __VA_ARGS__)
#  define SPDLOG_CRITICAL(...) \
      SPDLOG_LOGGER_CRITICAL(spdlog::default_logger_raw(), __VA_ARGS__)
#else
#  define SPDLOG_LOGGER_CRITICAL(logger, ...) (void)0
#  define SPDLOG_CRITICAL(...) (void)0
#endif
