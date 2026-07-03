#include <spdlog/spdlog.h>
#include <catch2/../catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>

import test.std_coroutine;

TEST_CASE("std.coroutine", "[async]")
{
    auto result = test_std_coroutine();
    REQUIRE(result.size() == 3);
    for (auto i = 0; i != 3; ++i)
    {
        REQUIRE(result[i] == std::to_string(i) + "rrrw");
    }
}

int main(int argc, char* argv[])
{
    spdlog::set_pattern("[%C-%m-%d %T.%e] [%^%L%$] [%-20!!:%4#] %v");
    return Catch::Session().run(argc, argv);
}
