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
};

void on_switch(SwitchData& _data, boost::asio::io_context& context)
{
    auto time = 100ms;

    auto cb = [&](std::shared_ptr<steady_timer> _timer, std::string _message)
    {
        return [&, _timer, _message](error_code _ec)
        {
            if (_ec)
            {
                SPDLOG_ERROR("{}", _ec.message());
                return;
            }
            _data.result += _message;
            on_switch(_data, context);
        };
    };

    CORO_BEGIN(_data.state);

    [&]
    {
        auto accept = std::make_shared<steady_timer>(context, time);
        accept->async_wait(cb(accept, "1"));
    }();
    CORO_YIELD(_data.state);

    for (_data.i = 0; _data.i < 3; ++_data.i)
    {
        SPDLOG_INFO("{}", _data.i);
        [&]
        {
            auto read = std::make_shared<steady_timer>(context, time);
            read->async_wait(cb(read, "2"));
        }();
        CORO_YIELD(_data.state);
    }

    [&]
    {
        auto write = std::make_shared<steady_timer>(context, time);
        write->async_wait(cb(write, "3"));
    }();
    CORO_YIELD(_data.state);

    CORO_END();
}

std::string test_switch()
{
    boost::asio::io_context context;

    std::string result = "0";
    int state = 0;

    SwitchData data{state, result, 0};

    on_switch(data, context);

    context.run();
    return data.result;
}
