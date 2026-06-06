module;

#include <boost/asio.hpp>
#include <spdlog/spdlog.h>

module test.callback;

import std;

using namespace std::chrono_literals;

using boost::asio::steady_timer;
using boost::system::error_code;

auto g_time = 100ms;

void on_read(error_code _ec, boost::asio::io_context& context, std::shared_ptr<steady_timer> _timer,
             std::string& result, int i)
{
    if (_ec)
    {
        SPDLOG_ERROR("{}", _ec.message());
        return;
    }
    SPDLOG_INFO("read: {}", i);
    if (i < 3)
    {
        result += "2";
        _timer->async_wait([&, _timer, i](error_code _ec) { on_read(_ec, context, _timer, result, i + 1); });
    }
    else
    {
        auto write = std::make_shared<steady_timer>(context, g_time);
        write->async_wait(
            [&, write](error_code _ec)
            {
                if (_ec)
                {
                    SPDLOG_ERROR("{}", _ec.message());
                    return;
                }
                SPDLOG_INFO("write");
                result += "3";
            });
    }
}

std::string test_callback()
{
    boost::asio::io_context context;

    std::string result = "0";

    SPDLOG_INFO("");

    {
        auto accept = std::make_shared<steady_timer>(context, g_time);

        accept->async_wait(
            [&, accept](error_code _ec)
            {
                if (_ec)
                {
                    SPDLOG_ERROR("{}", _ec.message());
                    return;
                }
                SPDLOG_INFO("accept");
                result += "1";
                auto read = std::make_shared<steady_timer>(context, g_time);
                read->async_wait([&, read](error_code _ec) { on_read(_ec, context, read, result, 0); });
            });
    }

    SPDLOG_INFO("");

    context.run();
    return result;
}
