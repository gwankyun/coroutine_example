#include <boost/asio.hpp>
#include <boost/system.hpp> // boost::system::error_code
#include <spdlog/spdlog.h>
#include <string>
#define BOOST_ALL_NO_LIB 1
#include <boost/context/continuation.hpp>

namespace context = boost::context;
using continuation_t = context::continuation;

namespace asio = boost::asio;
using error_code_t = boost::system::error_code;

void handle(asio::io_context& _io_context)
{
    std::unique_ptr<continuation_t> main;
    std::unique_ptr<continuation_t> sub;

    bool flag = false;

    sub = std::make_unique<continuation_t>(context::callcc(
        [&_io_context, &flag, &main](continuation_t&& _sink)
        {
            _sink = _sink.resume();
            error_code_t error;
            auto cb = [&flag, &error](error_code_t _e = error_code_t{})
            {
                error = _e;
                flag = true;
            };

            asio::post(_io_context, cb);
            flag = false;
            *main = main->resume();

            SPDLOG_INFO("read");
            asio::post(_io_context, cb);
            flag = false;
            *main = main->resume();

            SPDLOG_INFO("write");
            asio::steady_timer timer(_io_context, asio::chrono::seconds(1));
            timer.async_wait(cb);
            flag = false;
            *main = main->resume();

            if (error)
            {
                SPDLOG_INFO("{}", error.message());
                return std::move(_sink);
            }
            SPDLOG_INFO("timeout");
            return std::move(_sink);
        }));

    main = std::make_unique<continuation_t>(context::callcc(
        [&_io_context, &flag](continuation_t&& _sink)
        {
            _sink = _sink.resume();
            auto& sub = _sink;
            while (sub)
            {
                _io_context.run();
                if (flag)
                {
                    sub = sub.resume();
                }
            }
            return std::move(_sink);
        }));

    *sub = sub->resume();
}

int main()
{
    std::string log_format{"[%C-%m-%d %T.%e] [%^%L%$] [%-20!!:%4#] %v"};
    spdlog::set_pattern(log_format);

    asio::io_context io_context;

    handle(io_context);

    return 0;
}
