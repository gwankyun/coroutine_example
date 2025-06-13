#pragma once
#include "macro.h"
#include <catch2/internal/catch_compiler_capabilities.hpp>
#include <catch2/internal/catch_unique_name.hpp>

#define CATCH_INTERNAL_LINEINFO ::Catch::SourceLineInfo(__FILE__, static_cast<std::size_t>(__LINE__))

#define INTERNAL_CATCH_TESTCASE2(TestName, ...) \
    static void TestName(); \
    CATCH_INTERNAL_START_WARNINGS_SUPPRESSION \
    CATCH_INTERNAL_SUPPRESS_GLOBALS_WARNINGS \
    CATCH_INTERNAL_SUPPRESS_UNUSED_VARIABLE_WARNINGS \
    namespace \
    { \
        const Catch::AutoReg INTERNAL_CATCH_UNIQUE_NAME(autoRegistrar)( \
            Catch::makeTestInvoker(&TestName), CATCH_INTERNAL_LINEINFO, Catch::StringRef(), \
            Catch::NameAndTags{__VA_ARGS__} \
        ); \
    } /* NOLINT */ \
    CATCH_INTERNAL_STOP_WARNINGS_SUPPRESSION \
    static void TestName()
#define INTERNAL_CATCH_TESTCASE(...) \
    INTERNAL_CATCH_TESTCASE2(INTERNAL_CATCH_UNIQUE_NAME(CATCH2_INTERNAL_TEST_), __VA_ARGS__)

#define TEST_CASE(...) INTERNAL_CATCH_TESTCASE(__VA_ARGS__)

#define INTERNAL_CATCH_SECTION(...) \
    CATCH_INTERNAL_START_WARNINGS_SUPPRESSION \
    CATCH_INTERNAL_SUPPRESS_UNUSED_VARIABLE_WARNINGS \
    if (Catch::Section const& INTERNAL_CATCH_UNIQUE_NAME(catch_internal_Section) = \
            Catch::Section(CATCH_INTERNAL_LINEINFO, __VA_ARGS__)) \
    CATCH_INTERNAL_STOP_WARNINGS_SUPPRESSION

#define SECTION(...) INTERNAL_CATCH_SECTION(__VA_ARGS__)

#if defined(CATCH_CONFIG_PREFIX_ALL)
#  define CATCH_REQUIRE(_expression) Catch::require(CATCH_EXPRESSION(_expression))
#  define CATCH_CHECK(_expression) Catch::check(CATCH_EXPRESSION(_expression))
#else
#  define REQUIRE(_expression) Catch::require(CATCH_EXPRESSION(_expression))
#  define CHECK(_expression) Catch::check(CATCH_EXPRESSION(_expression))
#endif
