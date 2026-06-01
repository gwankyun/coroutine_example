module;

#include <spdlog/spdlog.h>
#include <boost/asio.hpp>

module test.callback;

import std;

using namespace std::chrono_literals;

std::string test_callback()
{
    boost::asio::io_context context;

    using boost::asio::steady_timer;
    using boost::system::error_code;

    std::string result = "0";

    auto time = 100ms;

    SPDLOG_INFO("");

    {
        auto accept = std::make_shared<steady_timer>(context, time);

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
                auto read = std::make_shared<steady_timer>(context, time);
                read->async_wait(
                    [&, read](error_code _ec)
                    {
                        if (_ec)
                        {
                            SPDLOG_ERROR("{}", _ec.message());
                            return;
                        }
                        SPDLOG_INFO("read");
                        result += "2";
                        auto write = std::make_shared<steady_timer>(context, time);
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
                    });
            });
    }

    SPDLOG_INFO("");

    context.run();
    return result;
}
