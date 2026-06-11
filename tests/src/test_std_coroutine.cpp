module;

#include <boost/asio.hpp>
#include <spdlog/spdlog.h>

module test.std_coroutine;

import std;

using namespace std::chrono_literals;
namespace asio = boost::asio;
using asio::steady_timer;
using boost::system::error_code;

namespace coro
{
    using handle = std::coroutine_handle<>;

    /// fire-and-forget 協程返回對象，僅作為啟動協程的「令牌」，調用者啓動後即失去控制權
    struct task
    {
        struct promise_type
        {
            task get_return_object()
            {
                return {}; // 不持有 coroutine_handle，調用者無法控制協程
            }
            std::suspend_never initial_suspend()
            {
                return {}; // 協程立即執行，不需外部 .resume() 啟動
            }
            std::suspend_never final_suspend() noexcept
            {
                return {}; // 協程結束自動銷燬幀，無泄漏但也無法事後訪問
            }
            void unhandled_exception()
            {
                std::terminate(); // 異常直接崩潰，無恢復機制
            }
            void return_void() {}
        };
    };

    /// 橋接協程與異步回調的核心機制：
    /// co_await awaiter{fn} → 協程掛起 → fn(調用者handle) 執行 →
    /// fn 內發起異步操作並註冊回調 → 回調完成時 .resume() → 協程從 co_await 後恢復
    // 通用模板：支持異步回調返回值
    struct awaiter
    {
        using Fn = std::function<void(const handle&)>;
        explicit awaiter(Fn _f) : f(_f) {}
        bool await_ready()
        {
            return false;
        }
        void await_suspend(handle _h)
        {
            f(_h);
        }
        void await_resume() {}
        Fn f;
    };

    /// @brief 在协程中处理回调
    /// @param _coroutine 协程
    /// @param _f 回调，在里面执行_coroutine.resume()即恢复
    /// @return 可供co_await的协程
    awaiter suspend_async(handle& _coroutine, std::function<void()> _f)
    {
        auto fn = [&, _f](const handle& _coro) {
            _coroutine = _coro; // 把协程句柄传出去
            _f();               // 业务回调
        };
        return awaiter{fn};
    }
} // namespace coro

coro::task connection_handle(asio::io_context& context, int id, std::unordered_map<int, std::string>& result)
{
    coro::handle coroutine; // 用於保存協程句柄
    error_code ec;
    auto time = 100ms;

    result[id] = std::to_string(id);

    auto cb = [&](error_code& ec) {
        return [&](error_code _ec) {
            ec = _ec;
            coroutine.resume();
        };
    };

    for (int i = 0; i < 3; ++i)
    {
        steady_timer read(context, time);

        co_await coro::suspend_async(coroutine, [&] { read.async_wait(cb(ec)); });

        if (ec)
        {
            SPDLOG_ERROR("{}", ec.message());
            co_return;
        }
        result[id] += "r";
    }

    steady_timer write(context, time);

    co_await coro::suspend_async(coroutine, [&] { write.async_wait(cb(ec)); });

    if (ec)
    {
        SPDLOG_ERROR("{}", ec.message());
        co_return;
    }
    result[id] += "w";
    co_return;
}

coro::task accept_handle(asio::io_context& context, std::unordered_map<int, std::string>& result)
{
    coro::handle coroutine; // 用於保存協程句柄
    error_code ec;
    auto time = 100ms;

    auto cb = [&](error_code& ec) {
        return [&](error_code _ec) {
            ec = _ec;
            coroutine.resume();
        };
    };

    for (auto i = 0; i != 3; ++i)
    {
        steady_timer accept(context, time);

        co_await coro::suspend_async(coroutine, [&] { accept.async_wait(cb(ec)); });

        if (ec)
        {
            SPDLOG_ERROR("{}", ec.message());
            co_return;
        }

        connection_handle(context, i, result);
    }
    co_return;
}

std::unordered_map<int, std::string> test_std_coroutine()
{
    asio::io_context context;

    std::unordered_map<int, std::string> result;
    auto time = 100ms;

    accept_handle(context, result);

    context.run();
    return result;
}
