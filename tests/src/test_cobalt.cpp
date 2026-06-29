module;

#include <boost/asio.hpp>
#include <boost/cobalt.hpp>
#include <spdlog/spdlog.h>

module test.cobalt;

import std;

namespace cobalt = boost::cobalt;
using namespace std::chrono_literals;
namespace asio = boost::asio;
using asio::steady_timer;
using boost::system::error_code;

// cobalt::promise 是 eager 协程（立即执行），类似原来的 task
cobalt::promise<void> connection_handle(steady_timer connection, int id, std::unordered_map<int, std::string>& result)
{
    auto executor = co_await cobalt::this_coro::executor;

    connection.expires_after(100ms);
    {
        auto [ec] = co_await connection.async_wait(asio::as_tuple(cobalt::use_op));

        if (ec)
        {
            SPDLOG_ERROR("{}", ec.message());
            co_return;
        }
    }

    result[id] = std::to_string(id);

    for (int i = 0; i < 3; ++i)
    {
        connection.expires_after(100ms);
        auto [ec] = co_await connection.async_wait(asio::as_tuple(cobalt::use_op));

        if (ec)
        {
            SPDLOG_ERROR("{}", ec.message());
            co_return;
        }
        result[id] += "r";
    }

    connection.expires_after(100ms);
    auto [ec] = co_await connection.async_wait(asio::as_tuple(cobalt::use_op));

    if (ec)
    {
        SPDLOG_ERROR("{}", ec.message());
        co_return;
    }
    result[id] += "w";
}

cobalt::promise<void> accept_handle(std::unordered_map<int, std::string>& result)
{
    auto executor = co_await cobalt::this_coro::executor;

    steady_timer acceptor(executor);

    for (auto i = 0; i != 3; ++i)
    {
        acceptor.expires_after(100ms);
        auto [ec] = co_await acceptor.async_wait(asio::as_tuple(cobalt::use_op));

        if (ec)
        {
            SPDLOG_ERROR("{}", ec.message());
            co_return;
        }

        SPDLOG_INFO("new connection: {}", i);

        steady_timer connection(executor);

        connection_handle(std::move(connection), i, result).detach(); // eager，立即开始执行
    }
}

std::unordered_map<int, std::string> test_cobalt()
{
    asio::io_context context;
    cobalt::this_thread::set_executor(context.get_executor());
    std::unordered_map<int, std::string> result;
    accept_handle(result).detach();
    context.run();
    return result;
}
