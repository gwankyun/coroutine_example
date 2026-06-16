#include <spdlog/spdlog.h>

#include <catch2/../catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>

import std;
import test.callback;
import test.switch_coro;
import test.coroutine2;
import test.context.continuation;
import test.context.fiber;
import test.fiber;
import test.std_coroutine;
import test.use_awaitable;
import test.cobalt;
import test.stdexec;

using namespace std::chrono_literals;

TEST_CASE("callback", "[async]")
{
    REQUIRE(test_callback() == "012223");
}

TEST_CASE("switch", "[async]")
{
    REQUIRE(test_switch_coro() == "012223");
}

TEST_CASE("coroutine2", "[async]")
{
    REQUIRE(test_coroutine2() == "012223");
}

TEST_CASE("context.continuation", "[async]")
{
    REQUIRE(test_context_continuation() == "012223");
}

TEST_CASE("context.fiber", "[async]")
{
    REQUIRE(test_context_fiber() == "012223");
}

TEST_CASE("fiber", "[async]")
{
    auto result = test_fiber();
    REQUIRE(result.size() == 3);
    for (auto i = 0; i != 3; ++i)
    {
         REQUIRE(result[i] == std::to_string(i) + "rrrw");
    }
}

TEST_CASE("std.coroutine", "[async]")
{
    auto result = test_std_coroutine();
    REQUIRE(result.size() == 3);
    for (auto i = 0; i != 3; ++i)
    {
        REQUIRE(result[i] == std::to_string(i) + "rrrw");
    }
}

TEST_CASE("use_awaitable", "[async]")
{
    auto result = test_use_awaitable();
    REQUIRE(result.size() == 3);
    for (auto i = 0; i != 3; ++i)
    {
        REQUIRE(result[i] == std::to_string(i) + "rrrw");
    }
}

TEST_CASE("cobalt", "[async]")
{
    auto result = test_cobalt();
    REQUIRE(result.size() == 3);
    for (auto i = 0; i != 3; ++i)
    {
        REQUIRE(result[i] == std::to_string(i) + "rrrw");
    }
}

TEST_CASE("stdexec", "[async]")
{
    auto result = test_stdexec();
    REQUIRE(result.size() == 3);
    for (auto i = 0; i != 3; ++i)
    {
        REQUIRE(result[i] == std::to_string(i) + "rrrw");
    }
}

int main(int _argc, char* _argv[])
{
    std::string log_format{"[%C-%m-%d %T.%e] [%^%L%$] [%-20!!:%4#] %v"};
    spdlog::set_pattern(log_format);

    auto result = Catch::Session().run(_argc, _argv);
    return result;
}
