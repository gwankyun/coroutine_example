#include <concepts>  // std::invocable
#include <coroutine> // std::coroutine_handle std::suspend_never
#include <functional>
#include <queue>
#include <string>
#include <vector>

#include <cstdlib>

#include "asio_lite.h"
#include "time_count.h"
#include <boost/asio.hpp>
#include <spdlog/spdlog.h>

namespace asio = boost::asio;
// namespace asio = lite::asio;

namespace type
{
    using coroutine = std::coroutine_handle<>;
    using id = int;

    struct task
    {
        struct promise_type
        {
            task get_return_object()
            {
                return {};
            }
            std::suspend_never initial_suspend()
            {
                return {};
            }
            std::suspend_never final_suspend() noexcept
            {
                return {};
            }
            void unhandled_exception()
            {
                std::exit(1);
            }
            void return_void()
            {
            }
        };
    };

    struct awaiter
    {
        using Fn = std::function<void(coroutine&)>;
        explicit awaiter(Fn _f) : f(_f)
        {
        }
        bool await_ready()
        {
            return false;
        }
        void await_suspend(coroutine _coroutine) // _coroutine為傳入的協程
        {
            f(_coroutine); // 暫停時調用
        }
        void await_resume()
        {
        }
        Fn f;
    };
} // namespace type

namespace t = type;

namespace func
{
    /// @brief 在協程中處理回調
    /// @param _coroutine 協程
    /// @param _f 回調，在裡面執行_coroutine.resume()即恢復
    /// @return 可供co_await的協程
    t::awaiter async_resume(t::coroutine& _coroutine, std::function<void()> _f)
    {
        auto fn = [&_coroutine, _f](const t::coroutine& _coro)
        {
            _coroutine = _coro; // 把協程句柄傳出去
            _f();               // 業務回調
        };
        return t::awaiter{fn};
    }
} // namespace func

namespace f = func;

t::task handle(asio::io_context& _io_context, t::id _id)
{
    t::coroutine coroutine; // 用於保存協程句柄

    auto resume = [&] { coroutine.resume(); };

    std::vector<std::string> vec{
        "a",
        "b",
        "c",
    };
    for (auto& i : vec)
    {
        co_await f::async_resume(coroutine, [&] { asio::post(_io_context, resume); });

        SPDLOG_INFO("id: {} value: {}", _id, i);
    }
}

t::task accept_handle(asio::io_context& _io_context)
{
    t::coroutine coroutine; // 用於保存協程句柄

    auto resume = [&] { coroutine.resume(); };

    for (auto i = 0; i < 3; i++)
    {
        co_await f::async_resume(coroutine, [&] { asio::post(_io_context, resume); });

        handle(_io_context, i);
    }
}

int main()
{
    std::string log_format{"[%C-%m-%d %T.%e] [%^%L%$] [%-20!!:%4#] %v"};
    spdlog::set_pattern(log_format);

    asio::io_context io_context;

    auto count = common::time_count(
        [&]
        {
            accept_handle(io_context);
            io_context.run();
        });

    SPDLOG_INFO("used times: {}", count);

    return 0;
}
