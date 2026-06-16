module;

#include <boost/asio.hpp>
#include <exec/asio/use_sender.hpp>
#include <exec/async_scope.hpp>
#include <exec/static_thread_pool.hpp>
#include <exec/task.hpp>
#include <spdlog/spdlog.h>
#include <stdexec/execution.hpp>

module test.stdexec;

import std;

namespace ex = stdexec;
using namespace std::chrono_literals;
namespace asio = boost::asio;
using asio::steady_timer;
using boost::system::error_code;

// 讀循環
exec::task<void> read_loop(steady_timer& timer, std::unordered_map<int, std::string>& result, int id)
{
    for (int j = 0; j < 3; ++j)
    {
        timer.expires_after(100ms);
        auto [ec] = co_await timer.async_wait(asio::as_tuple(exec::asio::use_sender));
        if (ec)
        {
            SPDLOG_ERROR("{}", ec.message());
            co_return;
        }
        result[id] += "r";
    }
}

// 連接處理
exec::task<void> connection_handle(steady_timer timer, int id, std::unordered_map<int, std::string>& result)
{
    result[id] = std::to_string(id);
    co_await read_loop(timer, result, id);
    timer.expires_after(100ms);
    auto [ec] = co_await timer.async_wait(asio::as_tuple(exec::asio::use_sender));
    if (ec)
    {
        SPDLOG_ERROR("{}", ec.message());
        co_return;
    }
    result[id] += "w";
}

// 頂層任務
exec::task<void> accept_main(asio::io_context& ctx, std::unordered_map<int, std::string>& result,
                             exec::async_scope& scope)
{
    steady_timer accept_timer(ctx);
    for (int i = 0; i < 3; ++i)
    {
        accept_timer.expires_after(100ms);
        auto [ec] = co_await accept_timer.async_wait(asio::as_tuple(exec::asio::use_sender));
        if (ec)
        {
            SPDLOG_ERROR("{}", ec.message());
            co_return;
        }
        SPDLOG_INFO("new connection: {}", i);
        scope.spawn(connection_handle(steady_timer(ctx), i, result));
    }
}

// 測試入口 - 完全移除 starts_on
// 策略：在 sync_wait 外部創建線程，手動運行 accept_main 在該線程上
std::unordered_map<int, std::string> test_stdexec()
{
    exec::static_thread_pool pool(2);
    asio::io_context ctx;
    exec::async_scope scope;
    std::unordered_map<int, std::string> result;

    auto guard = asio::make_work_guard(ctx);
    std::thread io_thread([&ctx]() { ctx.run(); });

    // 在 pool 線程上執行 accept_main
    auto coro_handle = std::async(std::launch::async, [&]() {
        ex::sync_wait(ex::just() | ex::let_value([&]() {
                          return ex::schedule(pool.get_scheduler()) |
                                 ex::then([&]() { ex::sync_wait(accept_main(ctx, result, scope)); });
                      }));
    });
    coro_handle.wait(); // 等待協程完成

    ex::sync_wait(scope.on_empty());

    guard.reset();
    ctx.stop();
    io_thread.join();

    return result;
}
