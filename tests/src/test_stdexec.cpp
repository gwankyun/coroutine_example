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

auto g_time = 100ms;

// 讀循環
exec::task<void> read_loop(steady_timer& timer, std::unordered_map<int, std::string>& result, int id)
{
    for (int j = 0; j < 3; ++j)
    {
        timer.expires_after(g_time);
        SPDLOG_INFO("id: {} j: {}", id, j);
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
    SPDLOG_INFO("enter id: {}", id);
    timer.expires_after(g_time);
    {
        auto [ec] = co_await timer.async_wait(asio::as_tuple(exec::asio::use_sender));
        if (ec)
        {
            SPDLOG_ERROR("{}", ec.message());
            co_return;
        }
    }

    result[id] = std::to_string(id);

    co_await read_loop(timer, result, id);

    timer.expires_after(g_time);
    auto [ec] = co_await timer.async_wait(asio::as_tuple(exec::asio::use_sender));
    if (ec)
    {
        SPDLOG_ERROR("{}", ec.message());
        co_return;
    }
    result[id] += "w";
    SPDLOG_INFO("exit id: {}", id);
}

// 頂層任務
exec::task<void> accept_main(asio::io_context& ctx, std::unordered_map<int, std::string>& result,
                             exec::async_scope& scope)
{
    steady_timer accept_timer(ctx);
    for (int i = 1; i < 4; ++i)
    {
        accept_timer.expires_after(g_time);
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
    exec::static_thread_pool pool(1);
    asio::io_context io_ctx;
    exec::async_scope scope; // 管理任務
    std::unordered_map<int, std::string> result;

    auto guard = asio::make_work_guard(io_ctx);            // 防止 io_ctx.run() 無任務時提前退出
    std::jthread io_thread([&io_ctx]() { io_ctx.run(); }); // 專門跑 asio 事件循環的線程

    // 在 pool 線程上執行 accept_main
    auto coro_handle = std::async(std::launch::async, [&]() {
        ex::sync_wait(ex::just() |                                    // 創建空 Sender
                      ex::let_value([&]() {                           // 在值上下文中執行
                          return ex::schedule(pool.get_scheduler()) | // 切到 pool 線程
                                 ex::then([&]() {                     // 在 pool 線程上執行
                                     ex::sync_wait(accept_main(io_ctx, result, scope));
                                 });
                      }));
    });
    coro_handle.wait(); // 等待 accept_main 協程完成

    // 等待所有子協程完成
    ex::sync_wait(scope.on_empty());

    guard.reset(); // 取消 work_guard，允許 io_ctx.run() 返回
    io_ctx.stop(); // 通知 io_ctx 停止

    return result;
}
