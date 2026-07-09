module;

#include <boost/asio.hpp>
#include <boost/context/continuation.hpp>
#include <boost/scope/defer.hpp>
#include <spdlog.hpp>

module test.context.continuation;

import std;
import io_scheduler;
import spdlog;

using namespace std::chrono_literals;
namespace asio = boost::asio;
using asio::steady_timer;
using boost::system::error_code;
namespace ctx = boost::context;

struct Context
{
    std::unordered_map<int, std::string> result;
    /// awake_cont用於存儲需要喚醒的協程ID，當異步操作完成時，喚醒協程繼續執行。
    std::queue<int> awake_cont;
    /// coro_cont用於存儲協程ID和對應的協程對象，當需要喚醒協程時，從coro_cont中找到對應的協程對象。
    std::unordered_map<int, std::unique_ptr<ctx::continuation>> coro_cont;
    std::unordered_map<int, std::unique_ptr<steady_timer>> timer_cont;
};

void resume(ctx::continuation& _fiber)
{
    _fiber = _fiber.resume();
}

auto g_time = 100ms;

ctx::continuation make_continuation(Context& _context, int _id, steady_timer& _timer)
{
    auto continuation = [&, _id](ctx::continuation&& _sink) {
        auto& awake_cont = _context.awake_cont;
        auto& result = _context.result;
        // 協程結束時自動喚醒，確保協程能夠正常退出。
        auto awake = [&] { awake_cont.push(_id); };
        BOOST_SCOPE_DEFER[&]
        {
            awake();
        };

        error_code e;

        auto cb = [&](error_code& e) {
            return [&](error_code _e) {
                e = _e;
                awake();
            };
        };

        SPDLOG_INFO("");
        auto& timer = _timer;
        timer.expires_after(g_time);
        timer.async_wait(cb(e));
        // 協程暫停，等待異步操作完成後再繼續執行。
        resume(_sink);
        if (e)
        {
            SPDLOG_ERROR("{}", e.message());
            return std::move(_sink);
        }
        result[_id] = std::to_string(_id);

        SPDLOG_INFO("");
        // 支持循環內的異步操作，協程可以在循環內多次暫停和繼續執行，直到循環結束。
        for (int i = 0; i < 3; ++i)
        {
            timer.expires_after(g_time);
            timer.async_wait(cb(e));
            resume(_sink);
            if (e)
            {
                SPDLOG_ERROR("{}", e.message());
                return std::move(_sink);
            }
            result[_id] += "r";
        }

        SPDLOG_INFO("");
        timer.expires_after(g_time);
        timer.async_wait(cb(e));
        resume(_sink);
        if (e)
        {
            SPDLOG_ERROR("{}", e.message());
            return std::move(_sink);
        }
        result[_id] += "w";

        asio::post(timer.get_executor(), [&, _id] { _context.timer_cont.erase(_id); });

        return std::move(_sink);
    };
    return ctx::callcc(continuation);
};

std::unordered_map<int, std::string> test_context_continuation()
{
    asio::io_context io_ctx;
    Context context;
    auto& result = context.result;

    auto time = 100ms;

    auto& awake_cont = context.awake_cont;
    auto& coro_cont = context.coro_cont;

    auto make_accept = [&](steady_timer& _acceptor) {
        auto continuation = [&](ctx::continuation&& _sink) {
            // 協程結束時自動喚醒，確保協程能夠正常退出。
            auto awake = [&] { awake_cont.push(0); };
            BOOST_SCOPE_DEFER[&]
            {
                awake();
            };

            error_code e;

            auto cb = [&](error_code& e) {
                return [&](error_code _e) {
                    e = _e;
                    awake();
                };
            };

            SPDLOG_INFO("");
            auto& acceptor = _acceptor;

            for (int id = 1; id < 4; ++id)
            {
                acceptor.expires_after(g_time);
                acceptor.async_wait(cb(e));
                resume(_sink);

                if (e)
                {
                    SPDLOG_ERROR("{}", e.message());
                    return std::move(_sink);
                }

                context.timer_cont[id] = std::make_unique<steady_timer>(acceptor.get_executor());
                auto& timer = *context.timer_cont[id];
                coro_cont[id] = std::make_unique<ctx::continuation>(make_continuation(context, id, timer));
            }
            return std::move(_sink);
        };
        return ctx::callcc(continuation);
    };

    /// 對稱協程可將調度算法放到子協程中，子協程可以互相喚醒
    auto main_continuation = [&] {
        steady_timer acceptor(io_ctx);
        // 急切協程，創建後自動運行
        coro_cont[0] = std::make_unique<ctx::continuation>(make_accept(acceptor));

        run_scheduler<ctx::continuation>(io_ctx, coro_cont, awake_cont, [](auto& _c) { resume(_c); });
    };

    auto main = std::make_unique<ctx::continuation>(ctx::callcc([&](ctx::continuation&& _sink) {
        SPDLOG_INFO("enter main");
        main_continuation();
        return std::move(_sink);
    }));

    return result;
}
