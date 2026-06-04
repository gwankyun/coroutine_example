#include <spdlog/spdlog.h>

#include <catch2/../catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>

import std;
import test.callback;
import test.switch_coro;
import test.coroutine2;
import test.continuation;

using namespace std::chrono_literals;

TEST_CASE("async", "[callback]")
{
    REQUIRE(test_callback() == "0123");
}

//using boost::asio::steady_timer;
//using boost::system::error_code;

TEST_CASE("async", "[switch]")
{
    REQUIRE(test_switch() == "012223");
}

TEST_CASE("async", "[coroutine2]")
{
    REQUIRE(test_coroutine2() == "012223");
}

TEST_CASE("async", "[continuation]")
{
    REQUIRE(test_continuation() == "012223");
}

int main(int _argc, char* _argv[])
{
    std::string log_format{"[%C-%m-%d %T.%e] [%^%L%$] [%-10!!:%4#] %v"};
    spdlog::set_pattern(log_format);

    auto result = Catch::Session().run(_argc, _argv);
    return result;
}
