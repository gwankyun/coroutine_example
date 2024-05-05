#include <boost/asio.hpp>
#include <boost/system.hpp> // boost::system::error_code
#include <spdlog/spdlog.h>
#include <string>
#define BOOST_ALL_NO_LIB 1
#include <boost/coroutine2/all.hpp>
#include <queue>
#include <unordered_map>

namespace asio = boost::asio;
using error_code_t = boost::system::error_code;

namespace coro2 = boost::coroutines2;

template <typename T = void>
using pull_type = typename coro2::coroutine<T>::pull_type;

template <typename T = void>
using push_type = typename coro2::coroutine<T>::push_type;

void handle(asio::io_context& _io_context)
{
    using pull_t = pull_type<>;
    using push_t = push_type<>;

    bool flag = false;

    pull_t resume(
        [&flag, &_io_context](push_t& _yield)
        {
            error_code_t error;
            auto cb = [&flag, &error](error_code_t _e = error_code_t{})
            {
                error = _e;
                flag = true;
            };

            asio::post(_io_context, cb);
            flag = false;
            _yield();

            SPDLOG_INFO("read");
            asio::post(_io_context, cb);
            flag = false;
            _yield();

            SPDLOG_INFO("write");
            asio::steady_timer timer(_io_context, asio::chrono::seconds(1));
            timer.async_wait(cb);
            flag = false;
            _yield();

            if (error)
            {
                SPDLOG_INFO("{}", error.message());
                return;
            }
            SPDLOG_INFO("timeout");
        });

    while (resume)
    {
        _io_context.run();
        if (flag)
        {
            resume();
        }
    }
}

int main()
{
    std::string log_format{"[%C-%m-%d %T.%e] [%^%L%$] [%-20!!:%4#] %v"};
    spdlog::set_pattern(log_format);

    asio::io_context io_context;

    handle(io_context);

    return 0;
}
