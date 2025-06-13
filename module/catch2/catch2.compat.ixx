module;
#include <catch2/catch_test_macros.hpp>

export module catch2.compat;
export import catch2.base;

export namespace Catch
{
    using Catch::AutoReg;
    using Catch::makeTestInvoker;
    using Catch::NameAndTags;
    using Catch::Section;
    using Catch::StringRef;
} // namespace Catch
