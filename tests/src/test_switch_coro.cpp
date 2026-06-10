module;

#include <boost/asio.hpp>
#include <spdlog/spdlog.h>

module test.switch_coro;

import std;

using namespace std::chrono_literals;
using boost::asio::steady_timer;
using boost::system::error_code;

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

struct SwitchData
{
    int state;
    std::string result;
    int i;
    error_code ec;
};

void on_switch(SwitchData& _data, boost::asio::io_context& context)
{
    auto time = 100ms;
    auto& ec = _data.ec;
    auto& state = _data.state;
    auto& result = _data.result;
    auto& i = _data.i;

    auto cb = [&](std::shared_ptr<steady_timer> _timer, error_code& ec) {
        return [&, _timer](error_code _ec) {
            ec = _ec;
            on_switch(_data, context);
        };
    };

    CORO_BEGIN(state);

    [&] {
        auto accept = std::make_shared<steady_timer>(context, time);
        accept->async_wait(cb(accept, ec));
    }();
    CORO_YIELD(state);
    if (ec)
    {
        SPDLOG_ERROR("{}", ec.message());
        return;
    }
    result += "1";

    for (i = 0; i < 3; ++i)
    {
        SPDLOG_INFO("{}", i);
        [&] {
            auto read = std::make_shared<steady_timer>(context, time);
            read->async_wait(cb(read, ec));
        }();
        CORO_YIELD(_data.state);
        if (ec)
        {
            SPDLOG_ERROR("{}", ec.message());
            return;
        }
        result += "2";
    }

    [&] {
        auto write = std::make_shared<steady_timer>(context, time);
        write->async_wait(cb(write, ec));
    }();
    CORO_YIELD(state);
    if (ec)
    {
        SPDLOG_ERROR("{}", ec.message());
        return;
    }
    result += "3";

    CORO_END();
}

std::string test_switch_coro()
{
    boost::asio::io_context context;

    std::string result = "0";
    int state = 0;

    SwitchData data{state, result, 0, error_code()};

    on_switch(data, context);

    context.run();
    return data.result;
}
