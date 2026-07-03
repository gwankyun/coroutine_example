#include <spdlog/spdlog.h>
#include <catch2/../catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>

import test.use_awaitable;

TEST_CASE("use_awaitable", "[async]")
{
    auto result = test_use_awaitable();
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
