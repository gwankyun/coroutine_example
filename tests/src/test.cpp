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
    REQUIRE(test_fiber() == "012223");
}

int main(int _argc, char* _argv[])
{
    std::string log_format{"[%C-%m-%d %T.%e] [%^%L%$] [%-20!!:%4#] %v"};
    spdlog::set_pattern(log_format);

    auto result = Catch::Session().run(_argc, _argv);
    return result;
}
