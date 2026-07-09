#include <spdlog.hpp>
#include <catch2/../catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>

import test.stdexec;
import spdlog;

TEST_CASE("stdexec", "[async]")
{
    auto result = test_stdexec();
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
