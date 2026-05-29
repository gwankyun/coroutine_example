#include <spdlog/spdlog.h>

#include <catch2/../catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>

#include <boost/asio.hpp>

#include <boost/coroutine2/all.hpp>

import std;

using namespace std::chrono_literals;

namespace coro2 = boost::coroutines2;

#define CORO_BEGIN(_state) \
    switch (_state) \
    { \
    case 0:

#define CORO_YIELD(_state) \
    do \
    { \
        _state = __LINE__; \
        return; \
    case __LINE__:; \
    } while (false)

#define CORO_END() }

TEST_CASE("async", "[callback]")
{
    boost::asio::io_context context;

    using boost::asio::steady_timer;
    using boost::system::error_code;

    std::string result = "0";

    SPDLOG_INFO("");

    {
        auto accept = std::make_shared<steady_timer>(context, 1s);

        accept->async_wait(
            [accept, &context, &result](error_code _ec)
            {
                if (_ec)
                {
                    SPDLOG_ERROR("{}", _ec.message());
                    return;
                }
                SPDLOG_INFO("accept");
                result += "1";
                auto read = std::make_shared<steady_timer>(context, 1s);
                read->async_wait(
                    [read, &context, &result](error_code _ec)
                    {
                        if (_ec)
                        {
                            SPDLOG_ERROR("{}", _ec.message());
                            return;
                        }
                        SPDLOG_INFO("read");
                        result += "2";
                        auto write = std::make_shared<steady_timer>(context, 1s);
                        write->async_wait(
                            [write, &context, &result](error_code _ec)
                            {
                                if (_ec)
                                {
                                    SPDLOG_ERROR("{}", _ec.message());
                                    return;
                                }
                                SPDLOG_INFO("write");
                                result += "3";
                            });
                    });
            });
    }

    SPDLOG_INFO("");

    context.run();
    REQUIRE(result == "0123");
}

using boost::asio::steady_timer;
using boost::system::error_code;

void on_switch(int& _state, boost::asio::io_context& context, std::string& result)
{
    CORO_BEGIN(_state);
    [&]
    {
        auto accept = std::make_shared<steady_timer>(context, 1s);
        accept->async_wait(
            [accept, &_state, &context, &result](error_code _ec)
            {
                if (_ec)
                {
                    SPDLOG_ERROR("{}", _ec.message());
                    return;
                }
                result += "1";
                on_switch(_state, context, result);
            });
    }();
    CORO_YIELD(_state);
    [&]
    {
        auto read = std::make_shared<steady_timer>(context, 1s);
        read->async_wait(
            [read, &_state, &context, &result](error_code _ec)
            {
                if (_ec)
                {
                    SPDLOG_ERROR("{}", _ec.message());
                    return;
                }
                result += "2";
                on_switch(_state, context, result);
            });
    }();
    CORO_YIELD(_state);
    [&]
    {
        auto write = std::make_shared<steady_timer>(context, 1s);
        write->async_wait(
            [write, &_state, &context, &result](error_code _ec)
            {
                if (_ec)
                {
                    SPDLOG_ERROR("{}", _ec.message());
                    return;
                }
                result += "3";
                on_switch(_state, context, result);
            });
    }();
    CORO_YIELD(_state);
    CORO_END();
}

TEST_CASE("async", "[switch]")
{
    boost::asio::io_context context;

    std::string result = "0";
    int state = 0;

    on_switch(state, context, result);

    context.run();
    REQUIRE(result == "0123");
}

TEST_CASE("async", "[coroutine2]")
{
    boost::asio::io_context context;

    std::string result = "0";

    typename coro2::coroutine<void>::pull_type pull(
        [&](typename coro2::coroutine<void>::push_type& _push)
        {
            auto accept = std::make_shared<steady_timer>(context, 1s);
            accept->async_wait(
                [accept, &context, &result, &_push](error_code _ec)
                {
                    result += "1";
                    _push();
                });
            _push();
            auto read = std::make_shared<steady_timer>(context, 1s);
            read->async_wait(
                [read, &context, &result, &_push](error_code _ec)
                {
                    result += "2";
                    _push();
                });
            auto write = std::make_shared<steady_timer>(context, 1s);
            write->async_wait(
                [write, &context, &result, &_push](error_code _ec)
                {
                    result += "3";
                    _push();
                });
        });

    pull();
    context.run();
    pull();
}

int main(int _argc, char* _argv[])
{
    std::string log_format{"[%C-%m-%d %T.%e] [%^%L%$] [%-10!!:%4#] %v"};
    spdlog::set_pattern(log_format);

    auto result = Catch::Session().run(_argc, _argv);
    return result;
}
