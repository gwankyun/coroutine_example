module;
#include "use_module.h"

#if !USE_STD_MODULE
#  include <chrono>
#  include <concepts>   // std::invocable
#  include <coroutine>  // std::coroutine_handle std::suspend_never
#  include <cstdlib>    // std::exit
#  include <functional> // std::function
#  include <queue>
#  include <string>
#  include <thread>
#  include <unordered_map>
#  include <vector>
#endif

#include "catch2.h"

#include "spdlog.h"

#include "asio_common.hpp"

#include "asio_lite.h"
#include "time_count.h"

// namespace asio = lite::asio;

#include "task.hpp"

export module asio_std_coroutine;

#if USE_STD_MODULE
import std;
#endif

#if USE_THIRD_MODULE
import catch2.compat;
import spdlog;
#endif

using namespace std::literals::chrono_literals;

namespace type
{
    using id = int;
    using output = std::unordered_map<id, std::string>;
} // namespace type

namespace t = type;

namespace f = func;

type::awaiter async_post(type::coroutine& _coroutine, asio::io_context& _io_context, std::function<void()> _f = [] {})
{
    auto cb = [&, _f]
    {
        _f();
        _coroutine.resume();
    };
    return func::async_resume(_coroutine, [&, cb] { asio::post(_io_context, cb); });
}

t::task handle(asio::io_context& _io_context, t::id _id, t::output& _output)
{
    t::coroutine coroutine; // 用於保存協程句柄

    std::string buffer = "abc";
    for (auto& v : buffer)
    {
        co_await async_post(
            coroutine, _io_context,
            [&]
            {
                _output[_id] += v;
                SPDLOG_INFO("id: {} value: {}", _id, v);
            }
        );
    }
}

t::task accept_handle(asio::io_context& _io_context, int _count, t::output& _output)
{
    t::coroutine coroutine; // 用於保存協程句柄

    for (auto i = 0; i < _count; i++)
    {
        co_await async_post(coroutine, _io_context, [&] { handle(_io_context, i, _output); });
    }
}

TEST_CASE("asio_context_fiber", "[context_fiber]")
{
    auto count = 3u;
    t::output output(count);

    asio::io_context io_context;

    accept_handle(io_context, count, output);
    io_context.run();

    for (auto& i : output)
    {
        REQUIRE(i.second == "abc");
    }
}

export int main(int _argc, char* _argv[])
{
    std::string log_format{"[%C-%m-%d %T.%e] [%^%L%$] [%-20!!:%4#] %v"};
    spdlog::set_pattern(log_format);

    auto result = Catch::Session().run(_argc, _argv);
    return result;
}
