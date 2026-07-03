module;

#include <boost/asio.hpp>
#include <spdlog/spdlog.h>

module test.std_coroutine;

import std;
import stdcoro;

using namespace std::chrono_literals;
namespace asio = boost::asio;
using asio::steady_timer;
using boost::system::error_code;

auto g_time = 100ms;

coro::eager_task connection_handle(steady_timer _connection, int _id, std::unordered_map<int, std::string>& _result)
{
    error_code ec;

    auto cb = [&](error_code& ec, coro::handle _h) {
        return [&, _h](error_code _ec) {
            ec = _ec;
            _h.resume();
        };
    };

    _connection.expires_after(g_time);
    co_await coro::suspend_async([&](auto _h) { _connection.async_wait(cb(ec, _h)); });
    if (ec)
    {
        SPDLOG_ERROR("{}", ec.message());
        co_return;
    }

    _result[_id] = std::to_string(_id);

    for (int i = 0; i < 3; ++i)
    {
        _connection.expires_after(g_time);
        co_await coro::suspend_async([&](auto _h) { _connection.async_wait(cb(ec, _h)); });
        if (ec)
        {
            SPDLOG_ERROR("{}", ec.message());
            co_return;
        }

        _result[_id] += "r";
    }

    _connection.expires_after(g_time);
    co_await coro::suspend_async([&](auto _h) { _connection.async_wait(cb(ec, _h)); });
    if (ec)
    {
        SPDLOG_ERROR("{}", ec.message());
        co_return;
    }

    _result[_id] += "w";
    co_return;
}

coro::lazy_task accept_handle(asio::io_context& _context, std::unordered_map<int, std::string>& _result)
{
    error_code ec;

    auto cb = [&](error_code& ec, coro::handle _h) {
        return [&, _h](error_code _ec) {
            ec = _ec;
            _h.resume();
        };
    };

    steady_timer acceptor(_context);

    for (auto i = 0; i != 3; ++i)
    {
        acceptor.expires_after(g_time);

        co_await coro::suspend_async([&](auto _h) { acceptor.async_wait(cb(ec, _h)); });

        if (ec)
        {
            SPDLOG_ERROR("{}", ec.message());
            co_return;
        }

        steady_timer connection(_context);

        auto t = connection_handle(std::move(connection), i, _result);
        t.detach([](std::exception_ptr ex) {
            try
            {
                std::rethrow_exception(ex);
            }
            catch (const std::exception& e)
            {
                SPDLOG_ERROR("{}", e.what());
            }
            catch (...)
            {
                SPDLOG_ERROR("unknown exception");
            }
        });
    }
    co_return;
}

std::unordered_map<int, std::string> test_std_coroutine()
{
    asio::io_context io_ctx;

    std::unordered_map<int, std::string> result;

    try
    {
        auto t = accept_handle(io_ctx, result);
        t.resume();

        do
        {
            SPDLOG_INFO("t: {}", t.done());
            io_ctx.run_one();
        } while (!t.done());
    }
    catch (const std::exception& e)
    {
        SPDLOG_ERROR("{}", e.what());
    }

    io_ctx.run();

    return result;
}
