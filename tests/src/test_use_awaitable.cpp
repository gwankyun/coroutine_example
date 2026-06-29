module;

#include <boost/asio.hpp>
#include <spdlog/spdlog.h>

module test.use_awaitable;

import std;

using namespace std::chrono_literals;
namespace asio = boost::asio;
using asio::steady_timer;
using boost::system::error_code;

// 1. 協程函數：用 asio::awaitable<T> 替代自定義 task
asio::awaitable<void> connection_handle(steady_timer _connection, int _id,
                                        std::unordered_map<int, std::string>& _result)
{
    auto executor = co_await asio::this_coro::executor; // 獲取當前執行器

    _connection.expires_after(100ms);
    {
        auto [ec] = co_await _connection.async_wait(asio::as_tuple(asio::use_awaitable));

        if (ec)
        {
            SPDLOG_ERROR("{}", ec.message());
            co_return;
        }
    }

    _result[_id] = std::to_string(_id);

    for (int i = 0; i < 3; ++i)
    {
        _connection.expires_after(100ms);

        // 2. 用 co_await + use_awaitable 替代手動 async_resume
        auto [ec] = co_await _connection.async_wait(asio::as_tuple(asio::use_awaitable));

        if (ec)
        {
            SPDLOG_ERROR("{}", ec.message());
            co_return;
        }
        _result[_id] += "r";
    }

    _connection.expires_after(100ms);
    auto [ec] = co_await _connection.async_wait(asio::as_tuple(asio::use_awaitable));

    if (ec)
    {
        SPDLOG_ERROR("{}", ec.message());
        co_return;
    }
    _result[_id] += "w";
}

asio::awaitable<void> accept_handle(std::unordered_map<int, std::string>& _result)
{
    auto executor = co_await asio::this_coro::executor;

    steady_timer acceptor(executor);

    for (auto i = 0; i != 3; ++i)
    {
        acceptor.expires_after(100ms);
        auto [ec] = co_await acceptor.async_wait(asio::as_tuple(asio::use_awaitable));

        if (ec)
        {
            SPDLOG_ERROR("{}", ec.message());
            co_return;
        }

        steady_timer connection(executor);

        // 3. 用 co_spawn 啟動子協程（fire-and-forget）
        asio::co_spawn(executor, connection_handle(std::move(connection), i, _result), asio::detached);
    }
}

// 4. 入口：co_spawn 啟動頂層協程
std::unordered_map<int, std::string> test_use_awaitable()
{
    asio::io_context context;
    std::unordered_map<int, std::string> result;

    asio::co_spawn(context, accept_handle(result), asio::detached);

    context.run();
    return result;
}
