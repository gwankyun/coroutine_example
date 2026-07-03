#include <catch2/../catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>
#include <spdlog/spdlog.h>

import test.coroutine2;

TEST_CASE("coroutine2", "[async]")
{
    auto result = test_coroutine2();
    REQUIRE(result.size() == 3);
    for (auto i = 1; i != 4; ++i)
    {
        REQUIRE(result[i] == std::to_string(i) + "rrrw");
    }
}

int main(int argc, char* argv[])
{
    spdlog::set_pattern("[%C-%m-%d %T.%e] [%^%L%$] [%-20!!:%4#] %v");
    return Catch::Session().run(argc, argv);
}
